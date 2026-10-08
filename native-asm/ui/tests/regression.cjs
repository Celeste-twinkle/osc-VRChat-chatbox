const assert = require('node:assert/strict');
const path = require('node:path');
const { pathToFileURL } = require('node:url');
const { launch, until, pause } = require('./browser-harness.cjs');
const url = pathToFileURL(path.resolve(__dirname, '../dist/index.html')).href;
const entry = (id, text) => ({ id, text });
let passed = 0;

function fixture(config = {}) {
  return `(() => {
    const config = ${JSON.stringify(config)};
    localStorage.clear();
    localStorage.setItem('vrcChatboxQuickText', JSON.stringify(config.local || []));
    window.__test = { requests: [], pendingPosts: [], server: config.server || [], exists: config.exists !== false,
      version: 1, errors: [], typing: [], failWrite: config.write === 'fail', read: config.read || '',
      clipboardCalls: 0, clipboardValue: '', maxPending: 0 };
    window.addEventListener('error', event => __test.errors.push(event.message));
    const tag = () => __test.exists ? '"v' + __test.version + '"' : '"missing00"';
    const response = (data, status=200, etag=tag()) => new Response(status === 204 ? null : JSON.stringify(data),
      {status, headers:{'Content-Type':'application/json',ETag:etag}});
    navigator.sendBeacon = (url, body) => {__test.requests.push({url,method:'BEACON',body}); if(url==='/typing') __test.typing.push(body);return true;};
    window.fetch = (url, options = {}) => {
      const method = options.method || 'GET';
      __test.requests.push({url,method,body:options.body,headers:options.headers});
      if (url === '/quicktext') {
        if (method === 'GET') {
          if (__test.read === 'reject') return Promise.reject(new TypeError('Failed read'));
          if (__test.read === '503') return Promise.resolve(response({},503));
          if (__test.read === 'invalid') return Promise.resolve(new Response('not-json',{headers:{ETag:tag()}}));
          const data = {items:structuredClone(__test.server)}, etag=tag();
          if (__test.read === 'deferred') return new Promise(resolve=>{__test.resolveRead=()=>resolve(response(data,200,etag));});
          return Promise.resolve(response(data,200,etag));
        }
        const items = JSON.parse(options.body).items, expected=options.headers['If-Match'];
        const save = () => {
          if (__test.failWrite) return response({},500);
          if (expected !== tag()) return response({},412);
          __test.server=items;__test.exists=true;__test.version++;return response(null,204);
        };
        if (config.write === 'deferred') return new Promise(resolve=>{
          __test.pendingPosts.push({items,finish:()=>resolve(save())});
          __test.maxPending=Math.max(__test.maxPending,__test.pendingPosts.length);
        });
        return Promise.resolve(save());
      }
      if (url === '/settings' && method === 'GET') {
        const data={translate:!!config.translate,provider:'mymemory',uiLang:config.lang || 'zh',typingOn:config.typingOn !== false};
        if (config.settingsRead==='deferred') return new Promise(resolve=>{__test.resolveSettings=()=>resolve(response(data));});
        return Promise.resolve(response(data));
      }
      if (url === '/prompts') return Promise.resolve(response({activeId:'',items:[]}));
      if (url === '/history' && method === 'GET') return Promise.resolve(response([]));
      if (url === '/lan-ip') return Promise.resolve(response({ip:'127.0.0.1',port:19001,enabled:false,allowed:false}));
      if (url === '/typing') __test.typing.push(options.body);
      return Promise.resolve(response(null,204));
    };
  })();`;
}

const cache = client => client.eval(`JSON.parse(localStorage.getItem('vrcChatboxQuickText'))`);
const add = (client, value) => client.eval(`(() => {document.getElementById('qtInput').value=${JSON.stringify(value)};document.getElementById('qtAdd').click();})()`);
const edit = (client, value) => client.eval(`(() => {document.querySelector('[data-qt="edit"]').click();document.getElementById('qtInput').value=${JSON.stringify(value)};document.getElementById('qtAdd').click();})()`);
const posts = client => client.eval(`__test.requests.filter(r=>r.url==='/quicktext'&&r.method==='POST').map(r=>JSON.parse(r.body).items)`);

async function main() {
  const browser = await launch();
  async function test(name, config, fn) {
    const client = await browser.page(url, fixture(config));
    try {
      if (!config.read) await until(client, `!document.getElementById('qtInput').disabled`);
      await fn(client);
      assert.deepEqual(await client.eval(`__test.errors`), [], 'Browser runtime errors');
      passed++;
      console.log('PASS ' + name);
    } finally { await browser.close(client); }
  }
  try {
    for (const read of ['reject','503','invalid']) {
      await test('failed initial read: ' + read, {read,local:[entry('cache','stale')],server:[entry('file','newer')]}, async client => {
        await until(client, `!document.getElementById('qtRetry').classList.contains('hide')`);
        assert.deepEqual(await posts(client), []);
        assert.deepEqual(await client.eval(`__test.server`), [entry('file','newer')]);
        assert.deepEqual(await cache(client), [entry('cache','stale')]);
        await client.eval(`__test.read='';document.getElementById('qtRetry').click()`);
        await until(client, `!document.getElementById('qtInput').disabled`);
        assert.deepEqual((await cache(client)).items, [entry('file','newer')]);
      });
    }
    await test('initial read blocks writes and preserves draft', {read:'deferred',server:[entry('file','saved')]}, async client => {
      await add(client,'draft before read');
      assert.deepEqual(await posts(client), []);
      await client.eval(`__test.resolveRead()`);
      await until(client, `!document.getElementById('qtInput').disabled`);
      assert.equal(await client.eval(`document.getElementById('qtInput').value`), 'draft before read');
      await client.eval(`document.getElementById('qtAdd').click()`);
      await until(client, `!JSON.parse(localStorage.getItem('vrcChatboxQuickText')).pending`);
      assert.deepEqual((await cache(client)).items.map(item=>item.text).sort(), ['draft before read','saved']);
    });
    await test('intentional empty file does not migrate stale cache', {server:[],local:[entry('old','deleted elsewhere')]}, async client => {
      assert.deepEqual((await cache(client)).items, []);
      assert.deepEqual(await posts(client), []);
    });
    await test('legacy browser data migrates only into a missing file', {exists:false,local:[entry('legacy','  raw\nline  ')]}, async client => {
      await until(client, `!JSON.parse(localStorage.getItem('vrcChatboxQuickText')).pending`);
      assert.deepEqual(await client.eval(`__test.server`), [entry('legacy','  raw\nline  ')]);
    });
    await test('edit and delete saves are serialized', {server:[entry('one','old')],write:'deferred'}, async client => {
      await edit(client,'changed');
      await client.eval(`document.querySelector('[data-qt="delete"]').click()`);
      assert.equal(await client.eval(`__test.pendingPosts.length`), 1);
      await client.eval(`__test.pendingPosts.shift().finish()`);
      await until(client, `__test.pendingPosts.length===1`);
      await client.eval(`__test.pendingPosts.shift().finish()`);
      await until(client, `!JSON.parse(localStorage.getItem('vrcChatboxQuickText')).pending`);
      assert.deepEqual(await client.eval(`__test.server`), []);
      assert.equal(await client.eval(`__test.maxPending`), 1);
    });
    let failedCache;
    await test('failed write keeps pending data', {server:[entry('one','old')],write:'fail'}, async client => {
      await edit(client,'unsaved change');
      await until(client, `!document.getElementById('qtRetry').classList.contains('hide')`);
      failedCache=await cache(client);
      assert.equal(failedCache.pending,true);
      assert.equal(failedCache.items[0].text,'unsaved change');
    });
    await test('reload recovers and retries a failed write', {server:[entry('one','old')],local:failedCache}, async client => {
      await until(client, `!JSON.parse(localStorage.getItem('vrcChatboxQuickText')).pending`);
      assert.equal((await cache(client)).items[0].text,'unsaved change');
      assert.equal(await client.eval(`__test.server[0].text`),'unsaved change');
    });
    await test('failed deletion survives reload', {server:[entry('one','old')],local:{items:[],pending:true,baseItems:[entry('one','old')]}}, async client => {
      await until(client, `!JSON.parse(localStorage.getItem('vrcChatboxQuickText')).pending`);
      assert.deepEqual(await client.eval(`__test.server`),[]);
    });
    await test('file conflict retains remote addition', {server:[entry('one','old')]}, async client => {
      await client.eval(`__test.server.push({id:'other',text:'from another page'});__test.version++`);
      await edit(client,'local change');
      await until(client, `!JSON.parse(localStorage.getItem('vrcChatboxQuickText')).pending`);
      assert.deepEqual((await cache(client)).items.map(item=>item.text).sort(),['from another page','local change']);
    });
    await test('conflicting edits preserve both versions', {server:[entry('one','old')]}, async client => {
      await client.eval(`__test.server[0].text='remote change';__test.version++`);
      await edit(client,'local change');
      await until(client, `!JSON.parse(localStorage.getItem('vrcChatboxQuickText')).pending`);
      const saved=(await cache(client)).items;
      assert.deepEqual(saved.map(item=>item.text).sort(),['local change','remote change']);
      assert.equal(new Set(saved.map(item=>item.id)).size,2);
    });
    await test('IME Enter does not add or send', {}, async client => {
      const result=await client.eval(`(() => {return ['qtInput','text'].map(id=>{const input=document.getElementById(id);input.value='composition';const e=new KeyboardEvent('keydown',{key:'Enter',isComposing:true,bubbles:true,cancelable:true});input.dispatchEvent(e);return {value:input.value,prevented:e.defaultPrevented};});})()`);
      assert.deepEqual(result,[{value:'composition',prevented:false},{value:'composition',prevented:false}]);
      assert.deepEqual(await posts(client),[]);
      assert.equal(await client.eval(`__test.requests.filter(r=>r.url==='/send').length`),0);
    });
    for (const success of [true,false]) {
      await test('clipboard API failure fallback: ' + success, {server:[entry('copy','  raw\nline  ')]}, async client => {
        await client.eval(`Object.defineProperty(navigator,'clipboard',{configurable:true,value:{writeText:()=>Promise.reject(new DOMException('Denied','NotAllowedError'))}});document.execCommand=()=>{__test.clipboardCalls++;__test.clipboardValue=document.activeElement.value;return ${success};};document.getElementById('qtCopy').click()`);
        await until(client, `__test.clipboardCalls===1`);
        assert.equal(await client.eval(`document.getElementById('message').classList.contains('e')`),!success);
        assert.equal(await client.eval(`__test.clipboardValue`),'  raw\nline  ');
        assert.equal(await client.eval(`document.querySelectorAll('textarea').length`),4);
      });
    }
    await test('raw send keeps draft and typing toggle clears once', {server:[entry('raw','  raw\nline  ')]}, async client => {
      await client.eval(`document.getElementById('text').value='draft';document.getElementById('text').dispatchEvent(new Event('input'))`);
      await until(client, `__test.typing.includes('true')`);
      await client.eval(`document.getElementById('qtSend').click()`);
      await until(client, `__test.requests.some(r=>r.url==='/send')`);
      assert.equal(await client.eval(`document.getElementById('text').value`),'draft');
      assert.deepEqual(await client.eval(`__test.requests.filter(r=>r.url==='/send').map(r=>r.body)`),['  raw\nline  ']);
      await client.eval(`const toggle=document.getElementById('typingOn');toggle.checked=false;toggle.dispatchEvent(new Event('change'));document.getElementById('text').dispatchEvent(new Event('input'))`);
      await pause(350);
      assert.deepEqual(await client.eval(`__test.typing`),['true','false']);
      assert.equal(await client.eval(`JSON.parse(__test.requests.filter(r=>r.url==='/settings'&&r.method==='POST').at(-1).body).typingOn`),false);
    });
    await test('pending recovery keeps entries beyond the file budget', {server:[],local:{items:Array.from({length:101},(_,i)=>entry('id'+i,'text '+i)),pending:true,baseItems:[]}}, async client => {
      assert.equal((await cache(client)).items.length,101);
      assert.equal((await cache(client)).pending,true);
      assert.deepEqual(await posts(client),[]);
      await add(client,'must not drop pending entries');
      assert.equal((await cache(client)).items.length,101);
    });
    await test('disabled typing stays off before settings finish loading', {typingOn:false,settingsRead:'deferred'}, async client => {
      await client.eval(`document.getElementById('text').value='early draft';document.getElementById('text').dispatchEvent(new Event('input'))`);
      await pause(350);
      assert.deepEqual(await client.eval(`__test.typing`),[]);
      await client.eval(`__test.resolveSettings()`);
      await until(client, `document.getElementById('typingOn').checked===false`);
      assert.deepEqual(await client.eval(`__test.typing`),[]);
    });
    for (const lang of ['zh','en','ja','ko']) for (const translate of [false,true]) {
      await test('320px layout: '+lang+', translate='+translate, {lang,translate,server:[entry('one','long text')]}, async client => {
        await client.send('Emulation.setDeviceMetricsOverride',{width:320,height:812,deviceScaleFactor:1,mobile:false});
        const overflow=await client.eval(`['button','trButton','clearBubble','copyTextBtn','clearBtn'].filter(id=>{const e=document.getElementById(id);return getComputedStyle(e).display!=='none'&&e.scrollWidth>e.clientWidth+1;})`);
        assert.deepEqual(overflow,[]);
        if(translate&&lang!=='zh') assert.notEqual(await client.eval(`document.getElementById('trButton').textContent`),'翻译发送');
      });
    }
  } finally { await browser.stop(); }
  console.log('Browser regression: '+passed+' passed');
}

main().catch(error=>{console.error(error.stack);process.exitCode=1;});
