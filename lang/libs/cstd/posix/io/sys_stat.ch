public type off_t = long

// TODO we're not sure about these three comptime constants

// Socket.
public comptime const _S_IFSOCK = 0xC000

// Symbolic link.
public comptime const _S_IFLNK = 0xA000

// Block device.
public comptime const _S_IFBLK = 0x6000

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