#include <regex>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <thread>
#include <chrono>
#include <codecvt>

struct Task {
    int repeatInterval; // Интервал повторения
    std::string time; // Время
    std::wstring exePath; // Путь к исполняемому файлу (широкая строка)
    std::wstring params; // Параметры запуска (широкая строка)
    int lastExecutionDay; // День последнего выполнения
};

bool isValidRepeatInterval(int interval) {
    return interval >= 1 && interval <= 365;
}

bool isValidTime(const std::string& time) {
    std::regex timePattern(R"(\d{2}:\d{2})");
    if (!std::regex_match(time, timePattern)) {
        return false;
    }

    int hour, minute;
    char colon;
    std::istringstream iss(time);
    iss >> hour >> colon >> minute;

    return colon == ':' && hour >= 0 && hour < 24 && minute >= 0 && minute < 60;
}

bool isValidExePath(const std::string& path) {
    return path.size() >= 4 && path.substr(path.size() - 4) == ".exe";
}

// Функция для запуска процесса
void runProcess(const std::wstring& exePath, const std::wstring& params) {
    STARTUPINFO si = {}; // для задания характеристик нового процесса
    PROCESS_INFORMATION pi = {}; //будет содержать информацию о запущенном процессе, включая дескрипторы процесса и потока, а также идентификаторы процесса и потока
    si.cb = sizeof(si);

    std::wstring command = exePath + L" " + params;

    if (CreateProcess(
        exePath.c_str(),
        const_cast<LPWSTR>(command.c_str()),
        nullptr, //указатель на атрибут безопасности процесса
        nullptr, //указатель на аттрибут безопасности потока
        FALSE,  //процесс не будет унаследован текущим процессом
        0, //процесс создается с обычным приоритетом
        nullptr, // рабочая директория по умолчанию
        nullptr, // окружение по умолчанию
        &si, 
        &pi)) {
        std::wcout << L"Запущен процесс: " << exePath << std::endl;
        CloseHandle(pi.hProcess); //закрытие дескрипторов процесса
        CloseHandle(pi.hThread); //закрытие дескриптора потока
    }
    else {
        std::wcerr << L"Ошибка запуска: " << GetLastError() << std::endl;
    }
}

std::vector<Task> readConfig(const std::string& filename) {
    std::vector<Task> tasks;
    std::ifstream file(filename);
    std::string line;
    int lineNum = 0;

    while (std::getline(file, line)) {
        lineNum++;
        std::istringstream iss(line);
        Task task;
        std::string exePath, params;


        if (iss >> task.repeatInterval >> task.time >> exePath) {
            std::getline(iss, params);


            if (!isValidRepeatInterval(task.repeatInterval)) {
                std::cerr << "Ошибка в интервале повторения на строке " << lineNum << ": " << task.repeatInterval << std::endl;
                continue; 
            }

            if (!isValidTime(task.time)) {
                std::cerr << "Ошибка в формате времени на строке " << lineNum << ": " << task.time << std::endl;
                continue; 
            }

            if (!isValidExePath(exePath)) {
                std::cerr << "Неправильный путь к файлу на строке " << lineNum << ": " << exePath << std::endl;
                continue; 
            }

            task.exePath = std::wstring(exePath.begin(), exePath.end());
            task.params = std::wstring(params.begin(), params.end());
            task.lastExecutionDay = -task.repeatInterval; 
            tasks.push_back(task);
        }
        else {
            std::cerr << "Ошибка разбора строки " << lineNum << std::endl;
        }
    }

    return tasks;
}

std::string getCurrentTime() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << st.wHour << ":"
        << std::setw(2) << std::setfill('0') << st.wMinute;
    return oss.str();
}

int getCurrentDayOfYear() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    int dayOfYear = st.wDay;
    for (int month = 1; month < st.wMonth; month++) {
        dayOfYear += (month == 2 && ((st.wYear % 4 == 0 && st.wYear % 100 != 0) || (st.wYear % 400 == 0))) ? 29 : 31 - (month % 2);
    }
    return dayOfYear;
}

void waitNextMinute() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    int secondsToWait = 60 - st.wSecond; // Сколько секунд до следующей минуты
    std::this_thread::sleep_for(std::chrono::seconds(secondsToWait));
}

std::string wstringToString(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

int main() {
    setlocale(LC_ALL, "ru");
    const std::string configFile = "tasks.txt"; 
    std::vector<Task> tasks = readConfig(configFile);

    while (true) {
        waitNextMinute();

        std::string currentTime = getCurrentTime();
        int currentDayOfYear = getCurrentDayOfYear();

        for (auto& task : tasks) {
            if (currentTime == task.time && (currentDayOfYear - task.lastExecutionDay) >= task.repeatInterval) {
                runProcess(task.exePath, task.params);
                task.lastExecutionDay = currentDayOfYear; 
                std::cout << "Процесс " << wstringToString(task.exePath)
                    << " запущен в " << task.time << std::endl;
            }
        }
    }

    return 0;
}