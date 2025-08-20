/* NULL-terminated array of "NAME=VALUE" environment variables.  */
@extern
public var __environ : **mut char;

if(def.gnu) {
    @extern
    public var environ : **mut char;
}

/** @def O_RDONLY Open for reading only. */
public comptime const O_RDONLY = 0b00000000

/** @def O_WRONLY Open for writing only. */
public comptime const O_WRONLY = 0b00000001

/** @def O_RDWR   Open for reading and writing. */
public comptime const O_RDWR = 0b00000010

/** @def O_CREAT  Create file if it does not exist. */
public comptime const O_CREAT = 0b01000000

/** @def O_TRUNC  Truncate file to zero length. */
public comptime const O_TRUNC = 0b1000000000

public enum OpenFlags : int {

    /** @def O_RDONLY Open for reading only. */
    ReadOnly = O_RDONLY,

    /** @def O_WRONLY Open for writing only. */
    WriteOnly = O_WRONLY,

    /** @def O_RDWR   Open for reading and writing. */
    ReadWrite = O_RDWR,

    /** @def O_CREAT  Create file if it does not exist. */
    Create = O_CREAT,   // 0100 octal = 0x40 = 64

    /** @def O_TRUNC  Truncate file to zero length. */
    Truncate = O_TRUNC, // 01000 octal = 0x200 = 512

}

public enum PermissionMode : uint {
    S_IRWXU = 0o700,  /* Read, write, and execute/search by owner */
    S_IRUSR = 0o400,  /* Read permission, owner */
    S_IWUSR = 0o200,  /* Write permission, owner */
    S_IXUSR = 0o100,  /* Execute/search permission, owner */
    S_IRWXG = 0o070,  /* Read, write, and execute/search by group */
    S_IRGRP = 0o040,  /* Read permission, group */
    S_IWGRP = 0o020,  /* Write permission, group */
    S_IXGRP = 0o010,  /* Execute/search permission, group */
    S_IRWXO = 0o007,  /* Read, write, and execute/search by others */
    S_IROTH = 0o004,  /* Read permission, others */
    S_IWOTH = 0o002,  /* Write permission, others */
    S_IXOTH = 0o001   /* Execute/search permission, others */
}

/**
 * @brief Open a file.
 * @param pathname Path to the file.
 * @param flags File access flags (O_RDONLY, O_WRONLY, O_RDWR).
 * @param mode Permissions for new file (if O_CREAT).
 * @return File descriptor ≥0 on success, or –1 on error.
 */
@extern
public func open(pathname : *char, flags : int, mode : int) : int

/**
 * @brief Close a file descriptor.
 * @param fd File descriptor to close.
 * @return 0 on success, –1 on error.
 */
@extern
public func close(fd : int) : int

/**
 * @brief Read bytes from a file descriptor.
 * @param fd File descriptor.
 * @param buf Buffer to read into.
 * @param count Maximum number of bytes to read.
 * @return Number of bytes read (≥0), or –1 on error.
 */
@extern
public func read(fd : int, buf : *mut void, count : ulong) : ssize_t

/**
 * @brief Write bytes to a file descriptor.
 * @param fd File descriptor.
 * @param buf Buffer containing data to write.
 * @param count Number of bytes to write.
 * @return Number of bytes written (≥0), or –1 on error.
 */
@extern
public func write(fd : int, buf : *void, count : ulong) : ssize_t

/**
 * @brief Create a new directory.
 * @param pathname Directory path to create.
 * @param mode Permissions for the new directory.
 * @return 0 on success, –1 on error.
 */
@extern
public func mkdir(pathname : *char, mode : uint) : int