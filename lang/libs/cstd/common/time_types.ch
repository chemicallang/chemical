/**
 * TODO time_t is implementation defined
 * @see https://en.cppreference.com/w/c/chrono/time_t
 */
@extern
public struct time_t {

}

/**
 * TODO clock_t is implementation defined
 * @see https://en.cppreference.com/w/c/chrono/clock_t
 */
@extern
public struct clock_t {

}

/**
 * Structure holding an interval broken down into seconds and nanoseconds.
 */
@extern
public struct timespec {

    /**
     * whole seconds (valid values are >= 0)
     *
     */
    var tv_sec : time_t

    /**
     * nanoseconds (valid values are [0, 999999999])
     * TODO the type of tv_nsec is long long on some platforms
     */
    var tv_nsec : long

}

/**
 * Structure holding a calendar date and time broken down into its components.
 */
@extern
public struct tm {
    /**
     * seconds after the minute – [0, 61](until C99)[0, 60](since C99)[note 1]
     */
    var sec : int
    /**
     * minutes after the hour – [0, 59]
     */
    var min : int
    /**
     * hours since midnight – [0, 23]
     */
    var hour : int
    /**
     * day of the month – [1, 31]
     */
    var mday : int
    /**
     * months since January – [0, 11]
     */
    var mon : int
    /**
     * years since 1900
     */
    var year : int
    /**
     * days since Sunday – [0, 6]
     */
    var wday : int
    /**
     * days since January 1 – [0, 365]
     */
    var yday : int
    /**
     * Daylight Saving Time flag. The value is positive if DST is in effect, zero if not and negative if no information is available
     */
    var isdst : int
}