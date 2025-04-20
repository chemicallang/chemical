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

if(def.windows) {

    public enum FileTypeBits : uint {

        /**
         * Bitmask for the file type bitfields.
         */
        Mask = _S_IFMT,
        /**
         *  Directory.
         */
        Directory = _S_IFDIR,
        /**
         *  Regular file.
         */
        Regular = _S_IFREG,
        /**
         *  Character device.
         */
        CharDev = _S_IFCHR,
        /**
         *  FIFO (named pipe).
         */
        FIFO = _S_IFIFO,

    };

} else {

public type off_t = long

// TODO we're not sure about these three comptime constants

// Socket.
@comptime public const _S_IFSOCK = 0xC000

// Symbolic link.
@comptime public const _S_IFLNK = 0xA000

// Block device.
@comptime public const _S_IFBLK = 0x6000

/**
 * @enum FileTypeBits
 * @brief Bit flags for Unix file types, as used in st_mode from struct Stat.
 *        Values are based on POSIX definitions (from <sys/stat.h>).
 */
public enum FileTypeBits : uint {

    /**
     * @def Mask
     * Bitmask for the file type bitfields (alias for _S_IFMT).
     */
    Mask = _S_IFMT,

    /**
     * @def Socket
     * Socket file type (alias for _S_IFSOCK).
     */
    Socket = _S_IFSOCK,

    /**
     * @def Symlink
     * Symbolic link file type (alias for _S_IFLNK).
     */
    Symlink = _S_IFLNK,

    /**
     * @def Regular
     * Regular file type (alias for _S_IFREG).
     */
    Regular = _S_IFREG,

    /**
     * @def BlockDev
     * Block device file type (alias for _S_IFBLK).
     */
    BlockDev = _S_IFBLK,

    /**
     * @def Directory
     * Directory file type (alias for _S_IFDIR).
     */
    Directory = _S_IFDIR,

    /**
     * @def CharDev
     * Character device file type (alias for _S_IFCHR).
     */
    CharDev = _S_IFCHR,

    /**
     * @def FIFO
     * FIFO (named pipe) file type (alias for _S_IFIFO).
     */
    FIFO = _S_IFIFO,
}

/**
 * @brief Test for directory.
 * #define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
 */
public func is_directory(mode : uint) : bool {
    return (mode & FileTypeBits.Mask) == FileTypeBits.Directory
}

/**
 * @brief Test for directory.
 * #define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
 */
public func S_ISDIR(mode : uint) : bool {
    return (mode & FileTypeBits.Mask) == FileTypeBits.Directory
}

/**
 * @brief Test for regular file.
 * #define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
 */
public func is_regular(mode : uint) : bool {
    return (mode & FileTypeBits.Mask) == FileTypeBits.Regular
}

/**
 * @brief Test for regular file.
 * #define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
 */
public func S_ISREG(mode : uint) : bool {
    return (mode & FileTypeBits.Mask) == FileTypeBits.Regular
}

@extern
public func lstat(pathname : *char, buf : *mut Stat) : int

}