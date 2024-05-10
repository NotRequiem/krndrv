#include "imports.h"

NTSTATUS ioctl(PDEVICE_OBJECT device_object, PIRP irp)
{
    (device_object);

    DbgPrintEx(77, 3, "device is being called");

    NTSTATUS x = ((NTSTATUS)0x00000000L);

    PIO_STACK_LOCATION CurrentIrpStack = IoGetCurrentIrpStackLocation(irp);

    if (CurrentIrpStack == ((void*)0))
    {
        IofCompleteRequest(irp, 0);
        return ((NTSTATUS)0xC000000DL);
    }

    ULONG IoCtl = CurrentIrpStack->Parameters.DeviceIoControl.IoControlCode;

    if (IoCtl != attach && IoCtl != read && IoCtl != write)
    {
        IofCompleteRequest(irp, 0);
        return ((NTSTATUS)0xC0000010L);
    }

    if (CurrentIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(Request))
    {
        IofCompleteRequest(irp, 0);
        return ((NTSTATUS)0xC0000023L);
    }

    PRequest request = (PRequest)irp->AssociatedIrp.SystemBuffer;

    if (request == ((void*)0))
    {
        IofCompleteRequest(irp, 0);
        return ((NTSTATUS)0xC000000DL);
    }

    PEPROCESS target_proc = ((void*)0);

    switch (IoCtl)
    {
        case attach:
            x = PsLookupProcessByProcessId(request->pid, &target_proc);
            if (!(((NTSTATUS)(x)) >= 0) || target_proc == ((void*)0))
            {
                IofCompleteRequest(irp, 0);
                return x;
            }
            break;

        case read:
            if (target_proc == NULL || request->buffer == NULL || request->size == 0) {
                x = STATUS_INVALID_PARAMETER;
                break;
            }

            __try {
                ProbeForRead(request->target, request->size, sizeof(UCHAR));
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                x = GetExceptionCode();
                break;
            }

            if (target_proc != ((void*)0))
                x = MmCopyVirtualMemory(
                    target_proc,
                    request->target,
                    IoGetCurrentProcess(),
                    request->buffer,
                    request->size,
                    0,
                    &request->return_size
                );

            break;

        case write:
            if (request->buffer == ((void*)0) || request->target == ((void*)0))
            {
                x = ((NTSTATUS)0xC000000DL);
                break;
            }

            __try
            {
                ProbeForRead(request->buffer, request->size, sizeof(UCHAR));
                ProbeForWrite(request->target, request->size, sizeof(UCHAR));
            }
            __except (1)
            {
                x = _exception_code();
                break;
            }

            if (target_proc == ((void*)0))
            {
                x = ((NTSTATUS)0xC000000DL);
                break;
            }

            x = MmCopyVirtualMemory(
                (IoCtl == read) ? target_proc : IoGetCurrentProcess(),
                (IoCtl == read) ? request->target : request->buffer,
                (IoCtl == read) ? IoGetCurrentProcess() : target_proc,
                (IoCtl == read) ? request->buffer : request->target,
                request->size,
                0,
                &request->return_size);
            break;

        default:
            x = ((NTSTATUS)0xC0000010L);
            break;
    }

    if (target_proc != ((void*)0))
    {
        ObfDereferenceObject(target_proc);
    }

    irp->IoStatus.Status = x;
    irp->IoStatus.Information = sizeof(Request);

    IofCompleteRequest(irp, 0);

    return x;
}

NTSTATUS start(PDEVICE_OBJECT obj, PIRP irp)
{
    (obj);
    IofCompleteRequest(irp, 0);
    return irp->IoStatus.Status;
}

NTSTATUS remove(PDEVICE_OBJECT obj, PIRP irp)
{
    (obj);
    IofCompleteRequest(irp, 0);
    return irp->IoStatus.Status;
}

NTSTATUS entry_point(PDRIVER_OBJECT DrvObj, PUNICODE_STRING RegPath)
{
    (RegPath);

    UNICODE_STRING device;
    RtlInitUnicodeString(&device, L"\\Driver\\KMDrv");

    PDEVICE_OBJECT object = 0;
    NTSTATUS status = IoCreateDevice(
        DrvObj,
        0,
        &device,
        0x00000022,
        0x00000100,
        0,
        &object);

    if (!(((NTSTATUS)(status)) >= 0))
    {
        DbgPrintEx(77, 3, "failed to create device");
        return status;
    }

    DbgPrintEx(77, 3, "created device");

    UNICODE_STRING dos_path;
    RtlInitUnicodeString(&dos_path, L"\\DosDevices\\KMDrv");

    status = IoCreateSymbolicLink(&dos_path, &device);
    if (!(((NTSTATUS)(status)) >= 0))
    {
        DbgPrintEx(77, 3, "failed to create symbolic link");
        IoDeleteDevice(object);
        return status;
    }

    DbgPrintEx(77, 3, "created link");

    object->Flags |= 0x00000004;

    DrvObj->MajorFunction[0x00] = start;
    DrvObj->MajorFunction[0x02] = remove;
    DrvObj->MajorFunction[0x0e] = ioctl;

    object->Flags &= ~0x00000080;

    DbgPrintEx(77, 3, "started");

    return ((NTSTATUS)0x00000000L);
}

NTSTATUS DriverEntry()
{
    DbgPrintEx(77, 3, "started");

    UNICODE_STRING dvr;
    RtlInitUnicodeString(&dvr, L"\\Driver\\KMDrv");

    return IoCreateDriver(&dvr, entry_point);
}
