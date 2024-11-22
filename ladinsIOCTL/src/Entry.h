#pragma once
#ifndef ENTRY_H
#define ENTRY_H

#include <ntifs.h>



// undocumented windows internal functions (exported by ntoskrnl)
extern "C" {
    NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName, PDRIVER_INITIALIZE InitializationFunction);
    NTKERNELAPI NTSTATUS MmCopyVirtualMemory(PEPROCESS SourceProcess, PVOID SourceAddress, PEPROCESS TargetProcess, PVOID TargetAddress, SIZE_T BufferSize, KPROCESSOR_MODE PreviousMode, PSIZE_T ReturnSize);
    NTKERNELAPI NTSTATUS ZwQueryVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID MemoryInformation, SIZE_T MemoryInformationLength, PSIZE_T ReturnLength);
}

constexpr ULONG init_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x775, METHOD_BUFFERED, FILE_SPECIAL_ACCESS); 
constexpr ULONG read_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x776, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
constexpr ULONG write_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x777, METHOD_BUFFERED, FILE_SPECIAL_ACCESS); 
constexpr ULONG query_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x778, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

struct info_t {
    HANDLE target_pid = 0;
    void* target_address = 0x0; // address in the target proces we want to read from / write to
    void* buffer_address = 0x0; // address in our usermode process to copy to (read mode) / read from (write mode)
    SIZE_T size = 0; 
    SIZE_T return_size = 0;
};

// Function prototypes
NTSTATUS ctl_io(PDEVICE_OBJECT device_obj, PIRP irp);
NTSTATUS unsupported_io(PDEVICE_OBJECT device_obj, PIRP irp);
NTSTATUS create_io(PDEVICE_OBJECT device_obj, PIRP irp);
NTSTATUS close_io(PDEVICE_OBJECT device_obj, PIRP irp);
NTSTATUS real_main(PDRIVER_OBJECT driver_obj, PUNICODE_STRING registery_path);
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver_obj, PUNICODE_STRING registery_path);

#endif // ENTRY_H
