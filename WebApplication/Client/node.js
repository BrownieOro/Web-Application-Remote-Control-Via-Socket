const WebSocket = require('ws');
const net = require('net');
const http = require('http');

// ===== HTTP SERVER =====
const httpServer = http.createServer((req, res) => {
    if (req.url === '/' && req.method === 'GET') {
        res.writeHead(200, { 'Content-Type': 'text/html' });
        res.end('<h1>WebSocket Bridge Server</h1><p>ws://localhost:8080</p>');
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

    ws.on('message', msg => {
        const strMsg = msg.toString();
        console.log('[WS] Received:', strMsg);

        // ===== SET_IP: Kết nối tới C++ server =====
        if (strMsg.startsWith('SET_IP ')) {
            const serverIP = strMsg.substring(7);
            tcpClient = new net.Socket();
            tcpClient.connect(9001, serverIP, () => {
                console.log('[TCP] Connected to', serverIP, ':9001');
                ws.send('Connected to server');
            });

            // Nhận data từ C++ server, relay về WebSocket
            tcpClient.on('data', data => {
                ws.send(data.toString());
            });

            tcpClient.on('close', () => {
                console.log('[TCP] Disconnected');
                ws.send('Server disconnected');
            });

            tcpClient.on('error', err => {
                console.error('[TCP] Error:', err.message);
                ws.send('Error: ' + err.message);
            });
        }
        // ===== LỆNH KHÁC: Relay tới C++ server =====
        else {
            if (tcpClient) {
                console.log('[RELAY] Sending:', strMsg);
                tcpClient.write(strMsg + '\n');
            } else {
                ws.send('Not connected to server');
            }
        }
    });

    ws.on('close', () => {
        console.log('[WS] Browser disconnected');
        if (tcpClient) tcpClient.destroy();
    });
});
