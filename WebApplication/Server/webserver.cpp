#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

#define PORT_HTTP 8080
#define PORT_SOCKET 9001

std::mutex clientsMutex;
std::vector<SOCKET> streamClients;
SOCKET tcpServerSocket = INVALID_SOCKET;

// ===== HÀM: Gửi HTTP response =====
void sendHttpResponse(SOCKET client, const char* body, const char* contentType = "text/html") {
    char response[4096];
    sprintf_s(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: keep-alive\r\n"
        "\r\n"
        "%s", contentType, (int)strlen(body), body);
    
    send(client, response, strlen(response), 0);
}

// ===== HÀM: Gửi MJPEG frame =====
void broadcastStreamFrame(const unsigned char* frameData, int frameSize) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    for (auto it = streamClients.begin(); it != streamClients.end(); ) {
        char frameHeader[256];
        sprintf_s(frameHeader, sizeof(frameHeader),
            "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n", frameSize);
        
        int sent = send(*it, frameHeader, strlen(frameHeader), 0);
        if (sent == SOCKET_ERROR) {
            closesocket(*it);
            it = streamClients.erase(it);
            continue;
        }
        
        send(*it, (const char*)frameData, frameSize, 0);
        send(*it, "\r\n", 2, 0);
        ++it;
    }
}

// ===== HÀM: Xử lý HTTP request =====
void handleHttpRequest(SOCKET client, const char* request) {
    if (strstr(request, "GET /stream/webcam") != NULL) {
        // MJPEG stream endpoint
        const char* headers =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
            "Cache-Control: no-cache, no-store, must-revalidate\r\n"
            "Connection: keep-alive\r\n"
            "\r\n";
        
        send(client, headers, strlen(headers), 0);
        
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            streamClients.push_back(client);
        }
        printf("Stream client connected, total: %d\n", (int)streamClients.size());
    }
    else if (strstr(request, "GET /") != NULL) {
        // Home page
        const char* body =
            "<h1>WebSocket Bridge Server Running</h1>"
            "<p>WebSocket: ws://localhost:8080</p>"
            "<p>Stream: http://localhost:8080/stream/webcam</p>";
        sendHttpResponse(client, body);
        closesocket(client);
    }
    else {
        // 404
        const char* notFound = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
        send(client, notFound, strlen(notFound), 0);
        closesocket(client);
    }
}

// ===== HÀM: HTTP client handler =====
void handleHttpClient(SOCKET client) {
    char buffer[4096];
    int recvLen = recv(client, buffer, sizeof(buffer) - 1, 0);
    
    if (recvLen > 0) {
        buffer[recvLen] = '\0';
        handleHttpRequest(client, buffer);
    }
}

// ===== HÀM: TCP relay server (relay WebSocket ↔ C++ socket) =====
void handleTcpRelay(SOCKET client) {
    char buffer[8192];
    
    while (true) {
        int recvLen = recv(client, buffer, sizeof(buffer), 0);
        
        if (recvLen <= 0) {
            closesocket(client);
            break;
        }
        
        buffer[recvLen] = '\0';
        printf("TCP relay received: %s\n", buffer);
        
        // Nếu là stream frame (binary data lớn hơn 1000 bytes)
        if (recvLen > 1000) {
            broadcastStreamFrame((unsigned char*)buffer, recvLen);
        }
    }
}

// ===== MAIN: HTTP Server =====
void runHttpServer() {
    WSADATA wsaData;
    SOCKET listenSocket = INVALID_SOCKET;
    
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return;
    }
    
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        printf("socket failed\n");
        WSACleanup();
        return;
    }
    
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    serverAddr.sin_port = htons(PORT_HTTP);
    
    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("bind failed\n");
        closesocket(listenSocket);
        WSACleanup();
        return;
    }
    
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed\n");
        closesocket(listenSocket);
        WSACleanup();
        return;
    }
    
    printf("HTTP server listening on port %d\n", PORT_HTTP);
    
    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            printf("accept failed\n");
            continue;
        }
        
        printf("HTTP client connected\n");
        
        // Xử lý client trong thread riêng
        std::thread([clientSocket]() {
            handleHttpClient(clientSocket);
        }).detach();
    }
    
    closesocket(listenSocket);
    WSACleanup();
}

// ===== MAIN =====
int main() {
    printf("Web Bridge Server Starting...\n");
    printf("HTTP Server on port %d\n", PORT_HTTP);
    
    // Chạy HTTP server
    std::thread httpThread(runHttpServer);
    httpThread.detach();
    
    // Chờ vô tận
    while (true) {
        Sleep(1000);
    }
    
    return 0;
}
