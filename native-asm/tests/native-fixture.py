"""Own an isolated native process and report its HTTP/OSC endpoints to Node."""
import json
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import winreg
from pathlib import Path

exe = Path(sys.argv[1]).resolve()
temporary = tempfile.TemporaryDirectory(prefix='vrc-chatbox-browser-integration-')
root = Path(temporary.name).resolve()
assert root.parent == Path(tempfile.gettempdir()).resolve()
assert root.name.startswith('vrc-chatbox-browser-integration-')
target = root / 'native-integration.exe'
shutil.copyfile(exe, target)
with socket.socket() as listener:
    listener.bind(('127.0.0.1', 0))
    port = listener.getsockname()[1]
udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udp.bind(('127.0.0.1', 0))
udp.settimeout(2)
settings = {'translate': False, 'provider': 'mymemory', 'startup': False,
            'startMinimized': False, 'lan': False, 'port': port,
            'oscPort': udp.getsockname()[1], 'typingOn': True, 'uiLang': 'en',
            'src': 'zh-CN', 'dst': 'en', 'format': 'both', 'bothOrder': 'translation-first'}
(root / 'settings.json').write_text(json.dumps(settings), encoding='utf-8')

try:
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Software\Microsoft\Windows\CurrentVersion\Run') as key:
        winreg.QueryValueEx(key, 'VRC Chatbox OSC')
    can_write_settings = False
except FileNotFoundError:
    can_write_settings = True

process = subprocess.Popen([str(target), '--minimized'], cwd=root, creationflags=subprocess.CREATE_NO_WINDOW)

def emit(value):
    print(json.dumps(value, ensure_ascii=True), flush=True)

try:
    for _ in range(100):
        try:
            with socket.create_connection(('127.0.0.1', port), timeout=0.1):
                break
        except OSError:
            if process.poll() is not None:
                raise RuntimeError('Native integration process exited before listening.')
            time.sleep(0.05)
    else:
        raise RuntimeError('Native integration process did not start.')
    emit({'ready': True, 'url': f'http://127.0.0.1:{port}/', 'root': str(root),
          'canWriteSettings': can_write_settings})
    for line in sys.stdin:
        request = json.loads(line)
        action = request['action']
        if action == 'snapshot':
            value = {}
            for name in ['quicktext', 'settings', 'history', 'prompts']:
                file = root / (name + '.json')
                value[name] = json.loads(file.read_text(encoding='utf-8')) if file.exists() else None
        elif action == 'osc':
            value = {'packet': udp.recv(8192).hex()}
        elif action == 'second-instance':
            child = subprocess.Popen([str(target), '--minimized'], cwd=root, creationflags=subprocess.CREATE_NO_WINDOW)
            value = {'exitCode': child.wait(timeout=5), 'serverAlive': process.poll() is None}
        elif action == 'exit':
            emit({'id': request['id'], 'value': True})
            break
        else:
            raise ValueError(action)
        emit({'id': request['id'], 'value': value})
finally:
    process.terminate()
    process.wait(timeout=5)
    udp.close()
    temporary.cleanup()
