public type PSECURITY_ATTRIBUTES = *mut SECURITY_ATTRIBUTES
public type LPSECURITY_ATTRIBUTES = *mut SECURITY_ATTRIBUTES

public comptime const FORMAT_MESSAGE_ALLOCATE_BUFFER = 0x00000100
public comptime const FORMAT_MESSAGE_IGNORE_INSERTS =  0x00000200
public comptime const FORMAT_MESSAGE_FROM_STRING =     0x00000400
public comptime const FORMAT_MESSAGE_FROM_HMODULE =    0x00000800
public comptime const FORMAT_MESSAGE_FROM_SYSTEM =     0x00001000
public comptime const FORMAT_MESSAGE_ARGUMENT_ARRAY =  0x00002000
public comptime const FORMAT_MESSAGE_MAX_WIDTH_MASK =  0x000000FF

@extern
@dllimport
@stdcall
public func FormatMessageA(
     dwFlags : DWORD,
     lpSource : LPCVOID,
     dwMessageId : DWORD,
     dwLanguageId : DWORD,
     lpBuffer : LPSTR,
     nSize : DWORD,
     Arguments : *va_list
) : DWORD;

@extern
@dllimport
@stdcall
public func CreateNamedPipeA(
    lpName : LPCSTR,
    dwOpenMode : DWORD,
    dwPipeMode : DWORD,
    nMaxInstances : DWORD,
    nOutBufferSize : DWORD,
    nInBufferSize : DWORD,
    nDefaultTimeOut : DWORD,
    lpSecurityAttributes : LPSECURITY_ATTRIBUTES
) : HANDLE;

public type LPOVERLAPPED = *mut OVERLAPPED
public type HLOCAL = HANDLE

@extern
@dllimport
@stdcall
public func ConnectNamedPipe(
    hNamedPipe : HANDLE,
    lpOverlapped : LPOVERLAPPED
) : BOOL

@extern
@dllimport
@stdcall
public func WaitNamedPipeA(
    lpNamedPipeName : LPCSTR,
    nTimeOut : DWORD
) : BOOL

@extern
@dllimport
@stdcall
public func LocalFree(
    hMem : HLOCAL
) : HLOCAL

//
// Define the dwOpenMode values for CreateNamedPipe
//

public comptime const PIPE_ACCESS_INBOUND = 0x00000001
public comptime const PIPE_ACCESS_OUTBOUND = 0x00000002
public comptime const PIPE_ACCESS_DUPLEX = 0x00000003

//
// Define the dwPipeMode values for CreateNamedPipe
//

public comptime const PIPE_WAIT =                   0x00000000
public comptime const PIPE_NOWAIT =                 0x00000001
public comptime const PIPE_READMODE_BYTE =          0x00000000
public comptime const PIPE_READMODE_MESSAGE =       0x00000002
public comptime const PIPE_TYPE_BYTE =              0x00000000
public comptime const PIPE_TYPE_MESSAGE =           0x00000004
public comptime const PIPE_ACCEPT_REMOTE_CLIENTS =  0x00000000
public comptime const PIPE_REJECT_REMOTE_CLIENTS =  0x00000008

// --- Anonymous pipe creation ---

/**
 * @brief Creates an anonymous pipe.
 * @param lpReadPipe Receives the read handle.
 * @param lpWritePipe Receives the write handle.
 * @param lpPipeAttributes Optional security attributes.
 * @param nSize Buffer size (0 = default).
 * @return Nonzero on success, zero on failure.
 */
@extern
@dllimport
@stdcall
public func CreatePipe(
    lpReadPipe : *mut HANDLE,
    lpWritePipe : *mut HANDLE,
    lpPipeAttributes : LPSECURITY_ATTRIBUTES,
    nSize : DWORD
) : BOOL

// --- Pipe peek ---

/**
 * @brief Peeks at the data in the pipe without removing it.
 * @param hNamedPipe Handle to the pipe.
 * @param lpBuffer Buffer to receive data (can be NULL for size query).
 * @param nBufferSize Size of the buffer.
 * @param lpBytesRead Receives number of bytes read into buffer.
 * @param lpTotalBytesAvail Receives total bytes available.
 * @param lpBytesLeftThisMessage Receives bytes left in message (byte-mode pipes: 0).
 * @return Nonzero on success, zero on failure.
 */
@extern
@dllimport
@stdcall
public func PeekNamedPipe(
    hNamedPipe : HANDLE,
    lpBuffer : LPVOID,
    nBufferSize : DWORD,
    lpBytesRead : *mut DWORD,
    lpTotalBytesAvail : *mut DWORD,
    lpBytesLeftThisMessage : *mut DWORD
) : BOOL