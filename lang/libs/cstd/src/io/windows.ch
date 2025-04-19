if(def.windows) {

/** @brief Windows handle type. */
public type HANDLE = *mut void;
/** @brief Boolean: nonzero is TRUE, zero is FALSE. */
public type BOOL = int;
/** @brief 32‑bit unsigned integer. */
public type DWORD = ulong;
/** @brief Pointer to constant string. */
public type LPCSTR = *char;

/** @def INVALID_HANDLE_VALUE Invalid handle constant. */
@comptime
public const INVALID_HANDLE_VALUE : HANDLE = -1

@comptime
public const FILE_ATTRIBUTE_DIRECTORY = 0x10

/**
 * @struct WIN32_FIND_DATAA
 * @brief Data returned by FindFirstFileA/FindNextFileA.
 */
public struct WIN32_FIND_DATAA {
    var dwFileAttributes : DWORD;  /**< File attributes. */
    var cFileName : char[260];    /**< Name of file found. */
};

/**
 * @brief Begin a file search.
 * @param lpFileName Path with wildcards.
 * @param lpFindFileData Out param for find data.
 * @return Search handle or INVALID_HANDLE_VALUE on error.
 */
public func FindFirstFileA(lpFileName : LPCSTR, lpFindFileData : *mut WIN32_FIND_DATAA) : HANDLE

/**
 * @brief Continue file search.
 * @param hFindFile Handle from FindFirstFileA.
 * @param lpFindFileData Out param for next data.
 * @return Nonzero on success, zero on failure/end.
 */
public func FindNextFileA(hFindFile : HANDLE, lpFindFileData : *mut WIN32_FIND_DATAA) : BOOL

/**
 * @brief Close a search handle.
 * @param hFindFile Handle to close.
 * @return Nonzero on success, zero on failure.
 */
public func FindClose(hFindFile : HANDLE) : BOOL

/**
 * @brief Create a directory.
 * @param lpPathName Path to the directory.
 * @param lpSecurityAttributes Unused; should be NULL.
 * @return Nonzero on success, zero on failure.
 */
public func CreateDirectoryA(lpPathName : *char, lpSecurityAttributes : *mut void) : BOOL

/**
 * @brief Remove a directory.
 * @param lpPathName Path to the directory.
 * @return Nonzero on success, zero on failure.
 */
public func RemoveDirectoryA(lpPathName : *char) : BOOL

/**
 * @brief Delete a file.
 * @param lpFileName Path to the file.
 * @return Nonzero on success, zero on failure.
 */
public func DeleteFileA(lpFileName : *char) : BOOL

}
