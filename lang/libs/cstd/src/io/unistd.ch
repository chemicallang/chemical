if(!def.windows) {

public enum OpenFlags : int {

    /** @def O_RDONLY Open for reading only. */
    ReadOnly = 0b00000000,

    /** @def O_WRONLY Open for writing only. */
    WriteOnly = 0b00000001,

    /** @def O_RDWR   Open for reading and writing. */
    ReadWrite   = 0b00000010,

    /** @def O_CREAT  Create file if it does not exist. */
    Create  = 0b01000000,   // 0100 octal = 0x40 = 64

    /** @def O_TRUNC  Truncate file to zero length. */
    Truncate  = 0b1000000000, // 01000 octal = 0x200 = 512

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

}