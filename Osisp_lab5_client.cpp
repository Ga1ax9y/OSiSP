#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

const int PORT = 8080;
const int BUFFER_SIZE = 512;

int main() {
    WSADATA wsaData;
    SOCKET CSocket;
    sockaddr_in SAddr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error: problem with WinSock\n";
        return 1;
    }

    CSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (CSocket == INVALID_SOCKET) {
        std::cerr << "Error: Problem with socket creation\n";
        WSACleanup();
        return 1;
    }

    SAddr.sin_family = AF_INET;
    SAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &SAddr.sin_addr);

    if (connect(CSocket, (sockaddr*)&SAddr, sizeof(SAddr)) == SOCKET_ERROR) {
        std::cerr << "Error: Connection failed\n";
        closesocket(CSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to the server\n";

    while (true) {
        std::cout << "\nCommands:\n";
        std::cout << "1. Send a message to the server\n";
        std::cout << "2. Exit\n";
        std::cout << "Your choice: ";

        int choice;
        std::cin >> choice;
        std::cin.ignore();

        if (choice == 2) {
            std::string exitMessage = "exit";
            send(CSocket, exitMessage.c_str(), exitMessage.length(), 0);
            std::cout << "Disconnected...\n";
            break;
        }
        else if (choice == 1) {
            std::cout << "Your message: ";
            std::string message;

            std::getline(std::cin, message);
            if (message.length() == 0) {
                std::cout << "Error";
            }
            send(CSocket, message.c_str(), message.length(), 0);

            char buff[BUFFER_SIZE] = { 0 };
            int bytes = recv(CSocket, buff, BUFFER_SIZE, 0);
            if (bytes > 0) {
                std::cout << "Response from the server: " << buff << "\n";
            }
            else {
                std::cout << "Error: problem with receiving response from server\n";
            }
        }
        else {
            std::cout << "Error: Incorreact choice. Try again.\n";
        }
    }

    closesocket(CSocket);
    WSACleanup();
    return 0;
}
