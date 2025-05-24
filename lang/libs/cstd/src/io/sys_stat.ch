/**
 * @struct Stat
 * @brief File status information.
 */
public struct Stat {
    var st_dev : ulong;     /**< Device ID */
    var st_ino : ulong;     /**< Inode number */
    var st_mode : uint;    /**< File mode (permissions + type) */
    var st_nlink : uint;   /**< Number of hard links */
    var st_rdev : ulong;    /**< Device ID (if special file) */
    var st_size : off_t;    /**< Total size, in bytes */
    var st_blksize : long; /**< Block size for filesystem I/O */
    var st_blocks : long;  /**< Number of 512B blocks allocated */
    var st_atime : long;   /**< Time of last access */
    var st_mtime : long;   /**< Time of last modification */
    var st_ctime : long;   /**< Time of last status change */
};

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
@comptime public const _S_IFMT = 0xF000
// Directory
@comptime public const _S_IFDIR = 0x4000
// Character special
@comptime public const _S_IFCHR = 0x2000
// Pipe
@comptime public const _S_IFIFO = 0x1000
// Regular
@comptime public const _S_IFREG = 0x8000

@comptime public const S_IFMT = _S_IFMT
@comptime public const S_IFDIR = _S_IFDIR
@comptime public const S_IFCHR = _S_IFCHR
@comptime public const S_IFREG = _S_IFREG