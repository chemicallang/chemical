/**
 * @struct timeval
 * @brief Time value with second and microsecond resolution.
 */
@extern
public struct timeval {
    var tv_sec : long;   /**< seconds */
    var tv_usec : long;  /**< microseconds */
};

/**
 * @brief Get current time of day.
 * @param tv Pointer to timeval struct to fill.
 * @param tz Unused; should be NULL.
 * @return 0 on success, –1 on error.
 */
@extern
public func gettimeofday(tv : *mut timeval, tz : *mut void) : int