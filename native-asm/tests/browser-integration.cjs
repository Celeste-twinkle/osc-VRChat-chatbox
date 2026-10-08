const assert = require('node:assert/strict');
const path = require('node:path');
const fs = require('node:fs');
const readline = require('node:readline');
const { spawn } = require('node:child_process');
const { launch, until, pause } = require('../ui/tests/browser-harness.cjs');
const executable = path.resolve(process.argv[2] || path.join(__dirname, '../dist/vrc-chatbox-osc-asm.exe'));
const fixture = spawn(process.env.PYTHON || 'python', [path.join(__dirname, 'native-fixture.py'), executable],
  { windowsHide: true, stdio: ['pipe','pipe','pipe'] });
let id = 0, passed = 0;
const pending = new Map();
const ready = new Promise((resolve, reject) => {
  const timer = setTimeout(() => reject(new Error('Native fixture startup timeout')),10000);
  readline.createInterface({input:fixture.stdout}).on('line',line=>{
    const message=JSON.parse(line);
    if(message.ready) {clearTimeout(timer);resolve(message);return;}
    const request=pending.get(message.id);
    if(request) {clearTimeout(request.timer);pending.delete(message.id);request.resolve(message.value);}
  });
  fixture.once('exit',code=>{if(code) reject(new Error('Native fixture exited: '+code));});
});
fixture.stderr.on('data',data=>process.stderr.write(data));
function command(action) {
  const next=++id;
  return new Promise((resolve,reject)=>{
    const timer=setTimeout(()=>{pending.delete(next);reject(new Error('Native fixture command timeout: '+action));},5000);
    pending.set(next,{resolve,reject,timer});fixture.stdin.write(JSON.stringify({id:next,action})+'\n');
  });
}
function init(canWriteSettings) {
  return `(() => {
    const nativeFetch=window.fetch.bind(window);
    window.__test={requests:[],errors:[],holdNextSave:false};
    window.addEventListener('error',event=>__test.errors.push(event.message));
    window.fetch=(url,options={})=>{
      __test.requests.push({url,method:options.method||'GET',body:options.body});
      if (url==='/settings'&&options.method==='POST'&&!${canWriteSettings}) return Promise.resolve(new Response(null,{status:204}));
      if(String(url).startsWith('https://api.mymemory.translated.net')) return Promise.resolve(new Response(JSON.stringify({responseStatus:200,responseData:{translatedText:'English translation'}}),{headers:{'Content-Type':'application/json'}}));
      if(url==='/quicktext'&&options.method==='POST'&&__test.holdNextSave) {
        __test.holdNextSave=false;
        return new Promise(resolve=>{__test.releaseSave=()=>nativeFetch(url,options).then(resolve);});
      }
      return nativeFetch(url,options);
    };
  })();`;
}
const settled=client=>until(client,`!!JSON.parse(localStorage.getItem('vrcChatboxQuickText')) && !JSON.parse(localStorage.getItem('vrcChatboxQuickText')).pending && !document.getElementById('qtInput').disabled`);
const add=(client,text)=>client.eval(`document.getElementById('qtInput').value=${JSON.stringify(text)};document.getElementById('qtAdd').click()`);
const edit=(client,text)=>client.eval(`document.querySelector('[data-qt="edit"]').click();document.getElementById('qtInput').value=${JSON.stringify(text)};document.getElementById('qtAdd').click()`);
const pass=name=>{passed++;console.log('PASS '+name);};

function oscStrings(hex) {
  const buffer=Buffer.from(hex,'hex');
  let offset=0;
  function string() {const end=buffer.indexOf(0,offset);const value=buffer.subarray(offset,end).toString('utf8');offset=(end+4)&~3;return value;}
  const address=string(),types=string();
  return {address,types,text:types.includes('s')?string():null};
}
async function receiveOsc(address) {
  for (let i=0;i<10;i++) {
    const packet=oscStrings((await command('osc')).packet);
    if(packet.address===address) return packet;
  }
  throw new Error('Expected OSC packet not received: '+address);
}

async function main() {
  const info=await ready, browser=await launch();
  let first,second,third;
  try {
    first=await browser.page(info.url,init(info.canWriteSettings));
    await settled(first);
    const raw='  中文\nsecond line  ';
    await add(first,raw);
    await settled(first);
    assert.equal((await command('snapshot')).quicktext.items[0].text,raw);
    pass('browser saves raw Unicode text through the native ETag API');

    second=await browser.page(info.url,init(info.canWriteSettings));
    await settled(second);
    await first.eval(`__test.holdNextSave=true`);
    await edit(first,'edit from page A');
    await until(first,`typeof __test.releaseSave==='function'`);
    await edit(second,'edit from page B');
    await settled(second);
    await first.eval(`__test.releaseSave()`);
    await until(first,`document.getElementById('qtCount').textContent.includes('conflicting')`);
    await settled(first);
    assert.deepEqual((await command('snapshot')).quicktext.items.map(item=>item.text).sort(),['edit from page A','edit from page B']);
    pass('two browser pages preserve both conflicting native file edits');

    await first.eval(`document.querySelector('[data-qt="delete"]').click();document.querySelector('[data-qt="delete"]').click()`);
    await settled(first);
    third=await browser.page(info.url,init(info.canWriteSettings));
    await settled(third);
    assert.deepEqual((await command('snapshot')).quicktext.items,[]);
    pass('an intentionally empty native file stays empty on a new page');

    await third.eval(`__test.holdNextSave=true`);
    await add(third,'recover after closing the page');
    await until(third,`typeof __test.releaseSave==='function'`);
    await browser.close(third);
    third=await browser.page(info.url,init(info.canWriteSettings));
    await settled(third);
    assert.equal((await command('snapshot')).quicktext.items[0].text,'recover after closing the page');
    pass('closing a page with an unsent save recovers pending text');

    await edit(third,raw);
    await settled(third);
    await third.eval(`document.getElementById('text').value='draft remains';document.getElementById('qtSend').click()`);
    await until(third,`__test.requests.some(r=>r.url==='/send')`);
    const rawPacket=await receiveOsc('/chatbox/input');
    assert.equal(rawPacket.address,'/chatbox/input');
    assert.equal(rawPacket.text,raw);
    assert.equal(await third.eval(`document.getElementById('text').value`),'draft remains');
    pass('native OSC packet keeps raw quick text and leaves the draft intact');

    await third.eval(`document.getElementById('text').value='typing test';document.getElementById('text').dispatchEvent(new Event('input'))`);
    const typingOn=await receiveOsc('/chatbox/typing');
    assert.equal(typingOn.address,'/chatbox/typing');
    assert.equal(typingOn.types,',T');
    await third.eval(`const toggle=document.getElementById('typingOn');toggle.checked=false;toggle.dispatchEvent(new Event('change'))`);
    const typingOff=await receiveOsc('/chatbox/typing');
    assert.equal(typingOff.address,'/chatbox/typing');
    assert.equal(typingOff.types,',F');
    if(info.canWriteSettings) {
      for(let i=0;i<50;i++) {if(!(await command('snapshot')).settings.typingOn) break;await pause(20);}
      assert.equal((await command('snapshot')).settings.typingOn,false);
    }
    pass('typing toggle emits true/false OSC and persists when the startup value is absent');

    await third.eval(`const on=document.getElementById('trOn');on.checked=true;on.dispatchEvent(new Event('change'));document.getElementById('text').value='你好';document.getElementById('trButton').click()`);
    await until(third,`__test.requests.filter(r=>r.url==='/send').length===2`);
    const translated=await receiveOsc('/chatbox/input');
    assert.equal(translated.address,'/chatbox/input');
    assert.ok(translated.text.indexOf('English translation')<translated.text.indexOf('你好'));
    await until(third,`__test.requests.some(r=>r.url==='/history')`);
    assert.equal((await command('snapshot')).history[0].order,'translation-first');
    pass('translation output order and native history still work');

    assert.deepEqual(await command('second-instance'),{exitCode:0,serverAlive:true});
    pass('packaged helper keeps its single-instance behavior');
    for(const client of [first,second,third]) assert.deepEqual(await client.eval(`__test.errors`),[]);

    if(process.env.QA_SCREENSHOT_DIR) {
      fs.mkdirSync(process.env.QA_SCREENSHOT_DIR,{recursive:true});
      await third.send('Emulation.setDeviceMetricsOverride',{width:375,height:812,deviceScaleFactor:1,mobile:false});
      const image=await third.send('Page.captureScreenshot',{format:'png'});
      fs.writeFileSync(path.join(process.env.QA_SCREENSHOT_DIR,'release-mobile-en.png'),Buffer.from(image.data,'base64'));
      await third.send('Emulation.setDeviceMetricsOverride',{width:1280,height:900,deviceScaleFactor:1,mobile:false});
      await third.eval(`document.getElementById('settingsBtn').click()`);
      const settings=await third.send('Page.captureScreenshot',{format:'png'});
      fs.writeFileSync(path.join(process.env.QA_SCREENSHOT_DIR,'release-settings-en.png'),Buffer.from(settings.data,'base64'));
    }
  } finally {
    await browser.stop();
    await command('exit');
    fixture.stdin.end();
  }
  console.log('Native browser integration: '+passed+' passed');
}
main().catch(error=>{console.error(error.stack);process.exitCode=1;fixture.stdin.end();});
