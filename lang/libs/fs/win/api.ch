public namespace fs {

public comptime const FILE_SHARE_READ =                 0x00000001
public comptime const FILE_SHARE_WRITE =                0x00000002
public comptime const FILE_SHARE_DELETE =               0x00000004
public comptime const FILE_ATTRIBUTE_READONLY =             0x00000001
public comptime const FILE_ATTRIBUTE_HIDDEN =               0x00000002
public comptime const FILE_ATTRIBUTE_SYSTEM =               0x00000004
public comptime const FILE_ATTRIBUTE_DIRECTORY =            0x00000010
public comptime const FILE_ATTRIBUTE_ARCHIVE =              0x00000020
public comptime const FILE_ATTRIBUTE_DEVICE =               0x00000040
public comptime const FILE_ATTRIBUTE_NORMAL =               0x00000080
public comptime const FILE_ATTRIBUTE_TEMPORARY =            0x00000100
public comptime const FILE_ATTRIBUTE_SPARSE_FILE =          0x00000200
public comptime const FILE_ATTRIBUTE_REPARSE_POINT =        0x00000400
public comptime const FILE_ATTRIBUTE_COMPRESSED =           0x00000800
public comptime const FILE_ATTRIBUTE_OFFLINE =              0x00001000
public comptime const FILE_ATTRIBUTE_NOT_CONTENT_INDEXED =  0x00002000
public comptime const FILE_ATTRIBUTE_ENCRYPTED =            0x00004000
public comptime const FILE_ATTRIBUTE_INTEGRITY_STREAM =     0x00008000
public comptime const FILE_ATTRIBUTE_VIRTUAL =              0x00010000
public comptime const FILE_ATTRIBUTE_NO_SCRUB_DATA =        0x00020000
public comptime const FILE_ATTRIBUTE_EA =                   0x00040000
public comptime const FILE_ATTRIBUTE_PINNED =               0x00080000
public comptime const FILE_ATTRIBUTE_UNPINNED =             0x00100000
public comptime const FILE_ATTRIBUTE_RECALL_ON_OPEN =       0x00040000
public comptime const FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS = 0x00400000
public comptime const TREE_CONNECT_ATTRIBUTE_PRIVACY =      0x00004000
public comptime const TREE_CONNECT_ATTRIBUTE_INTEGRITY =    0x00008000
public comptime const TREE_CONNECT_ATTRIBUTE_GLOBAL =       0x00000004
public comptime const TREE_CONNECT_ATTRIBUTE_PINNED =       0x00000002
public comptime const FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL =  0x20000000
public comptime const FILE_NOTIFY_CHANGE_FILE_NAME =    0x00000001
public comptime const FILE_NOTIFY_CHANGE_DIR_NAME =     0x00000002
public comptime const FILE_NOTIFY_CHANGE_ATTRIBUTES =   0x00000004
public comptime const FILE_NOTIFY_CHANGE_SIZE =         0x00000008
public comptime const FILE_NOTIFY_CHANGE_LAST_WRITE =   0x00000010
public comptime const FILE_NOTIFY_CHANGE_LAST_ACCESS =  0x00000020
public comptime const FILE_NOTIFY_CHANGE_CREATION =     0x00000040
public comptime const FILE_NOTIFY_CHANGE_SECURITY =     0x00000100
public comptime const FILE_ACTION_ADDED =                   0x00000001
public comptime const FILE_ACTION_REMOVED =                 0x00000002
public comptime const FILE_ACTION_MODIFIED =                0x00000003
public comptime const FILE_ACTION_RENAMED_OLD_NAME =        0x00000004
public comptime const FILE_ACTION_RENAMED_NEW_NAME =        0x00000005
public comptime const MAILSLOT_NO_MESSAGE =                 -1 as DWORD
public comptime const MAILSLOT_WAIT_FOREVER =               -1 as DWORD
public comptime const FILE_CASE_SENSITIVE_SEARCH =          0x00000001
public comptime const FILE_CASE_PRESERVED_NAMES =           0x00000002
public comptime const FILE_UNICODE_ON_DISK =                0x00000004
public comptime const FILE_PERSISTENT_ACLS =                0x00000008
public comptime const FILE_FILE_COMPRESSION =               0x00000010
public comptime const FILE_VOLUME_QUOTAS =                  0x00000020
public comptime const FILE_SUPPORTS_SPARSE_FILES =          0x00000040
public comptime const FILE_SUPPORTS_REPARSE_POINTS =        0x00000080
public comptime const FILE_SUPPORTS_REMOTE_STORAGE =        0x00000100
public comptime const FILE_RETURNS_CLEANUP_RESULT_INFO =    0x00000200
public comptime const FILE_SUPPORTS_POSIX_UNLINK_RENAME =   0x00000400
public comptime const FILE_SUPPORTS_BYPASS_IO =             0x00000800
public comptime const FILE_SUPPORTS_STREAM_SNAPSHOTS =      0x00001000
public comptime const FILE_SUPPORTS_CASE_SENSITIVE_DIRS =   0x00002000

public comptime const FILE_VOLUME_IS_COMPRESSED =           0x00008000
public comptime const FILE_SUPPORTS_OBJECT_IDS =            0x00010000
public comptime const FILE_SUPPORTS_ENCRYPTION =            0x00020000
public comptime const FILE_NAMED_STREAMS =                  0x00040000
public comptime const FILE_READ_ONLY_VOLUME =               0x00080000
public comptime const FILE_SEQUENTIAL_WRITE_ONCE =          0x00100000
public comptime const FILE_SUPPORTS_TRANSACTIONS =          0x00200000
public comptime const FILE_SUPPORTS_HARD_LINKS =            0x00400000
public comptime const FILE_SUPPORTS_EXTENDED_ATTRIBUTES =   0x00800000
public comptime const FILE_SUPPORTS_OPEN_BY_FILE_ID =       0x01000000
public comptime const FILE_SUPPORTS_USN_JOURNAL =           0x02000000
public comptime const FILE_SUPPORTS_INTEGRITY_STREAMS =     0x04000000
public comptime const FILE_SUPPORTS_BLOCK_REFCOUNTING =     0x08000000
public comptime const FILE_SUPPORTS_SPARSE_VDL =            0x10000000
public comptime const FILE_DAX_VOLUME =                     0x20000000
public comptime const FILE_SUPPORTS_GHOSTING =              0x40000000



public union _ULARGE_INTEGER {
    struct {
        var LowPart : DWORD;
        var HighPart : DWORD;
    } DUMMYSTRUCTNAME;
    struct {
        var LowPart : DWORD;
        var HighPart : DWORD;
    } u;
    var QuadPart : ULONGLONG;
};

public type ULARGE_INTEGER = _ULARGE_INTEGER
public type PULARGE_INTEGER = *mut ULARGE_INTEGER;

@dllimport
@stdcall
@extern
public func GetFullPathNameW(
    lpFileName : LPCWSTR,
    nBufferLength : DWORD,
    lpBuffer : LPWSTR,
    lpFilePart : LPWSTR*
) : DWORD

@dllimport
@stdcall
@extern
public func CreateFileW(
    lpFileName : LPCWSTR,
    dwDesiredAccess : DWORD,
    dwShareMode : DWORD,
    lpSecurityAttributes : LPSECURITY_ATTRIBUTES,
    dwCreationDisposition : DWORD,
    dwFlagsAndAttributes : DWORD,
    hTemplateFile : HANDLE
) : HANDLE

@dllimport
@stdcall
@extern
public func FlushFileBuffers(
    hFile : HANDLE
) : BOOL

@dllimport
@stdcall
@extern
public func GetDiskFreeSpaceExW(
    lpDirectoryName : LPCWSTR,
    lpFreeBytesAvailableToCaller : PULARGE_INTEGER,
    lpTotalNumberOfBytes : PULARGE_INTEGER,
    lpTotalNumberOfFreeBytes : PULARGE_INTEGER
) : BOOL

@dllimport
@stdcall
@extern
public func LockFileEx(
    hFile : HANDLE,
    dwFlags : DWORD,
    dwReserved : DWORD,
    nNumberOfBytesToLockLow : DWORD,
    nNumberOfBytesToLockHigh : DWORD,
    lpOverlapped : LPOVERLAPPED
) : BOOL

@dllimport
@stdcall
@extern
public func SetEndOfFile(
    hFile : HANDLE
) : BOOL


@dllimport
@stdcall
@extern
public func SetFileAttributesW(
    lpFileName : LPCWSTR,
    dwFileAttributes : DWORD
) : BOOL

@dllimport
@stdcall
@extern
public func SetFileTime(
    hFile : HANDLE,
    lpCreationTime : *FILETIME,
    lpLastAccessTime : *FILETIME,
    lpLastWriteTime : *FILETIME
) : BOOL

@dllimport
@stdcall
@extern
public func CreateDirectoryW(
    lpPathName : LPCWSTR,
    lpSecurityAttributes : LPSECURITY_ATTRIBUTES
) : BOOL

public struct _WIN32_FILE_ATTRIBUTE_DATA {
    var dwFileAttributes : DWORD;
    var ftCreationTime : FILETIME;
    var ftLastAccessTime : FILETIME;
    var ftLastWriteTime : FILETIME;
    var nFileSizeHigh : DWORD;
    var nFileSizeLow : DWORD;
};

public type WIN32_FILE_ATTRIBUTE_DATA = _WIN32_FILE_ATTRIBUTE_DATA
public type LPWIN32_FILE_ATTRIBUTE_DATA = *mut _WIN32_FILE_ATTRIBUTE_DATA

enum GET_FILEEX_INFO_LEVELS {
    GetFileExInfoStandard,
    GetFileExMaxInfoLevel
}

@dllimport
@stdcall
@extern
public func GetFileAttributesExW(
    lpFileName : LPCWSTR,
    fInfoLevelId : GET_FILEEX_INFO_LEVELS,
    lpFileInformation : LPVOID
) : BOOL;

@dllimport
@stdcall
@extern
public func GetTempFileNameW(
    lpPathName : LPCWSTR,
    lpPrefixString : LPCWSTR,
    uUnique : UINT,
    lpTempFileName : LPWSTR
) : UINT


@dllimport
@stdcall
@extern
public func RemoveDirectoryW(
    lpPathName : LPCWSTR
) : BOOL

comptime const MAX_PATH =          260

struct _WIN32_FIND_DATAW {
    var dwFileAttributes : DWORD;
    var ftCreationTime : FILETIME;
    var ftLastAccessTime : FILETIME;
    var ftLastWriteTime : FILETIME;
    var nFileSizeHigh : DWORD;
    var nFileSizeLow : DWORD;
    var dwReserved0 : DWORD;
    var dwReserved1 : DWORD;
    var cFileName : WCHAR[MAX_PATH];
    var cAlternateFileName : WCHAR[14];
}

public type WIN32_FIND_DATAW = _WIN32_FIND_DATAW
public type PWIN32_FIND_DATAW = *mut _WIN32_FIND_DATAW
public type LPWIN32_FIND_DATAW = *mut _WIN32_FIND_DATAW

@dllimport
@stdcall
@extern
public func FindFirstFileW(
     lpFileName : LPCWSTR,
     lpFindFileData : LPWIN32_FIND_DATAW
) : HANDLE

comptime const LOCKFILE_FAIL_IMMEDIATELY =  0x00000001
comptime const LOCKFILE_EXCLUSIVE_LOCK =     0x00000002

@stdcall
@dllimport
@extern
public func FindNextFileW(
    hFindFile : HANDLE,
    lpFindFileData : LPWIN32_FIND_DATAW
) : BOOL

@stdcall
@dllimport
@extern
public func DeleteFileW(
    lpFileName : LPCWSTR
) : BOOL

@dllimport
@stdcall
@extern
public func GetFinalPathNameByHandleW(
    hFile : HANDLE,
    lpszFilePath : LPWSTR,
    cchFilePath : DWORD,
    dwFlags : DWORD
) : DWORD

@dllimport
@stdcall
@extern
public func GetTempPathW(
    nBufferLength : DWORD,
    lpBuffer : LPWSTR
) : DWORD

//
// MessageId: ERROR_FILE_NOT_FOUND
//
// MessageText:
//
// The system cannot find the file specified.
//
public comptime const ERROR_FILE_NOT_FOUND = 2L

@dllimport
@stdcall
@extern
public func CopyFileW(
    lpExistingFileName : LPCWSTR,
    lpNewFileName : LPCWSTR,
    bFailIfExists : BOOL
) : BOOL

@dllimport
@stdcall
@extern
public func CreateHardLinkW(
    lpFileName : LPCWSTR,
    lpExistingFileName : LPCWSTR,
    lpSecurityAttributes : LPSECURITY_ATTRIBUTES
) : BOOL

@dllimport
@stdcall
@extern
public func CreateSymbolicLinkW(
    lpSymlinkFileName : LPCWSTR,
    lpTargetFileName : LPCWSTR,
    dwFlags : DWORD
) : BOOL

//
//  These are flags supported through CreateFile (W7) and CreateFile2 (W8 and beyond)
//

comptime const FILE_FLAG_WRITE_THROUGH =         0x80000000
comptime const FILE_FLAG_OVERLAPPED =            0x40000000
comptime const FILE_FLAG_NO_BUFFERING =          0x20000000
comptime const FILE_FLAG_RANDOM_ACCESS =         0x10000000
comptime const FILE_FLAG_SEQUENTIAL_SCAN =       0x08000000
comptime const FILE_FLAG_DELETE_ON_CLOSE =       0x04000000
comptime const FILE_FLAG_BACKUP_SEMANTICS =      0x02000000
comptime const FILE_FLAG_POSIX_SEMANTICS =       0x01000000
comptime const FILE_FLAG_SESSION_AWARE =         0x00800000
comptime const FILE_FLAG_OPEN_REPARSE_POINT =    0x00200000
comptime const FILE_FLAG_OPEN_NO_RECALL =        0x00100000
comptime const FILE_FLAG_FIRST_PIPE_INSTANCE =   0x00080000

}