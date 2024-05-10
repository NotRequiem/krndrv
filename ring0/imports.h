#pragma once

#include <ntifs.h>

typedef struct _Request {
    HANDLE pid;
    PVOID target;
    PVOID buffer;
    SIZE_T size;
    SIZE_T return_size;
} Request, * PRequest;

typedef enum _Codes {
    attach = (((0x00000022) << 16) | (((0)) << 14) | ((0x696) << 2) | (0)),
    read = (((0x00000022) << 16) | (((0)) << 14) | ((0x697) << 2) | (0)),
    write = (((0x00000022) << 16) | (((0)) << 14) | ((0x698) << 2) | (0))
} Codes;

extern NTSTATUS IoCreateDriver(
    PUNICODE_STRING DriverName,
    PDRIVER_INITIALIZE InitializationFunction);

extern NTSTATUS MmCopyVirtualMemory(
    PEPROCESS SourceProcess,
    PVOID SourceAddress,
    PEPROCESS TargetProcess,
    PVOID TargetAddress,
    SIZE_T BufferSize,
    KPROCESSOR_MODE PreviousMode,
    PSIZE_T ReturnSize);


