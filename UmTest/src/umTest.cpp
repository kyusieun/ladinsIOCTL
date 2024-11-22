#include "umTest.h"


int main() {
    const DWORD pid = get_process_id(L"Dummy.exe");

    if (pid == 0) {
        std::cout << "[-] Failed to get process id." << std::endl;
        std::cin.get();
        return 1;
    }
    std::cout << "[+] Process ID: " << pid << std::endl;

    const HANDLE driver = CreateFileA("\\\\.\\ladinsIOCTL", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (driver == INVALID_HANDLE_VALUE) {
        std::cout << "[-] Failed to get driver handle." << std::endl;
        std::cin.get();
        return 1;
    }
    if (driver::attach_to_process(driver, pid)) {
        std::cout << "[+] Attached to process." << std::endl;
    }
    else {
        std::cout << "[-] Failed to attach to process." << std::endl;
        std::cin.get();
        return 1;
    }

    const std::uintptr_t base_address = get_module_base(pid, L"Dummy.exe");
    std::cout << "[+] Base Address: 0x" << std::hex << base_address << std::endl;

    // Read memory
    uintptr_t intPtr = base_address + 0x5074;
    std::cout << "[+] intPtr: 0x" << std::hex << intPtr << std::endl;
    int intRead = driver::RPM<int>(driver, intPtr);
    std::cout << "[+] intRead: " << std::dec << intRead << std::endl;

    // Write memory
    int newValue = 84;
    if (driver::WPM<int>(driver, intPtr, newValue)) {
        std::cout << "[+] Wrote new value: " << newValue << std::endl;
    }

    // Query memory
    MEMORY_BASIC_INFORMATION memInfo = {0};
    driver::VQE(driver, intPtr, &memInfo, sizeof(MEMORY_BASIC_INFORMATION));
    std::cout << "[+] Base Address: " << memInfo.BaseAddress << std::endl;
    std::cout << "[+] Allocation Base: " << memInfo.AllocationBase << std::endl;
	std::cout << "[+] Allocation Protect: " << memInfo.AllocationProtect << std::endl;
    std::cout << "[+] Region Size: " << memInfo.RegionSize << std::endl;
	std::cout << "[+] State: " << memInfo.State << std::endl;
	std::cout << "[+] Protect: " << memInfo.Protect << std::endl;
	std::cout << "[+] Type: " << memInfo.Type << std::endl;

    std::cin.get();

    CloseHandle(driver);
    return 0;
}
