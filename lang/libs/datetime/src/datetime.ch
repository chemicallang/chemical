public namespace datetime {

    // -----------------------------------------------------------------------
    // DateTime – a date and time with timezone awareness
    // -----------------------------------------------------------------------
    public struct DateTime {

        var year : i64
        var month : i64    // 1-12
        var day : i64      // 1-31
        var hour : i64     // 0-23
        var minute : i64   // 0-59
        var second : i64   // 0-59
        var nanos : i64    // 0-999_999_999
        var tz : TimeZone

        // ---- Constructors ---------------------------------------------------

        @constructor
        public func init() : DateTime {
            return DateTime {
                year : 1970, month : 1, day : 1,
                hour : 0, minute : 0, second : 0, nanos : 0,
                tz : TimeZone::utc()
            }
        }

        public func from_components(
            year : i64, month : i64, day : i64,
            hour : i64, minute : i64, second : i64, nanos : i64,
            tz : TimeZone
        ) : DateTime {
            return DateTime {
                year : year, month : month, day : day,
                hour : hour, minute : minute, second : second, nanos : nanos,
                tz : tz
            }
        }

        // Convert SystemTime to DateTime (UTC)
        public func from_system_time(st : &std::chrono::SystemTime) : DateTime {
            return from_system_time_tz(st, TimeZone::utc())
        }

        // Convert SystemTime to DateTime at given timezone
        public func from_system_time_tz(st : &std::chrono::SystemTime, tz : TimeZone) : DateTime {
            var epoch_secs = st.as_unix_epoch_secs()
            var st_nanos = st.as_unix_epoch_nanos() - epoch_secs * 1000000000
            var local_secs = epoch_secs + tz.offset_secs
            return from_unix_components(local_secs, st_nanos, tz)
        }

        // Convert DateTime to SystemTime (UTC)
        public func to_system_time(&self) : std::chrono::SystemTime {
            var days = days_from_civil(self.year, self.month, self.day)
            var total_secs = days * 86400 + self.hour * 3600 + self.minute * 60 + self.second
            total_secs = total_secs - self.tz.offset_secs
            return std::chrono::SystemTime::from_unix_epoch_nanos(total_secs, self.nanos)
        }

        // ---- Accessors ------------------------------------------------------

        public func year_val(&self) : i64 { return self.year }
        public func month_val(&self) : i64 { return self.month }
        public func day_val(&self) : i64 { return self.day }
        public func hour_val(&self) : i64 { return self.hour }
        public func minute_val(&self) : i64 { return self.minute }
        public func second_val(&self) : i64 { return self.second }
        public func subsec_nanos(&self) : i64 { return self.nanos }
        public func timezone(&self) : TimeZone { return self.tz }

        // ---- Calendar queries -----------------------------------------------

        public func day_of_week_val(&self) : i64 {
            return datetime::day_of_week(self.year, self.month, self.day)
        }

        public func day_of_year_val(&self) : i64 {
            return datetime::day_of_year(self.year, self.month, self.day)
        }

        public func is_leap_year_val(&self) : bool {
            return datetime::is_leap_year(self.year)
        }

        // ---- Formatting -----------------------------------------------------

        public func format(&self, fmt : std::string_view) : std::string {
            var result = std::string()
            var i : size_t = 0
            var len = fmt.size()
            while(i < len) {
                var c = fmt.get(i)
                if(c == '%' && i + 1 < len) {
                    i = i + 1
                    var spec = fmt.get(i)
                    if(spec == 'Y') {
                        result.append_integer(self.year as bigint)
                    } else if(spec == 'm') {
                        if(self.month < 10) result.append('0')
                        result.append_integer(self.month as bigint)
                    } else if(spec == 'd') {
                        if(self.day < 10) result.append('0')
                        result.append_integer(self.day as bigint)
                    } else if(spec == 'H') {
                        if(self.hour < 10) result.append('0')
                        result.append_integer(self.hour as bigint)
                    } else if(spec == 'M') {
                        if(self.minute < 10) result.append('0')
                        result.append_integer(self.minute as bigint)
                    } else if(spec == 'S') {
                        if(self.second < 10) result.append('0')
                        result.append_integer(self.second as bigint)
                    } else if(spec == 'f') {
                        var n = self.nanos
                        var divisor : i64 = 100000000
                        while(divisor > 0) {
                            result.append_integer((n / divisor) as bigint)
                            n = n % divisor
                            divisor = divisor / 10
                        }
                    } else if(spec == 'z') {
                        var utc_offset = self.tz.format_utc_offset()
                        result.append_view(utc_offset.to_view())
                    } else if(spec == 'Z') {
                        result.append_view(self.tz.tz_name.to_view())
                    } else if(spec == '%') {
                        result.append('%')
                    } else {
                        result.append('%')
                        result.append(spec)
                    }
                } else {
                    result.append(c)
                }
                i = i + 1
            }
            return result
        }

        // ---- Arithmetic -----------------------------------------------------

        public func add_duration(&self, dur : &std::chrono::Duration) : DateTime {
            var st = self.to_system_time()
            var new_st = st.add_duration(dur)
            return DateTime::from_system_time_tz(new_st, self.tz.copy())
        }

        public func sub_duration(&self, dur : &std::chrono::Duration) : DateTime {
            var st = self.to_system_time()
            var new_st = st.sub_duration(dur)
            return DateTime::from_system_time_tz(new_st, self.tz.copy())
        }

        public func duration_since(&self, other : &DateTime) : std::chrono::Duration {
            var st_self = self.to_system_time()
            var st_other = other.to_system_time()
            return st_self.duration_since(st_other)
        }

        // ---- Comparison -----------------------------------------------------

        public func equals(&self, other : &DateTime) : bool {
            return self.year == other.year &&
                self.month == other.month &&
                self.day == other.day &&
                self.hour == other.hour &&
                self.minute == other.minute &&
                self.second == other.second &&
                self.nanos == other.nanos &&
                self.tz.equals(other.tz)
        }

        // ---- Internal: civil calendar conversions ---------------------------

        // Convert days since epoch (1970-01-01) to year/month/day
        func civil_from_days(days : i64, y : *mut i64, m : *mut i64, d : *mut i64) {
            var z = days + 719468
            var era = if(z >= 0) z else z - 146096
            era = era / 146097
            var doe = z - era * 146097
            var yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365
            *y = yoe + era * 400
            var doy = doe - (365 * yoe + yoe / 4 - yoe / 100)
            var mp = (5 * doy + 2) / 153
            *d = doy - (153 * mp + 2) / 5 + 1
            if(mp < 10) {
                *m = mp + 3
            } else {
                *m = mp - 9
            }
            if(*m <= 2) {
                *y = *y + 1
            }
        }

        // Convert year/month/day to days since epoch (1970-01-01)
        func days_from_civil(y : i64, m : i64, d : i64) : i64 {
            var year = y
            var month = m
            if(month <= 2) {
                year = year - 1
                month = month + 12
            }
            var era = if(year >= 0) year else year - 399
            era = era / 400
            var yoe = year - era * 400
            var doy = (153 * (month - 3) + 2) / 5 + d - 1
            var doe = yoe * 365 + yoe / 4 - yoe / 100 + doy
            var days = era * 146097 + doe - 719468
            return days
        }

        // Internal: create DateTime from unix seconds + nanos + timezone
        func from_unix_components(secs : i64, nanos : i64, tz : TimeZone) : DateTime {
            var days = if(secs >= 0) secs / 86400 else (secs - 86399) / 86400
            var day_secs = secs - days * 86400
            if(day_secs < 0) {
                day_secs = day_secs + 86400
                days = days - 1
            }
            var y : i64 = 0
            var m : i64 = 0
            var d : i64 = 0
            civil_from_days(days, &mut y, &mut m, &mut d)
            var hour = day_secs / 3600
            var minute = (day_secs % 3600) / 60
            var second = day_secs % 60
            return DateTime {
                year : y, month : m, day : d,
                hour : hour, minute : minute, second : second, nanos : nanos,
                tz : tz
            }
        }

    }

}
