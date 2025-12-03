#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#define BRIDGE_PORT 8080
#define SERVER_PORT 9001

SOCKET tcpToServer = INVALID_SOCKET;
SOCKET wsClient = INVALID_SOCKET;
bool isConnected = false;

// ===== HÀM: Gửi HTTP response =====
void sendHttpResponse(SOCKET client, const char* body) {
    char response[2048];
    sprintf_s(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n%s", (int)strlen(body), body);
    send(client, response, strlen(response), 0);
}

// ===== HÀM: Xử lý HTTP client =====
void handleHttpClient(SOCKET client) {
    char buffer[4096];
    int recvLen = recv(client, buffer, sizeof(buffer) - 1, 0);
    
    if (recvLen > 0) {
        buffer[recvLen] = '\0';
        
        if (strstr(buffer, "GET /") != NULL) {
            const char* body = "<h1>Bridge Server Running</h1><p>Port: 8080</p>";
            sendHttpResponse(client, body);
        }
    }
    
    closesocket(client);
}

// ===== HÀM: Relay từ WebSocket client ↔ Server =====
void relayData() {
    char buffer[8192];
    
    while (isConnected && tcpToServer != INVALID_SOCKET && wsClient != INVALID_SOCKET) {
        // Nhận từ WebSocket client
        int recvLen = recv(wsClient, buffer, sizeof(buffer), 0);
        
        if (recvLen <= 0) {
            printf("[RELAY] Client disconnected\n");
            break;
        }
        
        buffer[recvLen] = '\0';
        printf("[RELAY] Received from client: %s\n", buffer);
        
        // Gửi tới server
        if (send(tcpToServer, buffer, recvLen, 0) == SOCKET_ERROR) {
            printf("[RELAY] Failed to send to server\n");
            break;
        }
        
        // Nhận response từ server
        int responseLen = recv(tcpToServer, buffer, sizeof(buffer), 0);
        if (responseLen > 0) {
            // Gửi lại client
            if (send(wsClient, buffer, responseLen, 0) == SOCKET_ERROR) {
                printf("[RELAY] Failed to send response to client\n");
                break;
            }
            printf("[RELAY] Sent response (%d bytes) to client\n", responseLen);
        }
    }
    
    closesocket(wsClient);
    closesocket(tcpToServer);
    wsClient = INVALID_SOCKET;
    tcpToServer = INVALID_SOCKET;
    isConnected = false;
}

// ===== HÀM: Xử lý simple TCP protocol (giả WebSocket) =====
void handleWebSocketClient(SOCKET client) {
    char buffer[1024];
    int recvLen = recv(client, buffer, sizeof(buffer) - 1, 0);
    
    if (recvLen <= 0) {
        closesocket(client);
        return;
    }
    
    buffer[recvLen] = '\0';
    printf("[WS] Received: %s\n", buffer);
    
    // Parse message
    std::string msg(buffer);
    
    // Nếu là SET_IP command, kết nối tới server
    if (msg.find("SET_IP") != std::string::npos) {
        // Extract IP từ message (định dạng: "SET_IP <ip>")
        size_t pos = msg.find(" ");
        std::string serverIP = (pos != std::string::npos) ? msg.substr(pos + 1) : "127.0.0.1";
        
        // Remove newline/whitespace
        while (!serverIP.empty() && (serverIP.back() == '\n' || serverIP.back() == '\r' || serverIP.back() == ' ')) {
            serverIP.pop_back();
        }
        
        printf("[BRIDGE] Connecting to server at %s:%d\n", serverIP.c_str(), SERVER_PORT);
        
        tcpToServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (tcpToServer == INVALID_SOCKET) {
            printf("[BRIDGE] Socket creation failed\n");
            closesocket(client);
            return;
        }
        
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(SERVER_PORT);
        serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
        
        if (connect(tcpToServer, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            printf("[BRIDGE] Connection to server failed\n");
            closesocket(tcpToServer);
            tcpToServer = INVALID_SOCKET;
            closesocket(client);
            return;
        }
        
        printf("[BRIDGE] Connected to server successfully\n");
        isConnected = true;
        wsClient = client;
        
        // Send confirmation
        const char* confirmation = "CONNECTED\n";
        send(wsClient, confirmation, strlen(confirmation), 0);
        
        // Relay data
        relayData();
    } else {
        closesocket(client);
    }
}

// ===== MAIN =====
int main() {
    WSADATA wsaData;
    SOCKET listenSocket = INVALID_SOCKET;
    
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
    
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }
    
    sockaddr_in bridgeAddr;
    bridgeAddr.sin_family = AF_INET;
    bridgeAddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    bridgeAddr.sin_port = htons(BRIDGE_PORT);
    
    if (bind(listenSocket, (sockaddr*)&bridgeAddr, sizeof(bridgeAddr)) == SOCKET_ERROR) {
        printf("Bind failed\n");
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    
    if (listen(listenSocket, 1) == SOCKET_ERROR) {
        printf("Listen failed\n");
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    
    printf("[BRIDGE] Bridge server listening on port %d\n", BRIDGE_PORT);
    printf("[BRIDGE] Waiting for client...\n");
    
    // Accept 1 client
    SOCKET clientSocket = accept(listenSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        printf("Accept failed\n");
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    
    printf("[BRIDGE] Client connected\n");
    
    // Handle client
    handleWebSocketClient(clientSocket);
    
    printf("[BRIDGE] Client disconnected, shutting down\n");
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}
