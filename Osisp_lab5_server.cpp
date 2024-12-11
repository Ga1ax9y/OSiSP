#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <map>
#pragma comment(lib, "ws2_32.lib")

const int PORT = 8080;
const int BUFFER_SIZE = 512;

std::string textToMorse(const std::string& text) {
    static const std::map<char, std::string> morseCode = {
        {'A', ".-"}, {'B', "-..."}, {'C', "-.-."}, {'D', "-.."}, {'E', "."},
        {'F', "..-."}, {'G', "--."}, {'H', "...."}, {'I', ".."}, {'J', ".---"},
        {'K', "-.-"}, {'L', ".-.."}, {'M', "--"}, {'N', "-."}, {'O', "---"},
        {'P', ".--."}, {'Q', "--.-"}, {'R', ".-."}, {'S', "..."}, {'T', "-"},
        {'U', "..-"}, {'V', "...-"}, {'W', ".--"}, {'X', "-..-"}, {'Y', "-.--"},
        {'Z', "--.."}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"},
        {'4', "....-"}, {'5', "....."}, {'6', "-...."}, {'7', "--..."},
        {'8', "---.."}, {'9', "----."}, {'0', "-----"}
    };

    std::string result;
    for (char c : text) {
        c = toupper(c);
        if (morseCode.find(c) != morseCode.end()) {
            result += morseCode.at(c) + " ";
        }
        else if (c == ' ') {
            result += "/ ";
        }
    }
    return result;
}

int main() {
    WSADATA wsaData;
    SOCKET SSocket, CSocket;
    sockaddr_in SAddr, CAddr;
    int CAddrSize = sizeof(CAddr);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error: WSAStartup failed\n";
        return 1;
    }

    SSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (SSocket == INVALID_SOCKET) {
        std::cerr << "Error: Invalid socket\n";
        WSACleanup();
        return 1;
    }

    SAddr.sin_addr.s_addr = INADDR_ANY;
    SAddr.sin_port = htons(PORT);
    SAddr.sin_family = AF_INET;

    if (bind(SSocket, (sockaddr*)&SAddr, sizeof(SAddr)) == SOCKET_ERROR) {
        std::cerr << "Error: Problem with bind\n";
        closesocket(SSocket);
        WSACleanup();
        return 1;
    }

    if (listen(SSocket, 5) == SOCKET_ERROR) {
        std::cerr << "Error: Problem with socktet\n";
        closesocket(SSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server works on port: " << PORT << "...\n";

    while (true) {
        CSocket = accept(SSocket, (sockaddr*)&CAddr, &CAddrSize);
        if (CSocket == INVALID_SOCKET) {
            std::cerr << "Error: Accept failed\n";
            continue;
        }

        std::cout << "Client connected\n";

        while (true) {
            char buff[BUFFER_SIZE] = { 0 };
            int bytesReceived = recv(CSocket, buff, BUFFER_SIZE, 0);
            if (bytesReceived <= 0) {
                std::cout << "Client disconnected\n";
                break;
            }

            std::string request(buff);
            if (request == "exit") {
                std::cout << "Client disconnected\n";
                break;
            }

            std::string response = textToMorse(request);
            send(CSocket, response.c_str(), response.length(), 0);
        }

        closesocket(CSocket);
    }

    closesocket(SSocket);
    WSACleanup();
    return 0;
}
