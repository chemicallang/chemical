if(!def.windows) {

/**
 * @struct stat
 * @brief File status information.
 */
@extern
public struct stat {
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

public enum FileTypeBits : uint {

    /** @def S_IFMT
     *  Bitmask for the file type bitfields.
     */
    Mask = 0xF000,

    /** @def S_IFDIR
     *  Directory.
     */
    Directory = 0x4000,

    /** @def S_IFREG
     *  Regular file.
     */
    Regular = 0x8000,

    /** @def S_IFSOCK
     *  Socket.
     */
    Socket = 0xC000,

    /** @def S_IFLNK
     *  Symbolic link.
     */
    Symlink = 0xA000,

    /** @def S_IFBLK
     *  Block device.
     */
    BlockDev = 0x6000,

    /** @def S_IFCHR
     *  Character device.
     */
    CharDev = 0x2000,

    /** @def S_IFIFO
     *  FIFO (named pipe).
     */
    FIFO = 0x1000,

}

/** TODO failing
public enum FilePerm : uint {
    // Owner
    OwnerRead  = 0o400,
    OwnerWrite = 0o200,
    OwnerExec  = 0o100,
    // Group
    GroupRead  = 0o040,
    GroupWrite = 0o020,
    GroupExec  = 0o010,
    // Others
    OtherRead  = 0o004,
    OtherWrite = 0o002,
    OtherExec  = 0o001,

    // Special bits
    SetUID = 0o4000,
    SetGID = 0o2000,
    Sticky = 0o1000,
}
**/

/** @brief Test for directory. */
// TODO public alias for  #define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

public func is_directory(mode : uint) : bool {
    return (mode & FileTypeBits.Mask) == FileTypeBits.Directory
}

/** @brief Test for regular file. */
// TODO public alias for #define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)

public func is_regular(mode : uint) : bool {
    return (mode & FileTypeBits.Mask) == FileTypeBits.Regular
}

/**
 * @brief Get file status.
 * @param pathname Path to the file.
 * @param buf Pointer to a stat struct to fill.
 * @return 0 on success, –1 on error.
 */
@extern
public func stat(pathname : *char, buf : *mut stat) : int

@extern
public func fstat(fd : int, buf : *mut stat) : int

@extern
public func lstat(pathname : *char, buf : *mut stat) : int

}