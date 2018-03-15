/**
* @file mingwdbghelp.h
* This file is needed for mingw32 because dbghelp.h do not exists.
*/

#ifdef __MINGW32__

#ifndef _INC_MINGWDBGHELP_H
#define _INC_MINGWDBGHELP_H

typedef enum _MINIDUMP_TYPE {
    MiniDumpNormal                          = 0x00000000,
    MiniDumpWithDataSegs                    = 0x00000001,
    MiniDumpWithFullMemory                  = 0x00000002,
    MiniDumpWithHandleData                  = 0x00000004,
    MiniDumpFilterMemory                    = 0x00000008,
    MiniDumpScanMemory                      = 0x00000010,
    MiniDumpWithUnloadedModules             = 0x00000020,
    MiniDumpWithIndirectlyReferencedMemory  = 0x00000040,
    MiniDumpFilterModulePaths               = 0x00000080,
    MiniDumpWithProcessThreadData           = 0x00000100,
    MiniDumpWithPrivateReadWriteMemory      = 0x00000200,
    MiniDumpWithoutOptionalData             = 0x00000400,
    MiniDumpWithFullMemoryInfo              = 0x00000800,
    MiniDumpWithThreadInfo                  = 0x00001000,
    MiniDumpWithCodeSegs                    = 0x00002000,
    MiniDumpWithoutAuxiliaryState           = 0x00004000,
    MiniDumpWithFullAuxiliaryState          = 0x00008000,
    MiniDumpWithPrivateWriteCopyMemory      = 0x00010000,
    MiniDumpIgnoreInaccessibleMemory        = 0x00020000,
    MiniDumpWithTokenInformation            = 0x00040000,
    MiniDumpWithModuleHeaders               = 0x00080000,
    MiniDumpFilterTriage                    = 0x00100000,
    MiniDumpValidTypeFlags                  = 0x001fffff
} MINIDUMP_TYPE;


typedef struct _MINIDUMP_EXCEPTION_INFORMATION {
    DWORD               ThreadId;
    PEXCEPTION_POINTERS ExceptionPointers;
    BOOL                ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION, *PMINIDUMP_EXCEPTION_INFORMATION;


typedef struct _MINIDUMP_USER_STREAM {
    UINT	Type;
    UINT	BufferSize;
    PVOID   Buffer;
} MINIDUMP_USER_STREAM, *PMINIDUMP_USER_STREAM;


typedef struct _MINIDUMP_USER_STREAM_INFORMATION {
	UINT                  UserStreamCount;
    PMINIDUMP_USER_STREAM UserStreamArray;
} MINIDUMP_USER_STREAM_INFORMATION, *PMINIDUMP_USER_STREAM_INFORMATION;


typedef struct _MINIDUMP_THREAD_CALLBACK {
	UINT    ThreadId;
    HANDLE  ThreadHandle;
    CONTEXT Context;
	UINT    SizeOfContext;
	ULONGLONG  StackBase;
	ULONGLONG  StackEnd;
} MINIDUMP_THREAD_CALLBACK, *PMINIDUMP_THREAD_CALLBACK;


typedef struct _MINIDUMP_THREAD_EX_CALLBACK {
    UINT    ThreadId;
    HANDLE  ThreadHandle;
    CONTEXT Context;
	UINT    SizeOfContext;
	ULONGLONG  StackBase;
	ULONGLONG  StackEnd;
	ULONGLONG  BackingStoreBase;
	ULONGLONG  BackingStoreEnd;
} MINIDUMP_THREAD_EX_CALLBACK, *PMINIDUMP_THREAD_EX_CALLBACK;


typedef struct _MINIDUMP_MODULE_CALLBACK {
    PWCHAR           FullPath;
	ULONGLONG           BaseOfImage;
	UINT             SizeOfImage;
	UINT             CheckSum;
	UINT             TimeDateStamp;
    VS_FIXEDFILEINFO VersionInfo;
    PVOID            CvRecord;
	UINT             SizeOfCvRecord;
    PVOID            MiscRecord;
	UINT             SizeOfMiscRecord;
} MINIDUMP_MODULE_CALLBACK, *PMINIDUMP_MODULE_CALLBACK;


typedef struct _MINIDUMP_INCLUDE_THREAD_CALLBACK {
	UINT  ThreadId;
} MINIDUMP_INCLUDE_THREAD_CALLBACK, *PMINIDUMP_INCLUDE_THREAD_CALLBACK;


typedef struct _MINIDUMP_INCLUDE_MODULE_CALLBACK {
	ULONGLONG BaseOfImage;
} MINIDUMP_INCLUDE_MODULE_CALLBACK, *PMINIDUMP_INCLUDE_MODULE_CALLBACK;


typedef struct _MINIDUMP_IO_CALLBACK {
    HANDLE  Handle;
	ULONGLONG  Offset;
    PVOID   Buffer;
	UINT    BufferBytes;
} MINIDUMP_IO_CALLBACK, *PMINIDUMP_IO_CALLBACK;


typedef struct _MINIDUMP_READ_MEMORY_FAILURE_CALLBACK {
	ULONGLONG  Offset;
	UINT    Bytes;
    HRESULT FailureStatus;
} MINIDUMP_READ_MEMORY_FAILURE_CALLBACK, *PMINIDUMP_READ_MEMORY_FAILURE_CALLBACK;


typedef struct _MINIDUMP_CALLBACK_INPUT {
	UINT   ProcessId;
    HANDLE ProcessHandle;
	UINT   CallbackType;
    union {
        HRESULT                               Status;
        MINIDUMP_THREAD_CALLBACK              Thread;
        MINIDUMP_THREAD_EX_CALLBACK           ThreadEx;
        MINIDUMP_MODULE_CALLBACK              Module;
        MINIDUMP_INCLUDE_THREAD_CALLBACK      IncludeThread;
        MINIDUMP_INCLUDE_MODULE_CALLBACK      IncludeModule;
        MINIDUMP_IO_CALLBACK                  Io;
        MINIDUMP_READ_MEMORY_FAILURE_CALLBACK ReadMemoryFailure;
		UINT                                  SecondaryFlags;
    };
} MINIDUMP_CALLBACK_INPUT, *PMINIDUMP_CALLBACK_INPUT;


typedef struct _MINIDUMP_MEMORY_INFO {
	ULONGLONG	BaseAddress;
	ULONGLONG	AllocationBase;
	ULONGLONG	AllocationProtect;
	ULONGLONG	__alignment1;
	ULONGLONG	RegionSize;
	UINT	State;
	UINT	Protect;
	UINT	Type;
	UINT	__alignment2;
} MINIDUMP_MEMORY_INFO, *PMINIDUMP_MEMORY_INFO;


typedef struct _MINIDUMP_CALLBACK_OUTPUT {
    union {
		UINT  ModuleWriteFlags;
		UINT  ThreadWriteFlags;
		UINT  SecondaryFlags;
        struct {
			ULONGLONG MemoryBase;
			UINT   MemorySize;
        };
        struct {
            BOOL CheckCancel;
            BOOL Cancel;
        };
        HANDLE Handle;
    };
    struct {
        MINIDUMP_MEMORY_INFO VmRegion;
        BOOL                 Continue;
    };
    HRESULT Status;
} MINIDUMP_CALLBACK_OUTPUT, *PMINIDUMP_CALLBACK_OUTPUT;


typedef BOOL (WINAPI * MINIDUMP_CALLBACK_ROUTINE) (
IN PVOID CallbackParam,
IN CONST PMINIDUMP_CALLBACK_INPUT CallbackInput,
        IN OUT PMINIDUMP_CALLBACK_OUTPUT CallbackOutput
);


typedef struct _MINIDUMP_CALLBACK_INFORMATION {
    MINIDUMP_CALLBACK_ROUTINE CallbackRoutine;
    PVOID                     CallbackParam;
} MINIDUMP_CALLBACK_INFORMATION, *PMINIDUMP_CALLBACK_INFORMATION;


#endif // _INC_MINGWDBGHELP_H

#endif  // __MINGW32__
