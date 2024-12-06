#include <windows.h>
#include <iostream>
#include <string>

#define PIPE_NAME L"\\\\.\\pipe\\TestPipe"

int main() {
    HANDLE hpipe;
    std::string message;

    hpipe = CreateFile(
        PIPE_NAME,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hpipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Error connecting to server: " << GetLastError() << std::endl;
        return 1;
    }

    while (true) {
        std::cout << "Enter message to send to server: ";
        std::getline(std::cin, message);

        DWORD bytesWritten;
        BOOL success = WriteFile(
            hpipe,
            message.c_str(),
            message.length(),
            &bytesWritten,
            NULL
        );

        if (!success) {
            std::cerr << "Error writing to pipe: " << GetLastError() << std::endl;
            break;
        }

        if (message == "STOP SERVER -F") {
            std::cout << "Shutdown command sent. Exiting client." << std::endl;
            break;
        }
    }

    CloseHandle(hpipe);
    return 0;
}
