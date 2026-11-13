"""Loopback-only, read-only CYD backup service with observable progress."""
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile
import threading
import time
import uuid
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / 'installer-web'
PORT = 8000
FLASH_SIZE = 0x400000
PORT_NAME = os.environ.get('CYD_PORT', 'COM5')
if not re.fullmatch(r'COM[1-9][0-9]{0,2}', PORT_NAME, re.I):
    raise SystemExit('CYD_PORT must be a Windows COM port, e.g. COM5')

lock = threading.Lock()
job = {'id': None, 'state': 'idle', 'percent': 0, 'message': 'Ready', 'sha256': None, 'data': None, 'started': 0}


def run_backup(job_id):
    try:
        with tempfile.TemporaryDirectory(prefix='cyd-private-') as tmp:
            target = Path(tmp) / 'full-flash.bin'
            cmd = [sys.executable, '-m', 'esptool', '--chip', 'esp32', '--port', PORT_NAME,
                   '--baud', '115200', 'read-flash', '0x0', hex(FLASH_SIZE), str(target)]
            # Esptool progress may be printed using carriage returns, so parse both CR and LF.
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    bufsize=0)
            tail = bytearray()
            current = bytearray()
            deadline = time.monotonic() + 1200
            try:
                while True:
                    if time.monotonic() > deadline:
                        proc.kill()
                        raise RuntimeError('Timed out after 20 minutes')
                    ch = proc.stdout.read(1)
                    if not ch:
                        break
                    if ch in (b'\r', b'\n'):
                        line = current.decode('utf-8', 'replace').strip()
                        current.clear()
                        if line:
                            tail.extend((line + '\n').encode('utf-8'))
                            if len(tail) > 6000:
                                del tail[:-4000]
                            # Do not infer progress from file size: esptool may preallocate it.
                            match = re.search(r'\((\d{1,3})\s*%\)', line)
                            if match:
                                pct = min(99, int(match.group(1)))
                                with lock:
                                    if job['id'] == job_id:
                                        job['percent'] = max(job['percent'], pct)
                                        job['message'] = 'Reading flash: ' + str(pct) + '%'
                            elif any(word in line.lower() for word in ('connecting', 'stub flasher', 'configuring flash', 'read ')):
                                with lock:
                                    if job['id'] == job_id:
                                        job['message'] = line[:180]
                    else:
                        current.extend(ch)
                        if len(current) > 4000:
                            del current[:-2000]
                rc = proc.wait(timeout=10)
            finally:
                if proc.poll() is None:
                    proc.kill()
                    proc.wait()
            if rc != 0:
                raise RuntimeError('esptool read failed: ' + tail.decode('utf-8', 'replace')[-500:])
            data = target.read_bytes()
            if len(data) != FLASH_SIZE:
                raise RuntimeError(f'Incomplete flash readback ({len(data)} bytes)')
            digest = hashlib.sha256(data).hexdigest()
            with lock:
                if job['id'] == job_id:
                    job.update(state='done', percent=100, message='Backup ready for download', sha256=digest, data=data)
    except Exception as exc:
        with lock:
            if job['id'] == job_id:
                job.update(state='error', message=str(exc)[:500], data=None)


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(WEB), **kwargs)

    def json_reply(self, code, payload):
        data = json.dumps(payload).encode('utf-8')
        self.send_response(code)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Cache-Control', 'no-store')
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.end_headers()
        self.wfile.write(data)

    def valid_origin(self):
        return self.headers.get('Origin') in (None, 'http://localhost:8000', 'http://127.0.0.1:8000')

    def do_POST(self):
        if urlparse(self.path).path != '/local-api/backup/start':
            self.send_error(404); return
        if not self.valid_origin():
            self.send_error(403); return
        if self.headers.get('Content-Length') not in ('0', None):
            self.send_error(400); return
        with lock:
            if job['state'] == 'running':
                self.json_reply(409, {'error': 'Backup already in progress'}); return
            job_id = uuid.uuid4().hex
            job.update(id=job_id, state='running', percent=0,
                       message='Connecting to ESP32...', sha256=None, data=None,
                       started=time.monotonic())
        threading.Thread(target=run_backup, args=(job_id,), daemon=True).start()
        self.json_reply(202, {'id': job_id})

    def do_GET(self):
        path = urlparse(self.path).path
        if path.startswith('/local-api/'):
            if not self.valid_origin():
                self.send_error(403); return
            with lock:
                state = {k: v for k, v in job.items() if k not in ('data', 'started')}
                elapsed = round(time.monotonic() - job['started']) if job['started'] else 0
                data = job['data']
            if path == '/local-api/backup/status':
                state['elapsed_seconds'] = elapsed
                self.json_reply(200, state)
                return
            if path == '/local-api/backup/download':
                if state['state'] != 'done' or data is None:
                    self.json_reply(409, {'error': 'Backup is not ready'}); return
                self.send_response(200)
                self.send_header('Content-Type', 'application/octet-stream')
                self.send_header('Content-Disposition', 'attachment; filename="cyd-full-backup.bin"')
                self.send_header('Content-Length', str(len(data)))
                self.send_header('X-Backup-SHA256', state['sha256'])
                self.send_header('Cache-Control', 'no-store')
                self.send_header('X-Content-Type-Options', 'nosniff')
                self.end_headers()
                self.wfile.write(data)
                return
            self.send_error(404); return
        super().do_GET()


if __name__ == '__main__':
    server = ThreadingHTTPServer(('127.0.0.1', PORT), Handler)
    print(f'Local-only CYD backup server: http://localhost:{PORT}/local-backup/ (port {PORT_NAME})', flush=True)
    server.serve_forever()
