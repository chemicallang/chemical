if (def.windows) {

    // TODO check these three declarations are correct
    public type usize = ubigint
    public type ULONG_PTR = *mut ulong
    public type UINT = uint

    /** @def NULL Pointer to nothing. */
    @comptime public const NULL : *void = null

    /** @brief Windows handle type. */
    public type HANDLE = *mut void
    /** @brief Module handle. */
    public type HMODULE = HANDLE
    /** @brief Instance handle. */
    public type HINSTANCE = HANDLE
    /** @brief Pointer to procedure. */
    public type FARPROC = *mut void

    /** @brief Boolean: nonzero is TRUE, zero is FALSE. */
    public type BOOL = int
    /** @brief Unsigned 8-bit integer. */
    public type BYTE = uchar
    /** @brief 16-bit unsigned integer. */
    public type WORD = ushort
    /** @brief 32-bit unsigned integer. */
    public type DWORD = uint32_t
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

    /** @brief Invalid handle constant. */
    @comptime public const INVALID_HANDLE_VALUE : HANDLE = -1

    /** @brief File attribute: directory. */
    @comptime public const FILE_ATTRIBUTE_DIRECTORY : DWORD = 0x10
    /** @brief File attribute: normal. */
    @comptime public const FILE_ATTRIBUTE_NORMAL : DWORD = 0x80

    /** @brief Desired access: read. */
    @comptime public const GENERIC_READ : DWORD = 0x80000000
    /** @brief Desired access: write. */
    @comptime public const GENERIC_WRITE : DWORD = 0x40000000

    /** @brief Share mode: read. */
    @comptime public const FILE_SHARE_READ : DWORD = 0x00000001
    /** @brief Share mode: write. */
    @comptime public const FILE_SHARE_WRITE : DWORD = 0x00000002

    /** @brief Creation disposition: open existing file. */
    @comptime public const OPEN_EXISTING : DWORD = 3
    /** @brief Creation disposition: create new file. */
    @comptime public const CREATE_NEW : DWORD = 1
    /** @brief Creation disposition: open or create. */
    @comptime public const OPEN_ALWAYS : DWORD = 4
    /** @brief Creation disposition: overwrite existing. */
    @comptime public const CREATE_ALWAYS : DWORD = 2

    /** @brief Memory allocation: commit pages. */
    @comptime public const MEM_COMMIT : DWORD = 0x1000
    /** @brief Memory allocation: reserve address range. */
    @comptime public const MEM_RESERVE : DWORD = 0x2000
    /** @brief Memory free: release pages. */
    @comptime public const MEM_RELEASE : DWORD = 0x8000

    /** @brief Page protection: read/write. */
    @comptime public const PAGE_READWRITE : DWORD = 0x04

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
        var Offset : DWORD         /**< File position low. */
        var OffsetHigh : DWORD     /**< File position high. */
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
     * @brief Retrieves the calling thread's last-error code value.
     * @return The calling thread's last-error code.
     */
    public func GetLastError() : DWORD

    /**
     * @brief Sets the calling thread's last-error code value.
     * @param dwErrCode The new error code.
     */
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
    public func CloseHandle(hObject : HANDLE) : BOOL

    /**
     * @brief Loads the specified module into the address space of the calling process.
     * @param lpLibFileName Path to the module.
     * @return Handle to the module, or NULL on failure.
     */
    public func LoadLibraryA(lpLibFileName : LPCSTR) : HMODULE

    /**
     * @brief Retrieves the address of an exported function or variable from the specified DLL.
     * @param hModule Handle to the DLL module.
     * @param lpProcName Name of the function or variable.
     * @return FARPROC pointer to the function or variable, or NULL on failure.
     */
    public func GetProcAddress(hModule : HMODULE, lpProcName : LPCSTR) : FARPROC

    /**
     * @brief Allocates memory in the virtual address space of the calling process.
     * @param lpAddress Desired starting address (optional).
     * @param dwSize Size of the region.
     * @param flAllocationType Allocation type flags.
     * @param flProtect Memory protection flags.
     * @return Pointer to the allocated memory, or NULL on failure.
     */
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
    public func VirtualFree(
        lpAddress : LPVOID,
        dwSize : SIZE_T,
        dwFreeType : DWORD
    ) : BOOL

    /**
     * @brief Causes the calling thread to sleep for a specified interval.
     * @param dwMilliseconds Time-out interval, in milliseconds.
     */
    public func Sleep(dwMilliseconds : DWORD) : void

    /**
     * @brief Terminates the calling process and returns an exit code.
     * @param uExitCode The exit code for the process.
     */
    public func ExitProcess(uExitCode : UINT) : void

    /**
     * @brief Displays a modal dialog box that contains a system icon, a set of buttons, and a brief application-generated message, optionally including a title.
     * @param hWnd Handle to owner window.
     * @param lpText Message to display.
     * @param lpCaption Dialog box title.
     * @param uType Dialog box style and button flags.
     * @return Button identifier the user clicked.
     */
    public func MessageBoxA(
        hWnd : HANDLE,
        lpText : LPCSTR,
        lpCaption : LPCSTR,
        uType : DWORD
    ) : int

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
        var cFileName : char[260];    /**< File name. */
        var cAlternateFileName : char[14]; /**< Short file name. */
    }

    /**
     * @brief Begins a file search.
     * @param lpFileName Path with wildcards.
     * @param lpFindFileData Out param for find data.
     * @return Search handle or INVALID_HANDLE_VALUE on error.
     */
    public func FindFirstFileA(lpFileName : LPCSTR, lpFindFileData : *mut WIN32_FIND_DATAA) : HANDLE

    /**
     * @brief Continues a file search.
     * @param hFindFile Handle from FindFirstFileA.
     * @param lpFindFileData Out param for next data.
     * @return Nonzero on success, zero on failure/end.
     */
    public func FindNextFileA(hFindFile : HANDLE, lpFindFileData : *mut WIN32_FIND_DATAA) : BOOL

    /**
     * @brief Closes a search handle.
     * @param hFindFile Handle to close.
     * @return Nonzero on success, zero on failure.
     */
    public func FindClose(hFindFile : HANDLE) : BOOL

    /**
     * @brief Creates a directory.
     * @param lpPathName Path to the directory.
     * @param lpSecurityAttributes Unused; should be NULL.
     * @return Nonzero on success, zero on failure.
     */
    public func CreateDirectoryA(lpPathName : LPCSTR, lpSecurityAttributes : *mut SECURITY_ATTRIBUTES) : BOOL

    /**
     * @brief Removes a directory.
     * @param lpPathName Path to the directory.
     * @return Nonzero on success, zero on failure.
     */
    public func RemoveDirectoryA(lpPathName : LPCSTR) : BOOL

    /**
     * @brief Deletes a file.
     * @param lpFileName Path to the file.
     * @return Nonzero on success, zero on failure.
     */
    public func DeleteFileA(lpFileName : LPCSTR) : BOOL

}
