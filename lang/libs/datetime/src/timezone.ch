public namespace datetime {

    // -----------------------------------------------------------------------
    // TimeZone – represents a fixed offset from UTC
    // -----------------------------------------------------------------------
    public struct TimeZone {

        var offset_secs : i64   // seconds east of UTC (-86400 .. +86400)
        var tz_name : std::string  // e.g. "UTC", "America/New_York", "+05:30"

        @constructor
        public func init() : TimeZone {
            return TimeZone { offset_secs : 0, tz_name : std::string("UTC") }
        }

        func copy(&self) : TimeZone {
            return TimeZone { offset_secs : offset_secs, tz_name : tz_name.copy() }
        }

        public func fixed(offset_secs : i64, name : std::string_view) : TimeZone {
            var s = std::string()
            s.append_view(name)
            return TimeZone { offset_secs : offset_secs, tz_name : s }
        }

        public func utc() : TimeZone {
            return TimeZone { offset_secs : 0, tz_name : std::string("UTC") }
        }

        public func local() : TimeZone {
            return TimeZone::utc()
        }

        public func offset_hours(&self) : i64 {
            return self.offset_secs / 3600
        }

        public func offset_minutes(&self) : i64 {
            var rem = self.offset_secs % 3600
            if(rem < 0) rem = -rem
            return rem / 60
        }

        public func equals(&self, other : &TimeZone) : bool {
            return self.offset_secs == other.offset_secs && self.tz_name.equals(other.tz_name)
        }

        public func format_utc_offset(&self) : std::string {
            var result = std::string()
            var total_secs = self.offset_secs
            var sign = '+'
            if(total_secs < 0) {
                sign = '-'
                total_secs = -total_secs
            }
            var hours = total_secs / 3600
            var mins = (total_secs % 3600) / 60
            result.append(sign)
            if(hours < 10) result.append('0')
            result.append_integer(hours as bigint)
            result.append(':')
            if(mins < 10) result.append('0')
            result.append_integer(mins as bigint)
            return result
        }

    }

}
