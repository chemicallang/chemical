public namespace datetime {

    // -----------------------------------------------------------------------
    // Calendar utilities
    // -----------------------------------------------------------------------

    public func is_leap_year(year : i64) : bool {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)
    }

    public func days_in_month(year : i64, month : i64) : i64 {
        if(month == 2) {
            return if(is_leap_year(year)) 29 else 28
        }
        // Months with 31 days: Jan(1), Mar(3), May(5), Jul(7), Aug(8), Oct(10), Dec(12)
        var m31 = if(month <= 7) (month % 2 == 1) else (month % 2 == 0)
        return if(m31) 31 else 30
    }

    // Tomohiko Sakamoto's algorithm for day of week
    // Returns 0=Sun, 1=Mon, ..., 6=Sat
    public func day_of_week(year : i64, month : i64, day : i64) : i64 {
        var y = year
        var m = month
        if(m < 3) {
            y = y - 1
            m = m + 12
        }
        var t = [0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4]
        return (y + y/4 - y/100 + y/400 + t[m - 1] + day) % 7
    }

    public func day_of_year(year : i64, month : i64, day : i64) : i64 {
        var days_before = [0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334]
        var doy = days_before[month - 1] + day
        if(month > 2 && is_leap_year(year)) {
            doy = doy + 1
        }
        return doy
    }

    // Validate date components
    public func is_valid_date(year : i64, month : i64, day : i64) : bool {
        if(month < 1 || month > 12) return false
        if(day < 1 || day > days_in_month(year, month)) return false
        return true
    }

    public func is_valid_time(hour : i64, minute : i64, second : i64, nanos : i64) : bool {
        if(hour < 0 || hour > 23) return false
        if(minute < 0 || minute > 59) return false
        if(second < 0 || second > 59) return false
        if(nanos < 0 || nanos > 999999999) return false
        return true
    }

}
