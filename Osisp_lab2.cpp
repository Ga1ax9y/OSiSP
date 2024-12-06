#include <windows.h>
#include <iostream>
#include <string>

struct Metadata {
    uint32_t data_size;
    uint32_t data_count;
    uint32_t file_size;
};

struct DataBlock {
    uint32_t length;
    char data[256];            
};

LPVOID InitializeDatabaseFile(HANDLE& hFile, HANDLE& hMapping, const std::wstring& filename, Metadata*& metadata, size_t initial_size);
void AddData(HANDLE hFile, HANDLE& hMapping, LPVOID& BaseAddress, Metadata*& metadata, const std::string& new_data);
void RemoveData(LPVOID BaseAddress, Metadata* metadata, uint32_t data_index);
void PrintData(LPVOID BaseAddress, Metadata* metadata);
void ExpandFile(HANDLE hFile, HANDLE& hMapping, LPVOID& BaseAddress, Metadata*& metadata, size_t additional_size);
void CloseDatabase(LPVOID BaseAddress, HANDLE hMapping, HANDLE hFile);
size_t CalculateInitialFileSize(size_t data_size, size_t data_count);

LPVOID InitializeDatabaseFile(HANDLE& hFile, HANDLE& hMapping, const std::wstring& filename, Metadata*& metadata, size_t initial_size) {
    hFile = CreateFileW(filename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка открытия или создания файла!" << std::endl;
        return NULL;
    }

    DWORD file_size = GetFileSize(hFile, NULL);
    if (file_size == INVALID_FILE_SIZE) {
        std::cerr << "Ошибка получения размера файла!" << std::endl;
        CloseHandle(hFile);
        return NULL;
    }

    if (file_size == 0) {
        SetFilePointer(hFile, initial_size - 1, NULL, FILE_BEGIN);
        if (!SetEndOfFile(hFile)) {
            std::cerr << "Ошибка установки конца файла!" << std::endl;
            CloseHandle(hFile);
            return NULL;
        }
        file_size = initial_size;
    }

    hMapping = CreateFileMappingW(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (hMapping == NULL) {
        std::cerr << "Ошибка создания отображения файла!" << std::endl;
        CloseHandle(hFile);
        return NULL;
    }

    LPVOID BaseAddress = MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (BaseAddress == NULL) {
        std::cerr << "Ошибка отображения файла в память!" << std::endl;
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return NULL;
    }

    metadata = (Metadata*)BaseAddress;

    if (file_size == initial_size) {
        metadata->data_size = sizeof(DataBlock);
        metadata->data_count = 0;
        metadata->file_size = initial_size;
    }
    else {
        if (metadata->data_size != sizeof(DataBlock)) {
            std::cerr << "Несоответствие размера записи. Ожидается: " << sizeof(DataBlock)
                << ", найдено: " << metadata->data_size << std::endl;
            UnmapViewOfFile(BaseAddress);
            CloseHandle(hMapping);
            CloseHandle(hFile);
            return NULL;
        }
    }

    return BaseAddress;
}

void AddData(HANDLE hFile, HANDLE& hMapping, LPVOID& BaseAddress, Metadata*& metadata, const std::string& new_data) {
    if ((metadata->data_count + 1) * metadata->data_size + sizeof(Metadata) > metadata->file_size) {
        std::cerr << "Не хватает места, требуется расширение файла." << std::endl;
        ExpandFile(hFile, hMapping, BaseAddress, metadata, 1024); // Расширяем файл на 1024 байта
        std::cout << "Файл был расширен до размера: " << metadata->file_size << " байт." << std::endl;
    }

    void* data_position = (char*)BaseAddress + sizeof(Metadata) + metadata->data_count * metadata->data_size;

    DataBlock* datablock = (DataBlock*)data_position;
    datablock->length = static_cast<uint32_t>(new_data.size());

    if (new_data.size() >= sizeof(datablock->data)) {
        std::cerr << "Длина записи превышает допустимый размер. Запись не добавлена." << std::endl;
        return;
    }

    memcpy(datablock->data, new_data.c_str(), datablock->length);

    if (datablock->length < sizeof(datablock->data) - 1) {
        memset(datablock->data + datablock->length, 0, sizeof(datablock->data) - datablock->length);
    }

    metadata->data_count++;
}

void RemoveData(LPVOID BaseAddress, Metadata* metadata, uint32_t data_index) {
    if (data_index >= metadata->data_count) {
        std::cerr << "Индекс записи для удаления превышает количество записей." << std::endl;
        return;
    }

    DataBlock* block_to_remove = (DataBlock*)((char*)BaseAddress + sizeof(Metadata) + data_index * metadata->data_size);

    memset(block_to_remove, 0, sizeof(DataBlock));

    for (uint32_t i = data_index; i < metadata->data_count - 1; ++i) {
        DataBlock* current_block = (DataBlock*)((char*)BaseAddress + sizeof(Metadata) + i * metadata->data_size);
        DataBlock* next_block = (DataBlock*)((char*)BaseAddress + sizeof(Metadata) + (i + 1) * metadata->data_size);
        memcpy(current_block, next_block, sizeof(DataBlock));
    }

    metadata->data_count--;
}

void PrintData(LPVOID BaseAddress, Metadata* metadata) {
    if (metadata->data_count == 0) {
        std::cout << "База данных пуста." << std::endl;
        return;
    }

    char* blocks = (char*)BaseAddress + sizeof(Metadata);

    for (uint32_t i = 0; i < metadata->data_count; ++i) {
        DataBlock* block = (DataBlock*)(blocks + i * metadata->data_size);

        if (block->length > sizeof(block->data) - 1) {
            std::cerr << "Некорректная длина записи " << i + 1 << "." << std::endl;
            continue;
        }

        std::string data_str(block->data, block->length);

        std::cout << "Запись " << i + 1 << ": " << data_str << std::endl;
    }
}

void ExpandFile(HANDLE hFile, HANDLE& hMapping, LPVOID& BaseAddress, Metadata*& metadata, size_t additional_size) {
    size_t data_to_add = additional_size / metadata->data_size;
    size_t new_size = metadata->file_size + data_to_add * metadata->data_size;

    SetFilePointer(hFile, new_size - 1, NULL, FILE_BEGIN);
    if (!SetEndOfFile(hFile)) {
        std::cerr << "Ошибка установки конца файла при расширении." << std::endl;
        return;
    }
    UnmapViewOfFile(BaseAddress);
    CloseHandle(hMapping);

    hMapping = CreateFileMappingW(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (hMapping == NULL) {
        std::cerr << "Ошибка создания отображения файла при расширении." << std::endl;
        CloseHandle(hFile);
        BaseAddress = NULL;
        return;
    }

    BaseAddress = MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (BaseAddress == NULL) {
        std::cerr << "Ошибка отображения файла в память при расширении." << std::endl;
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return;
    }

    metadata = (Metadata*)BaseAddress;
    metadata->file_size = new_size;
}

void CloseDatabase(LPVOID BaseAddress, HANDLE hMapping, HANDLE hFile) {
    if (BaseAddress != NULL) {
        UnmapViewOfFile(BaseAddress);
    }
    if (hMapping != NULL) {
        CloseHandle(hMapping);
    }
    if (hFile != NULL) {
        CloseHandle(hFile);
    }
}

size_t CalculateInitialFileSize(size_t data_size, size_t data_count) {
    return sizeof(Metadata) + data_size * data_count;
}

int main() {
    setlocale(LC_ALL, "ru");
    HANDLE hFile = NULL, hMapping = NULL;
    LPVOID BaseAddress = NULL;
    Metadata* metadata = NULL;

    size_t initial_size = CalculateInitialFileSize(sizeof(DataBlock), 5);

    BaseAddress = InitializeDatabaseFile(hFile, hMapping, L"database.dat", metadata, initial_size);
    if (BaseAddress == NULL) {
        std::cerr << "Не удалось инициализировать базу данных." << std::endl;
        return 1;
    }
    std::string user_input;
    std::cout << "Введите строку для помещения в базу данных: ";
    std::cin >> user_input;
    AddData(hFile, hMapping, BaseAddress, metadata, user_input);
    for (int i = 0; i < 5; i++)
    {
        AddData(hFile, hMapping, BaseAddress, metadata, "new_block"+ std::to_string(i));
    }

    std::cout << "Добавлены записи. Всего записей: " << metadata->data_count << std::endl;

    std::cout << "Содержимое базы данных:" << std::endl;
    PrintData(BaseAddress, metadata);

    if (metadata->data_count > 0) {
        RemoveData(BaseAddress, metadata, 0);
        std::cout << "Первая запись удалена. Осталось записей: " << metadata->data_count << std::endl;
    }

    std::cout << "Содержимое базы данных после удаления:" << std::endl;
    PrintData(BaseAddress, metadata);

    CloseDatabase(BaseAddress, hMapping, hFile);

    return 0;
}
