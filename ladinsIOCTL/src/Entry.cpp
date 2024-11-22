#include "Entry.h"


#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION (0x0400)
#endif

#ifndef PROCESS_VM_READ
#define PROCESS_VM_READ (0x0010)
#endif

NTSTATUS ctl_io(PDEVICE_OBJECT device_obj, PIRP irp) {
    UNREFERENCED_PARAMETER(device_obj);

    static PEPROCESS s_target_process = nullptr;
    static HANDLE s_target_process_handle = nullptr;
    irp->IoStatus.Information = sizeof(info_t);

    auto stack = IoGetCurrentIrpStackLocation(irp);
    auto buffer = static_cast<info_t*>(irp->AssociatedIrp.SystemBuffer);

    if (!stack || !buffer || stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(info_t)) {
        irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    const auto ctl_code = stack->Parameters.DeviceIoControl.IoControlCode;
    KPROCESSOR_MODE previous_mode = ExGetPreviousMode();

    switch (ctl_code) {
    case init_code:
        // Ensure target_pid is valid
        if (!NT_SUCCESS(PsLookupProcessByProcessId(buffer->target_pid, &s_target_process))) {
            s_target_process = nullptr;
            irp->IoStatus.Status = STATUS_INVALID_CID;
        }
        else {
            // Open process handle
            NTSTATUS status = ObOpenObjectByPointer(
                s_target_process,
                OBJ_KERNEL_HANDLE,
                NULL,
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                *PsProcessType,
                KernelMode,
                &s_target_process_handle
            );

            if (NT_SUCCESS(status)) {
                irp->IoStatus.Status = STATUS_SUCCESS;
            }
            else {
                s_target_process = nullptr;
                irp->IoStatus.Status = status;
            }
        }
        break;

    case read_code:
    case write_code:
        if (!s_target_process) {
            irp->IoStatus.Status = STATUS_INVALID_HANDLE;
        }
        else if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
            irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        }
        else {
            PEPROCESS source_process = (ctl_code == read_code) ? s_target_process : PsGetCurrentProcess();
            PEPROCESS target_process = (ctl_code == read_code) ? PsGetCurrentProcess() : s_target_process;

            void* source_address = (ctl_code == read_code) ? reinterpret_cast<void*>(buffer->target_address) : reinterpret_cast<void*>(buffer->buffer_address);
            void* target_address = (ctl_code == read_code) ? reinterpret_cast<void*>(buffer->buffer_address) : reinterpret_cast<void*>(buffer->target_address);

            __try {
                irp->IoStatus.Status = MmCopyVirtualMemory(
                    source_process,
                    source_address,
                    target_process,
                    target_address,
                    buffer->size,
                    previous_mode,
                    &buffer->return_size
                );
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                irp->IoStatus.Status = GetExceptionCode();
            }
        }
        break;

    case query_code:
        if (!s_target_process) {
            irp->IoStatus.Status = STATUS_INVALID_HANDLE;
        }
        else {
            MEMORY_BASIC_INFORMATION mem_info;
            SIZE_T return_length;

            irp->IoStatus.Status = ZwQueryVirtualMemory(
                s_target_process_handle,
                buffer->target_address,
                MemoryBasicInformation,
                &mem_info,
                sizeof(mem_info),
                &return_length
            );

            if (NT_SUCCESS(irp->IoStatus.Status)) {
                buffer->return_size = return_length;
                RtlCopyMemory(buffer->buffer_address, &mem_info, sizeof(mem_info));
            }
        }
        break;

    default:
        irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return irp->IoStatus.Status;
}


NTSTATUS unsupported_io(PDEVICE_OBJECT device_obj, PIRP irp) {
	UNREFERENCED_PARAMETER(device_obj);

	irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

NTSTATUS create_io(PDEVICE_OBJECT device_obj, PIRP irp) {
	UNREFERENCED_PARAMETER(device_obj);

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

NTSTATUS close_io(PDEVICE_OBJECT device_obj, PIRP irp) {
	UNREFERENCED_PARAMETER(device_obj);

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

NTSTATUS real_main(PDRIVER_OBJECT driver_obj, PUNICODE_STRING registery_path) {
	UNREFERENCED_PARAMETER(registery_path);

	UNICODE_STRING dev_name, sym_link;
	PDEVICE_OBJECT dev_obj;

	RtlInitUnicodeString(&dev_name, L"\\Device\\ladinsIOCTL");
	auto status = IoCreateDevice(driver_obj, 0, &dev_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &dev_obj);
	if (status != STATUS_SUCCESS) return status;

	RtlInitUnicodeString(&sym_link, L"\\DosDevices\\ladinsIOCTL");
	status = IoCreateSymbolicLink(&sym_link, &dev_name);
	if (status != STATUS_SUCCESS) return status;

	SetFlag(dev_obj->Flags, DO_BUFFERED_IO); //set DO_BUFFERED_IO bit to 1

	for (int t = 0; t <= IRP_MJ_MAXIMUM_FUNCTION; t++) //set all MajorFunction's to unsupported
		driver_obj->MajorFunction[t] = unsupported_io;

	//then set supported functions to appropriate handlers
	driver_obj->MajorFunction[IRP_MJ_CREATE] = create_io; //link our io create function
	driver_obj->MajorFunction[IRP_MJ_CLOSE] = close_io; //link our io close function
	driver_obj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ctl_io; //link our control code handler
	driver_obj->DriverUnload = NULL; //add later

	ClearFlag(dev_obj->Flags, DO_DEVICE_INITIALIZING); //set DO_DEVICE_INITIALIZING bit to 0 (we are done initializing)
	return status;
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver_obj, PUNICODE_STRING registery_path) {
	UNREFERENCED_PARAMETER(driver_obj);
	UNREFERENCED_PARAMETER(registery_path);
	DbgPrintEx(0, 0, "ladinsIOCTL Entry");
	UNICODE_STRING  drv_name;
	RtlInitUnicodeString(&drv_name, L"\\Driver\\ladinsIOCTL");
	IoCreateDriver(&drv_name, &real_main);

	return STATUS_SUCCESS;
}