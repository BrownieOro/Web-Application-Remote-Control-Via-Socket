const WebSocket = require('ws');
const net = require('net');
const http = require('http');
const fs = require('fs');
const path = require('path');

// ===== HTTP SERVER =====
const httpServer = http.createServer((req, res) => {
    if (req.url === '/' && req.method === 'GET') {
        // Serve Web.html
        const filePath = path.join(__dirname, 'Web.html');
        fs.readFile(filePath, 'utf8', (err, data) => {
            if (err) {
                res.writeHead(404);
                res.end('Web.html not found');
                return;
            }
            res.writeHead(200, { 'Content-Type': 'text/html' });
            res.end(data);
        });
    } else if (req.url.startsWith('/stream/')) {
        // Handle stream requests
        res.writeHead(200, { 'Content-Type': 'image/jpeg' });
        res.end('Stream placeholder');
    } else {
        res.writeHead(404);
        res.end('Not Found');
    }
});

httpServer.listen(8080, () => {
    console.log('[SERVER] HTTP + WebSocket listening on http://localhost:8080');
});

// ===== WebSocket SERVER =====
const wss = new WebSocket.Server({ server: httpServer });

wss.on('connection', ws => {
    console.log('[WS] Browser connected');

    let tcpClient = null;
    let serverIP = null;

    ws.on('message', msg => {
        const strMsg = msg.toString().trim();
        console.log('[WS] Received:', strMsg);

        // ===== SET_IP: Kết nối tới C++ server =====
        if (strMsg.startsWith('SET_IP ')) {
            serverIP = strMsg.substring(7).trim();
            
            // Close previous connection if exists
            if (tcpClient) {
                tcpClient.destroy();
            }

            tcpClient = new net.Socket();
            tcpClient.connect(9001, serverIP, () => {
                console.log('[TCP] Connected to', serverIP, ':9001');
                ws.send('✓ Connected to server\n');
            });

            // Nhận data từ C++ server, relay về WebSocket
            tcpClient.on('data', data => {
                const response = data.toString();
                console.log('[TCP] Response:', response);
                ws.send(response);
            });

            tcpClient.on('close', () => {
                console.log('[TCP] Disconnected');
                ws.send('✗ Server disconnected\n');
                tcpClient = null;
            });

            tcpClient.on('error', err => {
                console.error('[TCP] Error:', err.message);
                ws.send('✗ Error: ' + err.message + '\n');
                tcpClient = null;
            });
        }
        // ===== MODULE COMMANDS: Relay tới C++ server =====
        else if (strMsg) {
            if (tcpClient && tcpClient.writable) {
                console.log('[RELAY] Sending:', strMsg);
                tcpClient.write(strMsg + '\n');
            } else {
                ws.send('✗ Not connected to server. Please connect first.\n');
            }
        }
    });

    ws.on('close', () => {
        console.log('[WS] Browser disconnected');
        if (tcpClient) {
            tcpClient.destroy();
            tcpClient = null;
        }
    });

    ws.on('error', (err) => {
        console.error('[WS] Error:', err.message);
        if (tcpClient) {
            tcpClient.destroy();
            tcpClient = null;
        }
    });
});
