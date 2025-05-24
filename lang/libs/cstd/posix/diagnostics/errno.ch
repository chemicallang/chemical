/**
 * @brief Error number set by system calls on failure.
 */
@extern
public var errno : int;

/**
 * @brief Return human‑readable string for an error code.
 * @param errnum Error number.
 * @return Pointer to descriptive, null‑terminated string.
 */
@extern
public func strerror(errnum : int) : *mut char