#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>

#define PIPE_NAME L"\\\\.\\pipe\\TestPipe"
#define BUFFER_SIZE 512

std::mutex logMutex;
std::atomic<bool> isRunning(true);

void LogMessage(const std::string& message) {
    std::lock_guard<std::mutex> guard(logMutex);
    std::ofstream logFile("log.txt", std::ios::app);

    if (logFile.is_open()) {
        time_t now = time(0);
        tm localtm;
        localtime_s(&localtm, &now);

        char timebuff[80];
        strftime(timebuff, sizeof(timebuff), "%Y-%m-%d %H:%M:%S", &localtm);

        logFile << "[" << timebuff << "] " << message << std::endl;
        logFile.close();
    }
}

void HandleClient(HANDLE hpipe) {
    char buff[BUFFER_SIZE];
    DWORD bytesRead;

    while (isRunning.load() && ReadFile(hpipe, buff, sizeof(buff) - 1, &bytesRead, NULL)) {
        buff[bytesRead] = '\0';
        std::string message(buff);

        if (message == "STOP SERVER -F") {
            std::cout << "Shutdown command received from client. Stopping server." << std::endl;
            isRunning.store(false); 
            break;
        }

        LogMessage("Client ID: " + std::to_string(GetCurrentThreadId()) + " - Message: " + message);
    }

    DisconnectNamedPipe(hpipe);
    CloseHandle(hpipe);
    std::cout << "Client disconnected." << std::endl;
}

int main() {
    std::vector<std::thread> clientThreads;

    while (isRunning.load()) {
        HANDLE hpipe = CreateNamedPipe(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            BUFFER_SIZE,
            BUFFER_SIZE,
            0,
            NULL
        );

        if (hpipe == INVALID_HANDLE_VALUE) {
            std::cerr << "Error creating pipe: " << GetLastError() << std::endl;
            return 1;
        }

        std::cout << "Waiting for client connection..." << std::endl;
        BOOL connected = ConnectNamedPipe(hpipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (connected && isRunning.load()) {
            std::cout << "Client connected." << std::endl;
            clientThreads.emplace_back(HandleClient, hpipe);  
        }
        else {
            CloseHandle(hpipe);
        }
    }

    for (auto& t : clientThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

    std::cout << "Server stopped." << std::endl;
    return 0;
}
