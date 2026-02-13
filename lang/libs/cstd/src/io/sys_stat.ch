// Stat struct declarations for Windows and POSIX (Linux x86_64)
// Use: include this file where you declare fstat, lstat, fstatat usage.
// If you target macOS/BSD, ask and I'll provide a macOS/BSD variant.

// Common typedefs used below
// public type time_t     = i64;
public type dev_t      = u64;
public type ino_t      = u64;
public type mode_t     = u32;
public type nlink_t    = u64;
public type uid_t      = u32;
public type gid_t      = u32;
/**
 * @typedef off_t
 * File offset type, used for file sizes and positions.
 */
public type off_t      = i64;
public type blksize_t  = i64;
public type fsblkcnt_t = u64;
public type fsfilcnt_t = u64;

if(def.windows) {
    // -----------------------
    // Windows: _stat64-like layout (MSVCRT)
    // -----------------------
    // This is compatible with the common _stat64 structure returned by _stat64 / _fstat64.
    // It is intended for use with MSVCRT-style stat wrappers; on native Win32 API you
    // typically use WIN32_FIND_DATAW or GetFileInformationByHandle instead.
    public struct Stat {
        var st_dev    : dev_t;     // ID of device containing file
        var st_ino    : ino_t;     // file serial number (may be 0 on Windows)
        var st_mode   : mode_t;    // protection (file mode)
        var st_nlink  : nlink_t;   // number of hard links
        var st_uid    : uid_t;     // user ID of owner (not meaningful on Windows)
        var st_gid    : gid_t;     // group ID of owner (not meaningful on Windows)
        var st_rdev   : dev_t;     // device ID (if special file)
        var st_size   : off_t;     // total size, in bytes
        var st_atime  : time_t;    // time of last access
        var st_mtime  : time_t;    // time of last modification
        var st_ctime  : time_t;    // time of last status change (on Windows: creation time in some APIs)
    }
} else {

    public type __dev_t = ulong
    public type __ino_t = ulong
    public type __nlink_t = ulong
    public type __mode_t = uint
    public type __uid_t = uint
    public type __gid_t = uint
    public type __off_t = long
    public type __blksize_t = long
    public type __blkcnt_t = long
    public type __syscall_slong_t = long
    public type __time_t = long

    public struct Stat {
        var st_dev : __dev_t;		/* Device.  */
        var st_ino : __ino_t;		/* File serial number.	*
        var st_nlink : __nlink_t;		/* Link count.  */
        var st_mode : __mode_t;		/* File mode.  */
        var st_uid : __uid_t;		/* User ID of the file's owner.	*/
        var st_gid : __gid_t;		/* Group ID of the file's group.*/
        var __pad0 : int;
        var st_rdev : __dev_t;		/* Device number, if device.  */
        var st_size : __off_t;			/* Size of file, in bytes.  */
        var st_blksize : __blksize_t;	/* Optimal block size for I/O.  */
        var st_blocks : __blkcnt_t;		/* Number 512-byte blocks allocated. */
        /* Nanosecond resolution timestamps are stored in a format
           equivalent to 'struct timespec'.  This is the type used
           whenever possible but the Unix namespace rules do not allow the
           identifier 'timespec' to appear in the <sys/stat.h> header.
           Therefore we have to handle the use of this header in strictly
           standard-compliant sources special.  */
        var st_atime : timespec;		/* Time of last access.  */
        var st_mtime : timespec;		/* Time of last modification.  */
        var st_ctime : timespec;		/* Time of last status change.  */
        var __glibc_reserved : __syscall_slong_t[3];
    }

}

/**
 * @brief Get file status.
 * @param pathname Path to the file.
 * @param buf Pointer to a Stat struct to fill.
 * @return 0 on success, –1 on error.
 */
@extern
public func stat(pathname : *char, buf : *mut Stat) : int

@extern
public func fstat(fd : int, buf : *mut Stat) : int

// File type mask
public comptime const _S_IFMT = 0xF000
// Directory
public comptime const _S_IFDIR = 0x4000
// Character special
public comptime const _S_IFCHR = 0x2000
// Pipe
public comptime const _S_IFIFO = 0x1000
// Regular
public comptime const _S_IFREG = 0x8000

public comptime const S_IFMT = _S_IFMT
public comptime const S_IFDIR = _S_IFDIR
public comptime const S_IFCHR = _S_IFCHR
public comptime const S_IFREG = _S_IFREG