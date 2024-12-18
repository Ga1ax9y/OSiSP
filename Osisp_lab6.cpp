#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

void PrintRegistryKeys(HKEY hKey, const std::wstring& subKey) {
    HKEY hSubKey;
    /*открыть ключ (открытый разде реестра,имя открываемого подраздела, 
    !0 - символьная ссылка,права доступа,указ на перемен получ деск открытого ключа*/
    if (RegOpenKeyExW(hKey, subKey.c_str(), 0, KEY_READ, &hSubKey) != ERROR_SUCCESS)  { 
        std::wcerr << L"Failed to open key: " << subKey << std::endl;
        return;
    }

    DWORD index = 0;
    wchar_t keyName[256];
    DWORD keyNameSize;

    std::wcout << L"Subkeys under: " << subKey << L"\n";

    while (true) {
        keyNameSize = sizeof(keyName) / sizeof(wchar_t);
        /*(открытый разде реестра,индекс раздела,указа на буфер, размер буфера, 
        всегда 0, класс подраздела, структура для времени записи
        */
        if (RegEnumKeyExW(hSubKey, index, keyName, &keyNameSize, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) { //вывод подключей
            break;
        }

        std::wcout << L"Key: " << subKey + L"\\" + keyName << std::endl;
        index++;
    }

    RegCloseKey(hSubKey);
}

void PrintRegistryValues(HKEY hKey, const std::wstring& subKey) {
    HKEY hSubKey;
    if (RegOpenKeyExW(hKey, subKey.c_str(), 0, KEY_READ, &hSubKey) != ERROR_SUCCESS) {
        std::wcerr << L"Failed to open key: " << subKey << std::endl;
        return;
    }

    DWORD valueCount = 0;
    DWORD maxValueNameSize = 0;
    DWORD maxDataSize = 0;
    /*дескр открытого ключа, указ на буфер, указ на размер буфера, всегда 0, количество подключей,
    самый длиный подключ, самая длинная строка, самый длиный компонент, размер дескриптора безопасн,FILETIME*/
    if (RegQueryInfoKeyW(hSubKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &valueCount, &maxValueNameSize, &maxDataSize, nullptr, nullptr) != ERROR_SUCCESS) {//вывод значений
        RegCloseKey(hSubKey);
        std::wcerr << L"Failed to query key info: " << subKey << std::endl;
        return;
    }

    std::wcout << L"Found " << valueCount << L" values in key: " << subKey << std::endl;

    for (DWORD i = 0; i < valueCount; i++) {
        std::vector<wchar_t> valueName(maxValueNameSize + 1);
        DWORD valueNameSize = maxValueNameSize + 1;
        DWORD dataSize = 0;
        DWORD valueType = 0;

        if (RegEnumValueW(hSubKey, i, valueName.data(), &valueNameSize, nullptr, &valueType, nullptr, &dataSize) == ERROR_SUCCESS)  { 
            std::wstring valueNameStr(valueName.data(), valueNameSize);
            std::wcout << L"\"" << valueNameStr << L"\"=";

            wchar_t* ptr = nullptr;

            std::vector<BYTE> data(dataSize);
            if (RegQueryValueExW(hSubKey, valueNameStr.c_str(), nullptr, nullptr, data.data(), &dataSize) == ERROR_SUCCESS) { //инфо о ключе
                switch (valueType) {
                case REG_SZ:
                case REG_EXPAND_SZ:
                    std::wcout << L"\"" << reinterpret_cast<wchar_t*>(data.data()) << L"\"" << std::endl;
                    break;
                case REG_DWORD: 
                    std::wcout << L"\"" << *reinterpret_cast<DWORD*>(data.data()) << L"\"" << std::endl;
                    break;
                case REG_QWORD:
                    std::wcout << L"\"" << *reinterpret_cast<UINT64*>(data.data()) << L"\"" << std::endl;
                    break;
                case REG_BINARY:
                    std::wcout << L"\"";
                    for (DWORD j = 0; j < dataSize; j++) {
                        std::wcout << std::hex << std::setw(2) << std::setfill(L'0') << (int)data[j] << L" ";
                    }
                    std::wcout << std::dec << L"\"" << std::endl;
                    break;
                case REG_MULTI_SZ:
                    std::wcout << L"\"";
                    ptr = reinterpret_cast<wchar_t*>(data.data());
                    while (*ptr) {
                        std::wcout << ptr << L" ";
                        ptr += wcslen(ptr) + 1;
                    }
                    std::wcout << L"\"" << std::endl;
                    break;
                default:
                    std::wcout << L"Unknown data type" << std::endl;
                    break;
                }
            }
        }
    }

    RegCloseKey(hSubKey);
}
/* HKEY_CURRENT_USER\SOFTWARE\Discord\Modules\discord_aegis */

int main() {
    while (true) {
        std::wcout << L"Commands:\n";
        std::wcout << L"1. Show all keys\n";
        std::wcout << L"2. Show all values\n";
        std::wcout << L"3. Exit\n";
        std::wcout << L"Your choice: ";

        int choice;
        std::wcin >> choice;

        if (choice == 3) {
            std::wcout << L"Exiting..." << std::endl;
            break;
        }

        std::wcin.ignore();
        std::wcout << L"Enter the path of the key: ";
        std::wstring regPath;
        std::getline(std::wcin, regPath);

        HKEY rootKey;
        if (regPath.find(L"HKEY_LOCAL_MACHINE") == 0) {
            rootKey = HKEY_LOCAL_MACHINE;
        }
        else if (regPath.find(L"HKEY_USERS") == 0) {
            rootKey = HKEY_USERS;
        }
        else if (regPath.find(L"HKEY_CLASSES_ROOT") == 0) {
            rootKey = HKEY_CLASSES_ROOT;
        }
        else if (regPath.find(L"HKEY_CURRENT_USER") == 0) {
            rootKey = HKEY_CURRENT_USER;
        }
        else if (regPath.find(L"HKEY_CURRENT_CONFIG") == 0) {
            rootKey = HKEY_CURRENT_CONFIG;
        }
        else {
            std::wcerr << L"Invalid root key. Valid options are: HKEY_LOCAL_MACHINE, HKEY_USERS, HKEY_CLASSES_ROOT, HKEY_CURRENT_USER, HKEY_CURRENT_CONFIG." << std::endl;
            continue;
        }

        size_t pos = regPath.find(L"\\");

        std::wstring subKey = (pos != std::wstring::npos) ? regPath.substr(pos + 1) : L"";

        // Выбор действия
        if (choice == 1) {
            PrintRegistryKeys(rootKey, subKey);
        }
        else if (choice == 2) {
            PrintRegistryValues(rootKey, subKey); 
        }
        else {
            std::wcerr << L"Invalid choice. Please try again." << std::endl;
        }
    }

    return 0;
}
