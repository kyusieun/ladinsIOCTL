# ladinsIOCTL  

**ladinsIOCTL** provides a **kernel-mode** **RPM**(Read Process Memory), **WPM** (Write Process Memory), and **VQE** (Virtual Query Memory) operations. The project also includes a simple user-mode application for testing the driver's functionality.  

---

## 📂 Project Structure  

- **ladinsIOCTL/**  
  Contains the kernel driver source code that implements RPM, WPM, and VQE features.  
  Includes the IOCTL codes and kernel function calls required for these operations.  

- **UmTest/**  
  A user-mode application for testing the kernel driver through IOCTL calls.  
  Contains simple test cases to verify functionality.  

---

## ⚙️ Key Features  


<details><summary>UmTest.h
</summary>

``` cpp
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

```
</details>

1. **init_code**  
   - Attach to process with PID.

2. **read_code(ReadProcessMemory)**  
   - Reads data from a specified memory address in a target process.  

3. **write_code(WriteProcessMemory)**  
   - Writes data to a specified memory address in a target process.  

4. **query_code(VirtualQueryMemoryEx)**  
   - Queries memory mapping information (e.g., page state) of a specified memory address.  

---

## 🔨 Build and Run  

### Building and Installing the Driver  
1. Build the **ladinsIOCTL** project using Visual Studio 2022.  
2. Copy the resulting `.sys` file to an appropriate directory.  
3. Load the driver using [kdmapper](https://github.com/TheCruZ/kdmapper)

---

## ⚠ Limitation
This project can **bypass user-mode anti-cheat** systems, but additional work is required to bypass kernel-mode anti-cheat systems.
