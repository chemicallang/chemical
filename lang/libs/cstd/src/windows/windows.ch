if (def.windows) {

    // TODO check these three declarations are correct
    // - usize: ubigint (unsigned 64-bit) seems reasonable for a modern target's pointer size.
    // - ULONG_PTR: Changed from *mut ulong to usize. ULONG_PTR is an unsigned integer type large enough to hold a pointer.
    // - UINT: uint seems correct (typically 32-bit unsigned).
    public type usize = ubigint
    public type ULONG_PTR = usize // Was *mut ulong, which is incorrect. Should be pointer-sized uint.
    public type UINT = uint

    public type off_t = long

    /** @brief Windows handle type. */
    public type HANDLE = *mut void
    /** @brief Module handle. */
    public type HMODULE = HANDLE
    /** @brief Instance handle. */
    public type HINSTANCE = HANDLE
    /** @brief Pointer to procedure. */
    public type FARPROC = *mut void
    /** @brief Handle to Registry Key. */
    public type HKEY = HANDLE

    /** @brief Boolean: nonzero is TRUE, zero is FALSE. */
    public type BOOL = int
    /** @brief Unsigned 8-bit integer. */
    public type BYTE = uchar
    /** @brief 16-bit unsigned integer. */
    public type WORD = ushort
    /** @brief 32-bit unsigned integer. */
    public type DWORD = uint32_t
    /** @brief Signed 32-bit integer. */
    public type LONG = int32_t
    /** @brief 64-bit signed integer. */
    public type LONGLONG = bigint
    /** @brief 64-bit unsigned integer. */
    public type ULONGLONG = ubigint
    /** @brief Pointer-sized unsigned integer. */
    public type SIZE_T = usize
    /** @brief Generic pointer. */
    public type LPVOID = *mut void
    /** @brief Pointer to constant data. */
    public type LPCVOID = *void
    /** @brief Pointer to CHAR string. */
    public type LPSTR = *mut char
    /** @brief Pointer to constant CHAR string. */
    public type LPCSTR = *char
    /** @brief Pointer to WCHAR string. */
    public type LPWSTR = *mut ushort
    /** @brief Pointer to constant WCHAR string. */
    public type LPCWSTR = *ushort
    /** @brief Pointer to DWORD. */
    public type LPDWORD = *mut DWORD

    /** @brief Invalid handle constant. */
    @comptime public const INVALID_HANDLE_VALUE : HANDLE = -1 as HANDLE
    /** @brief Invalid file attributes constant. */
    @comptime public const INVALID_FILE_ATTRIBUTES : DWORD = 0xFFFFFFFF as DWORD

    /** @brief File attribute: directory. */
    @comptime public const FILE_ATTRIBUTE_DIRECTORY : DWORD = 0x10 as DWORD
    /** @brief File attribute: normal. */
    @comptime public const FILE_ATTRIBUTE_NORMAL : DWORD = 0x80 as DWORD
    /** @brief File attribute: read-only. */
    @comptime public const FILE_ATTRIBUTE_READONLY : DWORD = 0x01 as DWORD
    /** @brief File attribute: hidden. */
    @comptime public const FILE_ATTRIBUTE_HIDDEN : DWORD = 0x02 as DWORD
    /** @brief File attribute: system. */
    @comptime public const FILE_ATTRIBUTE_SYSTEM : DWORD = 0x04 as DWORD
    /** @brief File attribute: archive. */
    @comptime public const FILE_ATTRIBUTE_ARCHIVE : DWORD = 0x20 as DWORD

    /** @brief Desired access: read. */
    @comptime public const GENERIC_READ : DWORD = 0x80000000 as DWORD
    /** @brief Desired access: write. */
    @comptime public const GENERIC_WRITE : DWORD = 0x40000000 as DWORD
    /** @brief Desired access: execute. */
    @comptime public const GENERIC_EXECUTE : DWORD = 0x20000000 as DWORD
    /** @brief Desired access: all. */
    @comptime public const GENERIC_ALL : DWORD = 0x10000000 as DWORD

    /** @brief Share mode: read. */
    @comptime public const FILE_SHARE_READ : DWORD = 0x00000001 as DWORD
    /** @brief Share mode: write. */
    @comptime public const FILE_SHARE_WRITE : DWORD = 0x00000002 as DWORD
    /** @brief Share mode: delete. */
    @comptime public const FILE_SHARE_DELETE : DWORD = 0x00000004 as DWORD

    /** @brief Creation disposition: create new file, fail if exists. */
    @comptime public const CREATE_NEW : DWORD = 1 as DWORD
    /** @brief Creation disposition: create new file, overwrite if exists. */
    @comptime public const CREATE_ALWAYS : DWORD = 2 as DWORD
    /** @brief Creation disposition: open existing file. */
    @comptime public const OPEN_EXISTING : DWORD = 3 as DWORD
    /** @brief Creation disposition: open file, create if it doesn't exist. */
    @comptime public const OPEN_ALWAYS : DWORD = 4 as DWORD
    /** @brief Creation disposition: truncate existing file to zero length. */
    @comptime public const TRUNCATE_EXISTING : DWORD = 5 as DWORD

    /** @brief File move method: beginning of file. */
    @comptime public const FILE_BEGIN : DWORD = 0 as DWORD
    /** @brief File move method: current position. */
    @comptime public const FILE_CURRENT : DWORD = 1 as DWORD
    /** @brief File move method: end of file. */
    @comptime public const FILE_END : DWORD = 2 as DWORD

    /** @brief Memory allocation: commit pages. */
    @comptime public const MEM_COMMIT : DWORD = 0x1000 as DWORD
    /** @brief Memory allocation: reserve address range. */
    @comptime public const MEM_RESERVE : DWORD = 0x2000 as DWORD
    /** @brief Memory free: release pages. */
    @comptime public const MEM_RELEASE : DWORD = 0x8000 as DWORD
    /** @brief Memory free: decommit pages. */
    @comptime public const MEM_DECOMMIT : DWORD = 0x4000 as DWORD

    /** @brief Page protection: read/write. */
    @comptime public const PAGE_READWRITE : DWORD = 0x04 as DWORD
    /** @brief Page protection: execute. */
    @comptime public const PAGE_EXECUTE : DWORD = 0x10 as DWORD
    /** @brief Page protection: execute/read. */
    @comptime public const PAGE_EXECUTE_READ : DWORD = 0x20 as DWORD
    /** @brief Page protection: execute/read/write. */
    @comptime public const PAGE_EXECUTE_READWRITE : DWORD = 0x40 as DWORD
    /** @brief Page protection: no access. */
    @comptime public const PAGE_NOACCESS : DWORD = 0x01 as DWORD
    /** @brief Page protection: read only. */
    @comptime public const PAGE_READONLY : DWORD = 0x02 as DWORD

    /** @brief Standard Handle: Input */
    @comptime public const STD_INPUT_HANDLE : DWORD = -10 as DWORD
    /** @brief Standard Handle: Output */
    @comptime public const STD_OUTPUT_HANDLE : DWORD = -11 as DWORD
    /** @brief Standard Handle: Error */
    @comptime public const STD_ERROR_HANDLE : DWORD = -12 as DWORD

    // --- Added registry constants ---
    /** @brief Registry Key: HKEY_CLASSES_ROOT */
    @comptime public const HKEY_CLASSES_ROOT : HKEY = 0x80000000 as HKEY
    /** @brief Registry Key: HKEY_CURRENT_USER */
    @comptime public const HKEY_CURRENT_USER : HKEY = 0x80000001 as HKEY
    /** @brief Registry Key: HKEY_LOCAL_MACHINE */
    @comptime public const HKEY_LOCAL_MACHINE : HKEY = 0x80000002 as HKEY
    /** @brief Registry Key: HKEY_USERS */
    @comptime public const HKEY_USERS : HKEY = 0x80000003 as HKEY
    /** @brief Registry Key: HKEY_CURRENT_CONFIG */
    @comptime public const HKEY_CURRENT_CONFIG : HKEY = 0x80000005 as HKEY

    /** @brief Registry Access: Query Value */
    @comptime public const KEY_QUERY_VALUE : DWORD = 0x0001 as DWORD
    /** @brief Registry Access: Set Value */
    @comptime public const KEY_SET_VALUE : DWORD = 0x0002 as DWORD
    /** @brief Registry Access: Create Sub Key */
    @comptime public const KEY_CREATE_SUB_KEY : DWORD = 0x0004 as DWORD
    /** @brief Registry Access: Enumerate Sub Keys */
    @comptime public const KEY_ENUMERATE_SUB_KEYS : DWORD = 0x0008 as DWORD
    /** @brief Registry Access: Notify */
    @comptime public const KEY_NOTIFY : DWORD = 0x0010 as DWORD
    /** @brief Registry Access: Create Link */
    @comptime public const KEY_CREATE_LINK : DWORD = 0x0020 as DWORD
    /** @brief Registry Access: Write */
    @comptime public const KEY_WRITE : DWORD = 0x20006 as DWORD // (STANDARD_RIGHTS_WRITE | KEY_SET_VALUE | KEY_CREATE_SUB_KEY) & (~SYNCHRONIZE)
    /** @brief Registry Access: Read */
    @comptime public const KEY_READ : DWORD = 0x20019 as DWORD // (STANDARD_RIGHTS_READ | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_NOTIFY) & (~SYNCHRONIZE)
    /** @brief Registry Access: All Access */
    @comptime public const KEY_ALL_ACCESS : DWORD = 0xF003F as DWORD // (STANDARD_RIGHTS_ALL | KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS | KEY_NOTIFY | KEY_CREATE_LINK) & (~SYNCHRONIZE)

    /** @brief Registry Value Type: String */
    @comptime public const REG_SZ : DWORD = 1 as DWORD
    /** @brief Registry Value Type: Expandable String */
    @comptime public const REG_EXPAND_SZ : DWORD = 2 as DWORD
    /** @brief Registry Value Type: Binary */
    @comptime public const REG_BINARY : DWORD = 3 as DWORD
    /** @brief Registry Value Type: DWORD */
    @comptime public const REG_DWORD : DWORD = 4 as DWORD
    /** @brief Registry Value Type: DWORD Big Endian */
    @comptime public const REG_DWORD_BIG_ENDIAN : DWORD = 5 as DWORD
    /** @brief Registry Value Type: Link */
    @comptime public const REG_LINK : DWORD = 6 as DWORD
    /** @brief Registry Value Type: Multi-String */
    @comptime public const REG_MULTI_SZ : DWORD = 7 as DWORD
    /** @brief Registry Value Type: Resource List */
    @comptime public const REG_RESOURCE_LIST : DWORD = 8 as DWORD
    /** @brief Registry Value Type: QWORD */
    @comptime public const REG_QWORD : DWORD = 11 as DWORD

    /**
     * @struct LARGE_INTEGER
     * @brief 64-bit signed integer value. Can be accessed as a whole or parts.
     */
    public union LARGE_INTEGER { // Changed to union based on typical use
        struct {
            var LowPart : DWORD
            var HighPart : LONG
        }
        var QuadPart : LONGLONG
    }

    /**
     * @struct ULARGE_INTEGER
     * @brief 64-bit unsigned integer value. Can be accessed as a whole or parts.
     */
    public union ULARGE_INTEGER {
        struct {
            var LowPart : DWORD
            var HighPart : DWORD
        }
        var QuadPart : ULONGLONG
    }


    /**
     * @struct SECURITY_ATTRIBUTES
     * @brief Security attributes for object creation.
     */
    public struct SECURITY_ATTRIBUTES {
        var nLength : DWORD        /**< Size of this structure. */
        var lpSecurityDescriptor : LPVOID  /**< Security descriptor (opaque). */
        var bInheritHandle : BOOL  /**< Handle inheritance flag. */
    }

    /**
     * @struct OVERLAPPED
     * @brief Overlapped I/O structure.
     */
    public struct OVERLAPPED {
        var Internal : ULONG_PTR   /**< Reserved for kernel. */
        var InternalHigh : ULONG_PTR /**< Reserved for kernel. */
        // Alternative union representation common for Offset/Pointer
        union {
            struct {
                var Offset : DWORD         /**< File position low. */
                var OffsetHigh : DWORD     /**< File position high. */
            }
            var Pointer : LPVOID // Can be used instead of Offset/OffsetHigh
        }
        var hEvent : HANDLE        /**< Event handle for completion. */
    }

    /**
     * @struct SYSTEMTIME
     * @brief Date and time in human-readable form.
     */
    public struct SYSTEMTIME {
        var wYear : WORD
        var wMonth : WORD
        var wDayOfWeek : WORD
        var wDay : WORD
        var wHour : WORD
        var wMinute : WORD
        var wSecond : WORD
        var wMilliseconds : WORD
    }

    /**
     * @struct FILETIME
     * @brief 64-bit value representing 100-nanosecond intervals since January 1, 1601.
     */
    public struct FILETIME {
        var dwLowDateTime : DWORD
        var dwHighDateTime : DWORD
    }

    /**
     * @struct WIN32_FIND_DATAA
     * @brief Data returned by FindFirstFileA/FindNextFileA.
     */
    public struct WIN32_FIND_DATAA {
        var dwFileAttributes : DWORD;  /**< File attributes. */
        var ftCreationTime : FILETIME; /**< File creation time. */
        var ftLastAccessTime : FILETIME; /**< Last access time. */
        var ftLastWriteTime : FILETIME; /**< Last write time. */
        var nFileSizeHigh : DWORD;    /**< High-order DWORD of file size. */
        var nFileSizeLow : DWORD;     /**< Low-order DWORD of file size. */
        var dwReserved0 : DWORD;      /**< Reserved; do not use. */
        var dwReserved1 : DWORD;      /**< Reserved; do not use. */
        var cFileName : char[260];    /**< File name (MAX_PATH). */
        var cAlternateFileName : char[14]; /**< Short file name (8.3). */
        // Some definitions might include these, depending on _WIN32_WINNT version
        // var dwFileType : DWORD;       /**< File type. */
        // var dwCreatorType : DWORD;    /**< File creator type. */
        // var wFinderFlags : WORD;      /**< Finder flags. */
    }

    // --- Added missing STARTUPINFOA and PROCESS_INFORMATION structs ---
    /**
     * @struct STARTUPINFOA
     * @brief Specifies window station, desktop, standard handles, and appearance of the main window for a process at creation time.
     */
     public struct STARTUPINFOA {
        var cb: DWORD;                  /**< Size of the structure, in bytes. */
        var lpReserved: LPSTR;          /**< Reserved; must be NULL. */
        var lpDesktop: LPSTR;           /**< Name of the desktop, or NULL for the default desktop. */
        var lpTitle: LPSTR;             /**< Title displayed in the title bar. If NULL, the executable file name is used. */
        var dwX: DWORD;                 /**< X offset of the window corner, in pixels (if dwFlags specifies STARTF_USEPOSITION). */
        var dwY: DWORD;                 /**< Y offset of the window corner, in pixels (if dwFlags specifies STARTF_USEPOSITION). */
        var dwXSize: DWORD;             /**< Window width, in pixels (if dwFlags specifies STARTF_USESIZE). */
        var dwYSize: DWORD;             /**< Window height, in pixels (if dwFlags specifies STARTF_USESIZE). */
        var dwXCountChars: DWORD;       /**< Screen buffer width, in character columns (if dwFlags specifies STARTF_USECOUNTCHARS). */
        var dwYCountChars: DWORD;       /**< Screen buffer height, in character rows (if dwFlags specifies STARTF_USECOUNTCHARS). */
        var dwFillAttribute: DWORD;     /**< Initial text and background colors (if dwFlags specifies STARTF_USEFILLATTRIBUTE). */
        var dwFlags: DWORD;             /**< Bitmask determining which members are used. (e.g., STARTF_USESTDHANDLES) */
        var wShowWindow: WORD;          /**< How the window is to be shown (if dwFlags specifies STARTF_USESHOWWINDOW). (e.g., SW_HIDE) */
        var cbReserved2: WORD;          /**< Reserved; must be zero. */
        var lpReserved2: *mut BYTE;     /**< Reserved; must be NULL. */
        var hStdInput: HANDLE;          /**< Standard input handle (if dwFlags specifies STARTF_USESTDHANDLES). */
        var hStdOutput: HANDLE;         /**< Standard output handle (if dwFlags specifies STARTF_USESTDHANDLES). */
        var hStdError: HANDLE;          /**< Standard error handle (if dwFlags specifies STARTF_USESTDHANDLES). */
    }

    /**
     * @struct PROCESS_INFORMATION
     * @brief Contains information about a newly created process and its primary thread.
     */
    public struct PROCESS_INFORMATION {
        var hProcess: HANDLE;           /**< Handle to the newly created process. */
        var hThread: HANDLE;            /**< Handle to the primary thread of the newly created process. */
        var dwProcessId: DWORD;         /**< Identifier for the newly created process. */
        var dwThreadId: DWORD;          /**< Identifier for the primary thread of the newly created process. */
    }


    /**
     * @brief Retrieves the calling thread's last-error code value.
     * @return The calling thread's last-error code.
     */
    @extern
    public func GetLastError() : DWORD

    /**
     * @brief Sets the calling thread's last-error code value.
     * @param dwErrCode The new error code.
     */
    @extern
    public func SetLastError(dwErrCode : DWORD) : void

    /**
     * @brief Creates or opens a file or I/O device.
     * @param lpFileName Path to file or device.
     * @param dwDesiredAccess Access mode flags.
     * @param dwShareMode Share mode flags.
     * @param lpSecurityAttributes Optional security attributes.
     * @param dwCreationDisposition Action to take on files that exist or do not exist.
     * @param dwFlagsAndAttributes File attributes and flags.
     * @param hTemplateFile Handle to template file.
     * @return Handle to the file or device, or INVALID_HANDLE_VALUE on error.
     */
    @extern
    public func CreateFileA(
        lpFileName : LPCSTR,
        dwDesiredAccess : DWORD,
        dwShareMode : DWORD,
        lpSecurityAttributes : *mut SECURITY_ATTRIBUTES,
        dwCreationDisposition : DWORD,
        dwFlagsAndAttributes : DWORD,
        hTemplateFile : HANDLE
    ) : HANDLE

    /**
     * @brief Reads data from a file, starting at the position indicated by the file pointer.
     * @param hFile Handle to file.
     * @param lpBuffer Buffer to receive data.
     * @param nNumberOfBytesToRead Number of bytes to read.
     * @param lpNumberOfBytesRead Out param for number of bytes read.
     * @param lpOverlapped Optional overlapped structure.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func ReadFile(
        hFile : HANDLE,
        lpBuffer : LPVOID,
        nNumberOfBytesToRead : DWORD,
        lpNumberOfBytesRead : *mut DWORD,
        lpOverlapped : *mut OVERLAPPED
    ) : BOOL

    /**
     * @brief Writes data to a file at the position indicated by the file pointer.
     * @param hFile Handle to file.
     * @param lpBuffer Buffer containing data to write.
     * @param nNumberOfBytesToWrite Number of bytes to write.
     * @param lpNumberOfBytesWritten Out param for number of bytes written.
     * @param lpOverlapped Optional overlapped structure.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func WriteFile(
        hFile : HANDLE,
        lpBuffer : LPCVOID,
        nNumberOfBytesToWrite : DWORD,
        lpNumberOfBytesWritten : *mut DWORD,
        lpOverlapped : *mut OVERLAPPED
    ) : BOOL

    /**
     * @brief Closes an open object handle.
     * @param hObject Handle to be closed.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func CloseHandle(hObject : HANDLE) : BOOL

    /**
     * @brief Copies an existing file to a new file.
     * @param lpExistingFileName Name of the existing file.
     * @param lpNewFileName Name of the new file.
     * @param bFailIfExists If TRUE and the new file exists, the function fails. If FALSE and the new file exists, the function overwrites it.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func CopyFileA(
        lpExistingFileName : LPCSTR,
        lpNewFileName : LPCSTR,
        bFailIfExists : BOOL
    ) : BOOL

    /**
     * @brief Moves an existing file or directory, including its children.
     * @param lpExistingFileName The current name of the file or directory.
     * @param lpNewFileName The new name for the file or directory.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func MoveFileA(
        lpExistingFileName : LPCSTR,
        lpNewFileName : LPCSTR
    ) : BOOL

    /**
     * @brief Retrieves the size of the specified file.
     * @param hFile Handle to the file.
     * @param lpFileSize Out param for the file size (64-bit).
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func GetFileSizeEx(
        hFile : HANDLE,
        lpFileSize : *mut LARGE_INTEGER // Using LARGE_INTEGER for 64-bit size
    ) : BOOL

    /**
     * @brief Sets the file pointer of the specified file.
     * @param hFile Handle to the file.
     * @param liDistanceToMove The number of bytes to move the file pointer.
     * @param lpNewFilePointer Optional out param for the new file pointer.
     * @param dwMoveMethod The starting point for the file pointer move (FILE_BEGIN, FILE_CURRENT, FILE_END).
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func SetFilePointerEx(
        hFile : HANDLE,
        liDistanceToMove : LARGE_INTEGER,
        lpNewFilePointer : *mut LARGE_INTEGER,
        dwMoveMethod : DWORD
    ) : BOOL

    /**
     * @brief Retrieves file system attributes for a specified file or directory.
     * @param lpFileName The name of the file or directory.
     * @return The attributes of the file or directory, or INVALID_FILE_ATTRIBUTES on failure.
     */
    @extern
    public func GetFileAttributesA(lpFileName : LPCSTR) : DWORD

    /**
     * @brief Sets the attributes for a file or directory.
     * @param lpFileName The name of the file or directory.
     * @param dwFileAttributes The file attributes to set.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func SetFileAttributesA(
        lpFileName : LPCSTR,
        dwFileAttributes : DWORD
    ) : BOOL


    /**
     * @brief Retrieves the full path and file name of the specified file.
     * @param lpFileName The name of the file.
     * @param nBufferLength The size of the buffer for the path.
     * @param lpBuffer Buffer that receives the null-terminated path.
     * @param lpFilePart Optional out param pointer that receives the address of the file name component in the path.
     * @return Length of the string copied to lpBuffer (excluding null terminator), or 0 on failure.
     */
    @extern
    public func GetFullPathNameA(
        lpFileName : LPCSTR,
        nBufferLength : DWORD,
        lpBuffer : LPSTR,
        lpFilePart : *mut LPSTR // Pointer to pointer to char
    ) : DWORD

    /**
     * @brief Retrieves the current directory for the current process.
     * @param nBufferLength The length of the buffer for the current directory string, in characters.
     * @param lpBuffer Pointer to the buffer that receives the null-terminated current directory string.
     * @return If successful, the return value specifies the length of the string written to the buffer, not including the terminating null character. If fails, returns 0.
     */
    @extern
    public func GetCurrentDirectoryA(
        nBufferLength : DWORD,
        lpBuffer : LPSTR
    ) : DWORD

    /**
     * @brief Changes the current directory for the current process.
     * @param lpPathName The path to the new current directory.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func SetCurrentDirectoryA(lpPathName : LPCSTR) : BOOL


    /**
     * @brief Loads the specified module into the address space of the calling process.
     * @param lpLibFileName Path to the module.
     * @return Handle to the module, or NULL on failure.
     */
    @extern
    public func LoadLibraryA(lpLibFileName : LPCSTR) : HMODULE

    /**
     * @brief Frees the loaded dynamic-link library (DLL) module and, if necessary, decrements its reference count.
     * @param hLibModule A handle to the loaded library module.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func FreeLibrary(hLibModule : HMODULE) : BOOL

    /**
     * @brief Retrieves the address of an exported function or variable from the specified DLL.
     * @param hModule Handle to the DLL module.
     * @param lpProcName Name of the function or variable.
     * @return FARPROC pointer to the function or variable, or NULL on failure.
     */
    @extern
    public func GetProcAddress(hModule : HMODULE, lpProcName : LPCSTR) : FARPROC

    /**
     * @brief Allocates memory in the virtual address space of the calling process.
     * @param lpAddress Desired starting address (optional).
     * @param dwSize Size of the region.
     * @param flAllocationType Allocation type flags.
     * @param flProtect Memory protection flags.
     * @return Pointer to the allocated memory, or NULL on failure.
     */
    @extern
    public func VirtualAlloc(
        lpAddress : LPVOID,
        dwSize : SIZE_T,
        flAllocationType : DWORD,
        flProtect : DWORD
    ) : LPVOID

    /**
     * @brief Frees memory that was allocated by VirtualAlloc.
     * @param lpAddress Base address of the region.
     * @param dwSize Must be 0 if MEM_RELEASE is used.
     * @param dwFreeType Memory free type flags.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func VirtualFree(
        lpAddress : LPVOID,
        dwSize : SIZE_T,
        dwFreeType : DWORD
    ) : BOOL

    /**
     * @brief Causes the calling thread to sleep for a specified interval.
     * @param dwMilliseconds Time-out interval, in milliseconds.
     */
    @extern
    public func Sleep(dwMilliseconds : DWORD) : void

    /**
     * @brief Terminates the calling process and returns an exit code.
     * @param uExitCode The exit code for the process.
     */
    @extern
    public func ExitProcess(uExitCode : UINT) : void

    /**
     * @brief Displays a modal dialog box that contains a system icon, a set of buttons, and a brief application-generated message, optionally including a title.
     * @param hWnd Handle to owner window.
     * @param lpText Message to display.
     * @param lpCaption Dialog box title.
     * @param uType Dialog box style and button flags.
     * @return Button identifier the user clicked.
     */
    @extern
    public func MessageBoxA(
        hWnd : HANDLE,
        lpText : LPCSTR,
        lpCaption : LPCSTR,
        uType : DWORD
    ) : int

    /**
     * @brief Begins a file search.
     * @param lpFileName Path with wildcards.
     * @param lpFindFileData Out param for find data.
     * @return Search handle or INVALID_HANDLE_VALUE on error.
     */
    @extern
    public func FindFirstFileA(lpFileName : LPCSTR, lpFindFileData : *mut WIN32_FIND_DATAA) : HANDLE

    /**
     * @brief Continues a file search.
     * @param hFindFile Handle from FindFirstFileA.
     * @param lpFindFileData Out param for next data.
     * @return Nonzero on success, zero on failure/end.
     */
    @extern
    public func FindNextFileA(hFindFile : HANDLE, lpFindFileData : *mut WIN32_FIND_DATAA) : BOOL

    /**
     * @brief Closes a search handle.
     * @param hFindFile Handle to close.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func FindClose(hFindFile : HANDLE) : BOOL

    /**
     * @brief Creates a directory.
     * @param lpPathName Path to the directory.
     * @param lpSecurityAttributes Optional security attributes.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func CreateDirectoryA(lpPathName : LPCSTR, lpSecurityAttributes : *mut SECURITY_ATTRIBUTES) : BOOL

    /**
     * @brief Removes an existing empty directory.
     * @param lpPathName Path to the directory.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func RemoveDirectoryA(lpPathName : LPCSTR) : BOOL

    /**
     * @brief Deletes an existing file.
     * @param lpFileName Path to the file.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func DeleteFileA(lpFileName : LPCSTR) : BOOL

    /**
     * @brief Retrieves a handle to the specified standard device (standard input, standard output, or standard error).
     * @param nStdHandle The standard device (STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, or STD_ERROR_HANDLE).
     * @return Handle to the specified device, or INVALID_HANDLE_VALUE on error.
     */
    @extern
    public func GetStdHandle(nStdHandle : DWORD) : HANDLE

    /**
     * @brief Retrieves the current system date and time in Coordinated Universal Time (UTC).
     * @param lpSystemTime Pointer to a SYSTEMTIME structure to receive the current system date and time.
     */
    @extern
    public func GetSystemTime(lpSystemTime : *mut SYSTEMTIME) : void

    /**
     * @brief Retrieves the current local date and time.
     * @param lpLocalTime Pointer to a SYSTEMTIME structure to receive the current local date and time.
     */
    @extern
    public func GetLocalTime(lpLocalTime : *mut SYSTEMTIME) : void

    /**
     * @brief Converts a file time to system time format.
     * @param lpFileTime Pointer to a FILETIME structure containing the file time to be converted.
     * @param lpSystemTime Pointer to a SYSTEMTIME structure to receive the converted system time.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func FileTimeToSystemTime(lpFileTime : *FILETIME, lpSystemTime : *mut SYSTEMTIME) : BOOL

    /**
     * @brief Converts a system time to file time format.
     * @param lpSystemTime Pointer to a SYSTEMTIME structure containing the system time to be converted.
     * @param lpFileTime Pointer to a FILETIME structure to receive the converted file time.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func SystemTimeToFileTime(lpSystemTime : *SYSTEMTIME, lpFileTime : *mut FILETIME) : BOOL


    /**
     * @brief Retrieves the process identifier of the calling process.
     * @return The process identifier of the calling process.
     */
    @extern
    public func GetCurrentProcessId() : DWORD

    /**
     * @brief Retrieves the thread identifier of the calling thread.
     * @return The thread identifier of the calling thread.
     */
    @extern
    public func GetCurrentThreadId() : DWORD

    /**
     * @brief Creates a new process and its primary thread.
     * @param lpApplicationName Module name (optional, use NULL and set lpCommandLine).
     * @param lpCommandLine Command line to execute (mutable because the function might modify it).
     * @param lpProcessAttributes Security attributes for the process (optional).
     * @param lpThreadAttributes Security attributes for the thread (optional).
     * @param bInheritHandles If TRUE, handles from the calling process are inherited.
     * @param dwCreationFlags Flags controlling priority class and creation (e.g., CREATE_NEW_CONSOLE).
     * @param lpEnvironment Environment block for the new process (optional).
     * @param lpCurrentDirectory Full path to the current directory for the process (optional).
     * @param lpStartupInfo Pointer to a STARTUPINFOA structure.
     * @param lpProcessInformation Pointer to a PROCESS_INFORMATION structure receiving identification information about the new process.
     * @return Nonzero on success, zero on failure.
     */
    @extern
    public func CreateProcessA(
        lpApplicationName : LPCSTR,
        lpCommandLine : LPSTR, // Note: Can be modified by the function, thus LPSTR not LPCSTR
        lpProcessAttributes : *mut SECURITY_ATTRIBUTES,
        lpThreadAttributes : *mut SECURITY_ATTRIBUTES,
        bInheritHandles : BOOL,
        dwCreationFlags : DWORD,
        lpEnvironment : LPVOID,
        lpCurrentDirectory : LPCSTR,
        lpStartupInfo : *mut STARTUPINFOA,
        lpProcessInformation : *mut PROCESS_INFORMATION
    ) : BOOL


    // --- Added Registry Functions ---

    /**
     * @brief Opens the specified registry key.
     * @param hKey A handle to an open registry key (e.g., HKEY_LOCAL_MACHINE) or predefined handle.
     * @param lpSubKey The name of the registry subkey to be opened.
     * @param ulOptions Reserved, must be zero.
     * @param samDesired A mask that specifies the desired access rights to the key. (e.g., KEY_READ)
     * @param phkResult Out param pointer that receives a handle to the opened key.
     * @return ERROR_SUCCESS (0) if successful, or a nonzero error code.
     */
    @extern
    public func RegOpenKeyExA(
        hKey : HKEY,
        lpSubKey : LPCSTR,
        ulOptions : DWORD,
        samDesired : DWORD, // REGSAM type, often DWORD
        phkResult : *mut HKEY
    ) : LONG // Returns LSTATUS which is typically LONG

    /**
     * @brief Closes a handle to the specified registry key.
     * @param hKey Handle to the open key to be closed.
     * @return ERROR_SUCCESS (0) if successful, or a nonzero error code.
     */
    @extern
    public func RegCloseKey(hKey : HKEY) : LONG

    /**
     * @brief Retrieves the type and data for the specified value name associated with an open registry key.
     * @param hKey Handle to an open registry key.
     * @param lpValueName The name of the registry value. If NULL or empty, retrieves type/data for the key's unnamed/default value.
     * @param lpReserved Reserved, must be NULL.
     * @param lpType Optional out param pointer receiving the value's type code (e.g., REG_SZ).
     * @param lpData Optional out param buffer receiving the value's data.
     * @param lpcbData In/Out param pointer specifying size of lpData buffer / receiving size of data written.
     * @return ERROR_SUCCESS (0) if successful, or a nonzero error code.
     */
    @extern
    public func RegQueryValueExA(
        hKey : HKEY,
        lpValueName : LPCSTR,
        lpReserved : LPDWORD, // Often passed as NULL
        lpType : LPDWORD,
        lpData : LPVOID, // Use LPVOID for BYTE* flexibility
        lpcbData : LPDWORD
    ) : LONG

    /**
     * @brief Sets the data and type for a specified value under a registry key.
     * @param hKey Handle to an open registry key.
     * @param lpValueName The name of the value to be set.
     * @param Reserved Reserved, must be zero.
     * @param dwType The type of data to be stored (e.g., REG_SZ).
     * @param lpData The data to be stored.
     * @param cbData The size of the information stored in lpData, in bytes.
     * @return ERROR_SUCCESS (0) if successful, or a nonzero error code.
     */
     @extern
    public func RegSetValueExA(
        hKey : HKEY,
        lpValueName : LPCSTR,
        Reserved : DWORD,
        dwType : DWORD,
        lpData : *BYTE, // Pointer to BYTE for data buffer
        cbData : DWORD
    ) : LONG

    // --- Added CRT functions already present, kept for completeness ---

    /**
     * @brief Creates a directory (C runtime).
     * @param path Path to the directory (ANSI string).
     * @return 0 on success, -1 on failure (sets errno).
     */
    @extern
    public func _mkdir(path : LPCSTR) : int

    /** @brief Removes a directory (CRT).
     * @param path Path to the directory (ANSI string).
     * @return 0 on success, -1 on failure (sets errno).
     */
    @extern
    public func _rmdir(path : LPCSTR) : int

    /** @brief Changes the current working directory (CRT).
     * @param path New working directory (ANSI string).
     * @return 0 on success, -1 on failure (sets errno).
     */
    @extern
    public func _chdir(path : LPCSTR) : int

    /** @brief Gets the current working directory (CRT).
     * @param buffer Buffer to receive path (ANSI string).
     * @param maxlen Maximum length of the buffer.
     * @return Pointer to buffer on success, NULL on failure.
     */
    @extern
    public func _getcwd(buffer : LPSTR, maxlen : int) : LPSTR

}
