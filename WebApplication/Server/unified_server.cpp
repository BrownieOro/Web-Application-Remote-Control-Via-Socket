#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <iostream>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

// Include module headers
#include "process_list_start_stop.h"
#include "screenshot.h"
#include "webcam.h"
#include "shutdown.h"
#include "keylogger.h"
#include "app_list_start_stop.h"

#define SERVER_PORT 9001
#define BRIDGE_PORT 8080

SOCKET tcpToServer = INVALID_SOCKET;
bool isConnected = false;

// ===== HÀM: Handle command từ client module =====
std::string handleCommand(const std::string& cmd) {
    printf("[SERVER] Received command: %s\n", cmd.c_str());
    
    // ===== PROCESS COMMANDS =====
    //if (cmd == "process_list") return listProcesses();
    //if (cmd.find("start_process ") == 0) {
    //    std::string proc = cmd.substr(13);
    //    return startProcess(proc);
    //}
    //if (cmd.find("stop_process ") == 0) {
    //    int pid = std::stoi(cmd.substr(12));
    //    return stopProcess(pid);
    //}
    
    // ===== APP COMMANDS =====
    //if (cmd == "list_apps") return listApps();
    //if (cmd.find("start_app ") == 0) {
    //    std::string app = cmd.substr(10);
    //    return startApp(app);
    //}
    //if (cmd.find("stop_app ") == 0) {
    //    std::string app = cmd.substr(9);
    //    return stopApp(app);
    //}
    
    // ===== SCREENSHOT =====
    //if (cmd == "screenshot") return takeScreenshot();
    //if (cmd == "screen_stream_start") return screenStreamStart();
    //if (cmd == "screen_stream_stop") return screenStreamStop();
    
    // ===== WEBCAM COMMANDS =====
    //if (cmd == "webcam") return webcamStart();
    //if (cmd == "webcam_stop") return webcamStop();
    //if (cmd == "webcam_capture") return webcamCapture();
    //if (cmd == "webcam_record") return webcamRecordVideo(15);
    //if (cmd == "webcam_stream_start") return webcamStreamStart();
    //if (cmd == "webcam_stream_stop") return webcamStreamStop();
    
    // ===== KEYLOGGER =====
    //if (cmd == "keylogger_start") return keyloggerStart();
    //if (cmd == "keylogger_stop") return keyloggerStop();
    //if (cmd == "keylogger_get") return keyloggerGetLogs();
    
    // ===== SHUTDOWN =====
    //if (cmd == "shutdown") {
    //    shutdownMachine();
    //    return "✓ Shutdown initiated";
    //}
    //if (cmd == "restart") {
    //    restartMachine();
    //    return "✓ Restart initiated";
    //}
    
    return "OK\n";
}

// ===== HÀM: Relay từ bridge client ↔ Internal command handler =====
void relayData(SOCKET bridgeClient) {
    char buffer[8192];
    while (isConnected && bridgeClient != INVALID_SOCKET) {
        int recvLen = recv(bridgeClient, buffer, sizeof(buffer), 0);
        if (recvLen <= 0) break;
        buffer[recvLen] = '\0';
        std::string response = handleCommand(buffer);
        send(bridgeClient, response.c_str(), response.length(), 0);
    }
    closesocket(bridgeClient);
    isConnected = false;
}

// ===== HÀM: Handle bridge client (Web.html) =====
void handleBridgeClient(SOCKET bridgeClient) {
    char buffer[1024];
    int recvLen = recv(bridgeClient, buffer, sizeof(buffer) - 1, 0);
    
    if (recvLen <= 0) {
        closesocket(bridgeClient);
        return;
    }
    
    buffer[recvLen] = '\0';
    printf("[BRIDGE] Received: %s\n", buffer);
    
    std::string msg(buffer);
    
    // Nếu là SET_IP command, proceed (không cần kết nối client socket)
    if (msg.find("SET_IP") != std::string::npos) {
        printf("[BRIDGE] SET_IP command, ready to relay commands\n");
        
        isConnected = true;
        
        // Send confirmation
        const char* confirmation = "CONNECTED\n";
        send(bridgeClient, confirmation, strlen(confirmation), 0);
        
        // Relay data
        relayData(bridgeClient);
    } else {
        closesocket(bridgeClient);
    }
}

void runBridgeServer() {
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in bridgeAddr = {0};
    bridgeAddr.sin_family = AF_INET;
    bridgeAddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    bridgeAddr.sin_port = htons(BRIDGE_PORT);
    bind(listenSocket, (sockaddr*)&bridgeAddr, sizeof(bridgeAddr));
    listen(listenSocket, 1);
    printf("[BRIDGE] Listening on port %d\n", BRIDGE_PORT);
    SOCKET bridgeClient = accept(listenSocket, NULL, NULL);
    if (bridgeClient != INVALID_SOCKET) handleBridgeClient(bridgeClient);
    closesocket(listenSocket);
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    printf("Server running on port %d...\n", BRIDGE_PORT);
    runBridgeServer();
    WSACleanup();
    return 0;
}
