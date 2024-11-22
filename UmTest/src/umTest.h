#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <ntstatus.h>


namespace driver {
    namespace codes {
        constexpr ULONG init_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x775, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
        constexpr ULONG read_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x776, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
        constexpr ULONG write_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x777, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
        constexpr ULONG query_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x778, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
    }

    struct info_t {
        UINT64 target_pid = 0;
        UINT64 target_address = 0x0; // address in the target process we want to read from / write to
        UINT64 buffer_address = 0x0; // address in our usermode process to copy to (read mode) / read from (write mode)
        UINT64 size = 0;
        UINT64 return_size = 0;
    };

    bool attach_to_process(HANDLE driver_handle, DWORD process_id) {
        info_t io_info;
        io_info.target_pid = process_id;

        return DeviceIoControl(driver_handle, codes::init_code, &io_info, sizeof(io_info), &io_info, sizeof(io_info), nullptr, nullptr);
    }

    template<typename T> T RPM(HANDLE driver_handle, const UINT64 address) {
        info_t io_info{};
        T read_data = {};

        io_info.target_address = address;
        io_info.buffer_address = reinterpret_cast<UINT64>(&read_data);
        io_info.size = sizeof(T);

        if (!DeviceIoControl(driver_handle, codes::read_code, &io_info, sizeof(io_info), &io_info, sizeof(io_info), nullptr, nullptr)) {
            DWORD error = GetLastError();
            std::cout << "RPM failed. Error code: " << error << std::endl;
        }
        return read_data;
    }

    template<typename T> bool WPM(HANDLE driver_handle, const UINT64 address, const T& data) {
        info_t io_info{};

        io_info.target_address = address;
        io_info.buffer_address = reinterpret_cast<UINT64>(&data);
        io_info.size = sizeof(T);

        if (!DeviceIoControl(driver_handle, codes::write_code, &io_info, sizeof(io_info), &io_info, sizeof(io_info), nullptr, nullptr)) {
            DWORD error = GetLastError();
            std::cout << "WPM failed. Error code: " << error << std::endl;
            return false;
        }
        return true;
    }

    bool VQE(HANDLE driver_handle, const UINT64 address, PMEMORY_BASIC_INFORMATION mbi, SIZE_T dwLength) {
        info_t io_info{};

        io_info.target_address = address;
        io_info.buffer_address = reinterpret_cast<UINT64>(mbi);
        io_info.size = dwLength;

        if (!DeviceIoControl(driver_handle, codes::query_code, &io_info, sizeof(io_info), &io_info, sizeof(io_info), nullptr, nullptr)) {
            DWORD error = GetLastError();
            std::cout << "VQE failed. Error code: " << error << std::endl;
            return false;
        }
        return true;
    }
}

static DWORD get_process_id(const wchar_t* process_name) {
    DWORD process_id = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 process_entry = { 0 };
        process_entry.dwSize = sizeof(process_entry);

        if (Process32First(snapshot, &process_entry)) {
            do {
                if (!_wcsicmp(process_entry.szExeFile, process_name)) {
                    process_id = process_entry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &process_entry));
        }

        CloseHandle(snapshot);
    }
    return process_id;
}

static std::uintptr_t get_module_base(DWORD pid, const wchar_t* module_name) {
    std::uintptr_t base_address = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);

    if (snapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 module_entry = { 0 };
        module_entry.dwSize = sizeof(module_entry);

        if (Module32First(snapshot, &module_entry)) {
            do {
                if (!_wcsicmp(module_entry.szModule, module_name)) {
                    base_address = reinterpret_cast<std::uintptr_t>(module_entry.modBaseAddr);
                    break;
                }
            } while (Module32Next(snapshot, &module_entry));
        }

        CloseHandle(snapshot);
    }
    return base_address;
}