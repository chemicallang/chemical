import "./common/std_types.ch"
import "./common/time_types.ch"

/**
 * Computes difference between two calendar times as time_t objects (time_end - time_beg) in seconds. If time_end refers to time point before time_beg then the result is negative.
 * @param time_beg, time_end	-	times to compare
 * @return Difference between two times in seconds.
 * @see Difference between two times in seconds.
 */
public func difftime(time_end : time_t, time_beg : time_t) : double

/**
 * Returns the current calendar time encoded as a time_t object, and also stores it in the time_t object pointed to by arg (unless arg is a null pointer)
 * @param arg	-	pointer to a time_t object where the time will be stored, or a null pointer
 * @return Current calendar time encoded as time_t object on success, (time_t)(-1) on error. If arg is not a null pointer, the return value is also stored in the object pointed to by arg.
 * @see https://en.cppreference.com/w/c/chrono/time
 */
public func time(arg : *mut time_t) : time_t

/**
 * Returns the approximate processor time used by the process since the beginning of an implementation-defined era related to the program's execution. To convert result value to seconds, divide it by CLOCKS_PER_SEC.
 * Only the difference between two values returned by different calls to clock is meaningful, as the beginning of the clock era does not have to coincide with the start of the program. clock time may advance faster or slower than the wall clock, depending on the execution resources given to the program by the operating system. For example, if the CPU is shared by other processes, clock time may advance slower than wall clock. On the other hand, if the current process is multithreaded and more than one execution core is available, clock time may advance faster than wall clock.
 * @return Processor time used by the program so far or (clock_t)(-1) if that information is unavailable or its value cannot be represented.
 * @see https://en.cppreference.com/w/c/chrono/clock
 */
public func clock() : clock_t

/**
 * Modifies the timespec object pointed to by ts to hold the current calendar time in the time base base.
 * Other macro constants beginning with TIME_ may be provided by the implementation to indicate additional time bases
 *  If base is TIME_UTC, then
 *      ts->tv_sec is set to the number of seconds since an implementation defined epoch, truncated to a whole value
 *      ts->tv_nsec member is set to the integral number of nanoseconds, rounded to the resolution of the system clock
 * @param ts	-	pointer to an object of type struct timespec
 * @param base	-	TIME_UTC or another nonzero integer value indicating the time base
 * @return The value of base if successful, zero otherwise.
 * @see https://en.cppreference.com/w/c/chrono/timespec_get
 */
public func timespec_get(ts : *mut timespec, base : int) : int

/**
 * TODO macro TIME_UTC
 * #define TIME_UTC
 * @see https://en.cppreference.com/w/c/chrono/timespec_get
 * Expands to a value suitable for use as the base argument of timespec_get
 */


/**
 * If ts is non-null and base is supported by timespec_get, modifies *ts to hold the resolution of time provided by timespec_get for base. For each supported base, multiple calls to timespec_getres during the same program execution have identical results.
 * @param ts	-	pointer to an object of type struct timespec
 * @param base	-	TIME_UTC or another nonzero integer value indicating the time base
 * @return The value of base if base is supported, zero otherwise.
 * @see https://en.cppreference.com/w/c/chrono/timespec_getres
 */
public func timespec_getres(ts : *mut timespec, base : int) : int

/**
 * Converts the date and time information from a given calendar time tp to a null-terminated multibyte character string str according to format string format. Up to count bytes are written.
 * @param str	-	pointer to the first element of the char array for output
 * @param count	-	maximum number of bytes to write
 * @param format	-	pointer to a null-terminated multibyte character string specifying the format of conversion
 * @param tp	-	pointer to a struct tm object specifying the time to format
 * @return The number of bytes written into the character array pointed to by str not including the terminating '\0' on success. If count was reached before the entire string could be stored, ​0​ is returned and the contents are indeterminate.
 * @see https://en.cppreference.com/w/c/chrono/strftime
 */
public func strftime(str : *mut char, count : size_t, format : *char, tp : *tm) : size_t

/**
 * Converts given time since epoch (a time_t value pointed to by timer) into calendar time, expressed in Coordinated Universal Time (UTC) in the struct tm format. The result is stored in static storage and a pointer to that static storage is returned.
 * @param timer	-	pointer to a time_t object to convert
 * @return pointer to a static internal tm object on success, or null pointer otherwise. The structure may be shared between gmtime, localtime, and ctime and may be overwritten on each invocation.
 * @see https://en.cppreference.com/w/c/chrono/gmtime
 */
public func gmtime(timer : *time_t) : *mut tm

/**
 * Same as gmtime, except that the function uses user-provided storage buf for the result.
 * @param timer	-	pointer to a time_t object to convert
 * @param buf	-	pointer to a struct tm object to store the result
 * @return copy of the buf pointer, or null pointer on error (which may be a runtime constraint violation or a failure to convert the specified time to UTC).
 * @see https://en.cppreference.com/w/c/chrono/gmtime
 */
public func gmtime_r(timer : *time_t, buf : *mut tm) : *mut tm

/**
 * Same as (1), except that the function uses user-provided storage buf for the result and that the following errors are detected at runtime and call the currently installed constraint handler function:
 *   timer or buf is a null pointer
 * As with all bounds-checked functions, gmtime_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <time.h>.
 * @param timer	-	pointer to a time_t object to convert
 * @param buf	-	pointer to a struct tm object to store the result
 * @return copy of the buf pointer, or null pointer on error (which may be a runtime constraint violation or a failure to convert the specified time to UTC).
 * @see https://en.cppreference.com/w/c/chrono/gmtime
 */
public func gmtime_s(timer : *time_t, buf : *mut tm) : *mut tm

/**
 * Converts given time since epoch (a time_t value pointed to by timer) into calendar time, expressed in local time, in the struct tm format. The result is stored in static storage and a pointer to that static storage is returned.
 * @param timer	-	pointer to a time_t object to convert
 * @param buf	-	pointer to a struct tm object to store the result
 * @return pointer to a static internal tm object on success, or null pointer otherwise. The structure may be shared between gmtime, localtime, and ctime and may be overwritten on each invocation.
 * @see https://en.cppreference.com/w/c/chrono/localtime
 */
public func localtime(timer : *time_t) : *mut tm

/**
 * Same as (1), except that the function uses user-provided storage buf for the result.
 * @param timer	-	pointer to a time_t object to convert
 * @param buf	-	pointer to a struct tm object to store the result
 * @return copy of the buf pointer, or null pointer on error (which may be a runtime constraint violation or a failure to convert the specified time to local calendar time).
 * @see https://en.cppreference.com/w/c/chrono/localtime
 */
public func localtime_r(timer : *time_t, buf : *mut tm) : *mut tm

/**
 * Same as (1), except that the function uses user-provided storage buf for the result and that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      timer or buf is a null pointer
 * As with all bounds-checked functions, localtime_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <time.h>.
 * @param timer	-	pointer to a time_t object to convert
 * @param buf	-	pointer to a struct tm object to store the result
 * @return copy of the buf pointer, or null pointer on error (which may be a runtime constraint violation or a failure to convert the specified time to local calendar time).
 * @see https://en.cppreference.com/w/c/chrono/localtime
 */
public func localtime_s(timer : *time_t, buf : *mut tm) : *mut tm

/**
 * Renormalizes local calendar time expressed as a struct tm object and also converts it to time since epoch as a time_t object. arg->tm_wday and arg->tm_yday are ignored. The values in arg are not checked for being out of range.
 * A negative value of arg->tm_isdst causes mktime to attempt to determine if Daylight Saving Time was in effect in the specified time.
 * If the conversion to time_t is successful, the arg object is modified. All fields of arg are updated to fit their proper ranges. arg->tm_wday and arg->tm_yday are recalculated using information available in other fields.
 * @param arg	-	pointer to a tm object specifying local calendar time to convert
 * @return time since epoch as a time_t object on success, or -1 if arg cannot be represented as a time_t object (POSIX also requires EOVERFLOW to be stored in errno in this case).
 * @see time since epoch as a time_t object on success, or -1 if arg cannot be represented as a time_t object (POSIX also requires EOVERFLOW to be stored in errno in this case).
 */
public func mktime(arg : *mut tm) : time_t

/**
 * TODO macro CLOCKS_PER_SEC
 * #define CLOCKS_PER_SEC
 * @see https://en.cppreference.com/w/c/chrono/CLOCKS_PER_SEC
 */

