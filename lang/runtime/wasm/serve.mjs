// Minimal static file server for the wasm demo/tests (no dependencies).
// Usage: node serve.mjs <dir> <port>
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';

const root = path.resolve(process.argv[2] ?? '.');
const port = Number(process.argv[3] ?? 4199);

const types = {
    '.html': 'text/html; charset=utf-8',
    '.mjs': 'text/javascript; charset=utf-8',
    '.js': 'text/javascript; charset=utf-8',
    '.wasm': 'application/wasm',
    '.css': 'text/css; charset=utf-8',
    '.json': 'application/json; charset=utf-8',
};

http.createServer((req, res) => {
    const url = new URL(req.url ?? '/', `http://${req.headers.host ?? 'localhost'}`);
    let file = path.join(root, decodeURIComponent(url.pathname));
    if (url.pathname === '/' || url.pathname.endsWith('/')) file = path.join(file, 'index.html');
    if (!file.startsWith(root)) {
        res.writeHead(403);
        res.end('forbidden');
        return;
    }
    fs.readFile(file, (err, data) => {
        if (err) {
            res.writeHead(404);
            res.end('not found');
            return;
        }
        res.writeHead(200, { 'Content-Type': types[path.extname(file)] ?? 'application/octet-stream' });
        res.end(data);
    });
}).listen(port, '127.0.0.1', () => {
    console.log(`wasm demo serving ${root} at http://127.0.0.1:${port}`);
});
