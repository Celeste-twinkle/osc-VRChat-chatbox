"""Exercise a built native EXE in an isolated data directory (stdlib only)."""
import ctypes
import json
import os
import random
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import unittest
import urllib.error
import urllib.request
from pathlib import Path

EXE = Path(sys.argv.pop(1) if len(sys.argv) > 1 else Path(__file__).resolve().parents[1] / 'dist/vrc-chatbox-osc-asm.exe').resolve()


class NativeRegression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix='vrc-chatbox-native-regression-')
        cls.root = Path(cls.temp.name).resolve()
        assert cls.root.parent == Path(tempfile.gettempdir()).resolve()
        assert cls.root.name.startswith('vrc-chatbox-native-regression-')
        cls.executable = cls.root / 'native-regression.exe'
        shutil.copyfile(EXE, cls.executable)
        with socket.socket() as listener:
            listener.bind(('127.0.0.1', 0))
            cls.port = listener.getsockname()[1]
        cls.udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        cls.udp.bind(('127.0.0.1', 0))
        cls.settings = {'translate': False, 'provider': 'mymemory', 'startup': False,
                        'startMinimized': True, 'lan': False, 'port': cls.port,
                        'oscPort': cls.udp.getsockname()[1], 'typingOn': False}
        cls.start()

    @classmethod
    def start(cls):
        if not (cls.root / 'settings.json').exists():
            (cls.root / 'settings.json').write_text(json.dumps(cls.settings), encoding='utf-8')
        cls.process = subprocess.Popen([str(cls.executable), '--minimized'], cwd=cls.root,
                                       creationflags=subprocess.CREATE_NO_WINDOW)
        for _ in range(100):
            try:
                with socket.create_connection(('127.0.0.1', cls.port), timeout=0.1):
                    return
            except OSError:
                if cls.process.poll() is not None:
                    raise RuntimeError('The test EXE exited; another running helper may own the single-instance mutex.')
                time.sleep(0.05)
        raise RuntimeError('Native test server did not start.')

    @classmethod
    def stop(cls):
        cls.process.terminate()
        cls.process.wait(timeout=5)

    @classmethod
    def tearDownClass(cls):
        cls.stop()
        cls.udp.close()
        cls.temp.cleanup()

    def request(self, method='GET', body=None, endpoint='/quicktext', headers=None):
        data = body.encode('utf-8') if isinstance(body, str) else body
        request = urllib.request.Request(f'http://127.0.0.1:{self.port}{endpoint}', data=data,
                                         method=method, headers={'Content-Type': 'application/json', **(headers or {})})
        try:
            response = urllib.request.urlopen(request, timeout=5)
        except urllib.error.HTTPError as error:
            response = error
        with response:
            return response.status, response.read(), dict(response.headers)

    def setUp(self):
        self.original = json.dumps({'items': [{'id': 'one', 'text': 'original'}]})
        self.assertEqual(self.request('POST', self.original)[0], 204)

    def assert_rejected(self, body):
        self.assertEqual(self.request('POST', body)[0], 400, repr(body))
        self.assertEqual((self.root / 'quicktext.json').read_text(encoding='utf-8'), self.original)

    def test_json_and_schema_rejection_preserves_file(self):
        cases = ['{garbage}', '{"items":[1,]}', '{"items":[{"id":"a","text":"\\q"}]}',
                 '{"items":[]}{"other":1}', '{"items":[}', '{}', '{"items":"wrong"}',
                 '{"items":[1]}', '{"items":[{"text":"missing id"}]}',
                 '{"items":[{"id":1,"text":"wrong id"}]}',
                 '{"items":[{"id":"a","text":false}]}',
                 '{"items":[],"items":[]}', '{"items":[{"id":"a","id":"b","text":"x"}]}',
                 '{"items":[],"x":01}', '{"items":[],"x":1.}', '{"items":[],"x":1e}',
                 '{"items":[],"x":+1}', '{"items":[],"x":True}', '{"items":[],}',
                 '{"items":[{"id":"a","text":"raw\nnewline"}]}',
                 b'{"items":[{"id":"a","text":"\xc0\x80"}]}',
                 b'{"items":[{"id":"a","text":"\xed\xa0\x80"}]}',
                 b'{"items":[{"id":"a","text":"\xf4\x90\x80\x80"}]}']
        for body in cases:
            with self.subTest(body=body):
                self.assert_rejected(body)

    def test_unicode_whitespace_extra_properties_and_field_order(self):
        value = {'meta': [True, False, None, -1, 0, 1.25e-5, {'nested': 'ok'}],
                 'items': [{'text': '  中文 日本語 한국어 😀\n\t"\\  ', 'extra': {'x': 1}, 'id': 'unicode'}]}
        for ascii_only in [True, False]:
            body = '\r\n ' + json.dumps(value, ensure_ascii=ascii_only, indent=2) + '\n'
            self.assertEqual(self.request('POST', body)[0], 204)
            status, data, headers = self.request()
            self.assertEqual(status, 200)
            self.assertEqual(json.loads(data), value)
            self.assertIn('ETag', headers)

    def test_content_limit_and_entry_limit(self):
        prefix = '{"items":[{"id":"one","text":"'
        suffix = '"}]}'
        body = prefix + 'a' * (32768 - len(prefix) - len(suffix)) + suffix
        self.assertEqual(self.request('POST', body)[0], 204)
        self.assertEqual(self.request()[1].decode('utf-8'), body)
        self.assertEqual(self.request('POST', body + ' ')[0], 413)
        self.assertEqual(self.request()[1].decode('utf-8'), body)
        values = [{'id': str(i), 'text': 'x'} for i in range(100)]
        self.assertEqual(self.request('POST', json.dumps({'items': values}))[0], 204)
        self.assertEqual(self.request('POST', json.dumps({'items': values + [{'id': 'extra', 'text': 'x'}]}))[0], 400)
        self.assertEqual(len(json.loads(self.request()[1])['items']), 100)

    def test_etag_conflict_and_missing_file_migration(self):
        tag = self.request()[2]['ETag']
        updated = json.dumps({'items': [{'id': 'one', 'text': 'changed'}]})
        status, _, headers = self.request('POST', updated, headers={'If-Match': tag})
        self.assertEqual(status, 204)
        self.assertNotEqual(headers['ETag'], tag)
        self.assertEqual(self.request('POST', self.original, headers={'If-Match': tag})[0], 412)
        self.assertEqual(self.request()[1].decode('utf-8'), updated)
        (self.root / 'quicktext.json').unlink()
        status, data, headers = self.request()
        self.assertEqual(status, 200)
        self.assertEqual(json.loads(data), {'items': []})
        self.assertEqual(headers['ETag'], '"missing00"')
        self.assertEqual(self.request('POST', self.original, headers={'If-Match': headers['ETag']})[0], 204)
        self.assertEqual(self.request('POST', '{"items":[]}', headers={'If-Match': headers['ETag']})[0], 412)
        self.assertEqual(self.request('POST', '{"items":[]}')[0], 204)
        self.assertNotEqual(self.request()[2]['ETag'], '"missing00"')

    def test_corrupt_file_is_an_error_not_a_missing_file(self):
        (self.root / 'quicktext.json').write_bytes(b'{broken}')
        self.assertEqual(self.request()[0], 500)
        self.assertEqual((self.root / 'quicktext.json').read_bytes(), b'{broken}')

    def test_failed_temp_write_preserves_existing_file(self):
        temporary = self.root / 'quicktext.json.tmp'
        temporary.mkdir()
        try:
            self.assertEqual(self.request('POST', '{"items":[]}')[0], 500)
            self.assertEqual((self.root / 'quicktext.json').read_text(encoding='utf-8'), self.original)
        finally:
            temporary.rmdir()

    def test_fragmented_request_is_read_completely(self):
        body = json.dumps({'items': [{'id': 'fragmented', 'text': 'a' * 12000}]}).encode('utf-8')
        header = f'POST /quicktext HTTP/1.1\r\nHost: localhost\r\nContent-Length: {len(body)}\r\nConnection: close\r\n\r\n'.encode('ascii')
        with socket.create_connection(('127.0.0.1', self.port), timeout=5) as connection:
            connection.sendall(header + body[:20])
            time.sleep(0.05)
            connection.sendall(body[20:])
            response = b''
            while True:
                chunk = connection.recv(4096)
                if not chunk:
                    break
                response += chunk
        self.assertTrue(response.startswith(b'HTTP/1.1 204'))
        self.assertEqual(self.request()[1], body)

    def test_prompt_and_settings_regression(self):
        body = json.dumps({'activeId': '', 'items': [], 'glossary': 'OSC\nFBT'})
        self.assertEqual(self.request('POST', body, '/prompts')[0], 204)
        self.assertEqual(self.request(endpoint='/prompts')[1].decode('utf-8'), body)
        self.assertEqual(self.request('POST', '{"activeId":"","items":[1,]}', '/prompts')[0], 400)
        settings = json.loads(self.request(endpoint='/settings')[1])
        self.assertEqual(settings['port'], self.port)
        self.assertFalse(settings['typingOn'])

    def test_restart_persistence(self):
        tag = self.request()[2]['ETag']
        self.stop()
        self.start()
        self.assertEqual(self.request()[1].decode('utf-8'), self.original)
        self.assertEqual(self.request()[2]['ETag'], tag)

    def test_generated_valid_json(self):
        randomizer = random.Random(11)
        for i in range(30):
            value = {'items': [{'id': str(i), 'text': ''.join(randomizer.choice('abc中文😀\n\t"\\') for _ in range(30))}],
                     'extra': [None, False, randomizer.random(), {'value': [i, True]}]}
            body = json.dumps(value, ensure_ascii=bool(i % 2), separators=(',', ':'))
            self.assertEqual(self.request('POST', body)[0], 204)
            self.assertEqual(json.loads(self.request()[1]), value)


if __name__ == '__main__':
    if os.name != 'nt':
        raise SystemExit('Native regression requires Windows.')
    if not EXE.is_file():
        raise SystemExit(f'Build the native EXE first: {EXE}')
    unittest.main(verbosity=2)
