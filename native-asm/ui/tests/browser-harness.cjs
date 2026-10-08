const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const { spawn } = require('node:child_process');
const pause = ms => new Promise(resolve => setTimeout(resolve, ms));

class CDP {
  constructor(url) {
    this.socket = new WebSocket(url);
    this.next = 0;
    this.pending = new Map();
    this.socket.addEventListener('message', event => {
      const message = JSON.parse(event.data), request = this.pending.get(message.id);
      if (!request) return;
      this.pending.delete(message.id);
      clearTimeout(request.timer);
      if (message.error) request.reject(new Error(JSON.stringify(message.error)));
      else request.resolve(message.result);
    });
    this.ready = new Promise((resolve, reject) => {
      this.socket.addEventListener('open', resolve, { once: true });
      this.socket.addEventListener('error', reject, { once: true });
    });
  }
  async send(method, params = {}) {
    await this.ready;
    const id = ++this.next;
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => { this.pending.delete(id); reject(new Error(`Timeout: ${method}`)); }, 10000);
      this.pending.set(id, { resolve, reject, timer });
      this.socket.send(JSON.stringify({ id, method, params }));
    });
  }
  async eval(expression) {
    const result = await this.send('Runtime.evaluate', { expression, returnByValue: true, awaitPromise: true });
    if (result.exceptionDetails) throw new Error(JSON.stringify(result.exceptionDetails));
    return result.result.value;
  }
}

async function launch() {
  const candidates = [process.env.CHROME_PATH,
    'C:/Program Files/Google/Chrome/Application/chrome.exe',
    'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'].filter(Boolean);
  const executable = candidates.find(candidate => fs.existsSync(candidate));
  if (!executable) throw new Error('Set CHROME_PATH to a Chromium browser executable.');
  const tempRoot = fs.realpathSync(os.tmpdir());
  const profile = fs.mkdtempSync(path.join(tempRoot, 'vrc-chatbox-regression-'));
  const processHandle = spawn(executable, ['--headless=new', '--no-first-run',
    '--no-default-browser-check', '--disable-gpu', '--remote-debugging-port=0',
    `--user-data-dir=${profile}`, 'about:blank'], { windowsHide: true, stdio: 'ignore' });
  const portFile = path.join(profile, 'DevToolsActivePort');
  for (let i = 0; i < 200 && !fs.existsSync(portFile); i++) await pause(50);
  const port = Number(fs.readFileSync(portFile, 'utf8').split('\n')[0]);
  return {
    async page(url, script) {
      const target = await (await fetch(`http://127.0.0.1:${port}/json/new?about:blank`, { method: 'PUT' })).json();
      const client = new CDP(target.webSocketDebuggerUrl);
      client.targetId = target.id;
      await client.send('Page.enable');
      if (script) await client.send('Page.addScriptToEvaluateOnNewDocument', { source: script });
      await client.send('Page.navigate', { url });
      await until(client, `document.readyState === 'complete' && !!document.getElementById('qtSelect')`);
      return client;
    },
    async close(client) {
      await fetch(`http://127.0.0.1:${port}/json/close/${client.targetId}`);
      client.socket.close();
    },
    async stop() {
      const stopped = new Promise(resolve => processHandle.once('exit', resolve));
      processHandle.kill();
      await Promise.race([stopped, pause(3000)]);
      const resolved = path.resolve(profile);
      if (path.dirname(resolved) !== path.resolve(tempRoot) || !path.basename(resolved).startsWith('vrc-chatbox-regression-')) {
        throw new Error('Refusing to remove a browser profile outside the test temp directory.');
      }
      try { fs.rmSync(resolved, { recursive: true, force: true, maxRetries: 3, retryDelay: 100 }); } catch {}
    },
  };
}

async function until(client, expression) {
  for (let i = 0; i < 200; i++) {
    if (await client.eval(expression)) return;
    await pause(20);
  }
  throw new Error(`Condition did not become true: ${expression}`);
}

module.exports = { launch, until, pause };
