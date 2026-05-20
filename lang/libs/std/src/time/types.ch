// ---------------------------------------------------------------------------
// Duration – a span of time with nanosecond precision.
// ABI: two i64 fields = 16 bytes, POD, C-compatible layout.
// ---------------------------------------------------------------------------
public namespace std {

    public namespace chrono {

        comptime const NANOS_PER_MICRO : i64 = 1000
        comptime const NANOS_PER_MILLI : i64 = 1000000
        comptime const NANOS_PER_SEC   : i64 = 1000000000
        comptime const MICROS_PER_SEC  : i64 = 1000000
        comptime const MILLIS_PER_SEC  : i64 = 1000

        @direct_init
        public struct Duration {

            var secs : i64
            var nanos : i64

            // ---- Constructors -----------------------------------------------

            @constructor
            public func init() : Duration {
                return Duration { secs : 0, nanos : 0 }
            }

            @constructor
            public func from_parts(s : i64, n : i64) : Duration {
                var d = Duration { secs : s, nanos : n }
                d.normalize()
                return d
            }

            public func from_secs(secs : i64) : Duration {
                return Duration { secs : secs, nanos : 0 }
            }

            public func from_millis(ms : i64) : Duration {
                var d = Duration { secs : ms / MILLIS_PER_SEC, nanos : (ms % MILLIS_PER_SEC) * NANOS_PER_MILLI }
                d.normalize()
                return d
            }

            public func from_micros(us : i64) : Duration {
                var d = Duration { secs : us / MICROS_PER_SEC, nanos : (us % MICROS_PER_SEC) * std::chrono::NANOS_PER_MICRO }
                d.normalize()
                return d
            }

            public func from_nanos(ns : i64) : Duration {
                var d = Duration { secs : ns / NANOS_PER_SEC, nanos : ns % NANOS_PER_SEC }
                d.normalize()
                return d
            }

            // ---- Accessors --------------------------------------------------

            public func as_secs(&self) : i64 {
                return self.secs
            }

            public func subsec_nanos(&self) : i64 {
                return self.nanos
            }

            public func as_millis(&self) : i64 {
                return self.secs * MILLIS_PER_SEC + self.nanos / NANOS_PER_MILLI
            }

            public func as_micros(&self) : i64 {
                if(self.secs < 0 && self.nanos > 0) {
                    return (self.secs + 1) * MICROS_PER_SEC - (NANOS_PER_SEC - self.nanos) / NANOS_PER_MICRO
                }
                return self.secs * MICROS_PER_SEC + self.nanos / NANOS_PER_MICRO
            }

            public func as_nanos(&self) : i64 {
                if(self.secs < 0 && self.nanos > 0) {
                    return (self.secs + 1) * NANOS_PER_SEC - (NANOS_PER_SEC - self.nanos)
                }
                return self.secs * NANOS_PER_SEC + self.nanos
            }

            // ---- Normalization ----------------------------------------------

            func normalize(&mut self) {
                if(self.nanos >= NANOS_PER_SEC || self.nanos <= -NANOS_PER_SEC) {
                    var extra = self.nanos / NANOS_PER_SEC
                    self.secs = self.secs + extra
                    self.nanos = self.nanos - extra * NANOS_PER_SEC
                }
                if(self.secs > 0 && self.nanos < 0) {
                    self.secs = self.secs - 1
                    self.nanos = self.nanos + NANOS_PER_SEC
                } else if(self.secs < 0 && self.nanos > 0) {
                    self.secs = self.secs + 1
                    self.nanos = self.nanos - NANOS_PER_SEC
                }
            }

            // ---- Comparison -------------------------------------------------

            public func equals(&self, other : &Duration) : bool {
                return self.secs == other.secs && self.nanos == other.nanos
            }

            public func cmp(&self, other : &Duration) : int {
                if(self.secs < other.secs) return -1
                if(self.secs > other.secs) return 1
                if(self.nanos < other.nanos) return -1
                if(self.nanos > other.nanos) return 1
                return 0
            }

            // ---- Arithmetic -------------------------------------------------

            public func add(&self, other : &Duration) : Duration {
                var d = Duration { secs : self.secs + other.secs, nanos : self.nanos + other.nanos }
                d.normalize()
                return d
            }

            public func sub(&self, other : &Duration) : Duration {
                var d = Duration { secs : self.secs - other.secs, nanos : self.nanos - other.nanos }
                d.normalize()
                return d
            }

            public func neg(&self) : Duration {
                var d = Duration { secs : -self.secs, nanos : -self.nanos }
                d.normalize()
                return d
            }

            public func mul_i64(&self, scalar : i64) : Duration {
                if(scalar == 0) return Duration { secs : 0, nanos : 0 }
                var total_nanos = self.as_nanos()
                var result_nanos = total_nanos * scalar
                return Duration::from_nanos(result_nanos)
            }

            public func div_i64(&self, scalar : i64) : Duration {
                if(scalar == 0) {
                    panic("Duration::div_i64: division by zero")
                }
                var total_nanos = self.as_nanos()
                var result_nanos = total_nanos / scalar
                return Duration::from_nanos(result_nanos)
            }

            public func abs(&self) : Duration {
                if(self.secs < 0) {
                    return self.neg()
                }
                return Duration { secs : self.secs, nanos : self.nanos }
            }

            public func is_zero(&self) : bool {
                return self.secs == 0 && self.nanos == 0
            }

            public func is_positive(&self) : bool {
                return self.secs > 0 || (self.secs == 0 && self.nanos > 0)
            }

            public func is_negative(&self) : bool {
                return self.secs < 0 || (self.secs == 0 && self.nanos < 0)
            }

        }

        // ---- Duration constants (at namespace level) ------------------------
        public comptime const DUR_ZERO        = std::chrono::Duration { secs : 0, nanos : 0 }
        public comptime const DUR_NANOSECOND  = std::chrono::Duration { secs : 0, nanos : 1 }
        public comptime const DUR_MICROSECOND = std::chrono::Duration { secs : 0, nanos : std::chrono::NANOS_PER_MICRO }
        public comptime const DUR_MILLISECOND = std::chrono::Duration { secs : 0, nanos : std::chrono::NANOS_PER_MILLI }
        public comptime const DUR_SECOND      = std::chrono::Duration { secs : 1, nanos : 0 }
        public comptime const DUR_MINUTE      = std::chrono::Duration { secs : 60, nanos : 0 }
        public comptime const DUR_HOUR        = std::chrono::Duration { secs : 3600, nanos : 0 }
        public comptime const DUR_DAY         = std::chrono::Duration { secs : 86400, nanos : 0 }
        public comptime const DUR_WEEK        = std::chrono::Duration { secs : 604800, nanos : 0 }

        // -----------------------------------------------------------------------
        // Instant – a monotonic clock timepoint (always increasing, not related to
        // wall clock). Useful for measuring elapsed time.
        // ABI: two i64 fields = 16 bytes, POD.
        // -----------------------------------------------------------------------
        @direct_init
        public struct Instant {

            var secs : i64
            var nanos : i64

            @constructor
            public func init() : Instant {
                return Instant { secs : 0, nanos : 0 }
            }

            public func now() : Instant {
                var s : i64 = 0
                var n : i64 = 0
                // Call platform-specific monotonic clock
                // now_monotonic is in std namespace, defined in platform time.ch
                now_monotonic(&mut s, &mut n)
                return Instant { secs : s, nanos : n }
            }

            public func duration_since(&self, earlier : &Instant) : Duration {
                var d = Duration::from_parts(self.secs - earlier.secs, self.nanos - earlier.nanos)
                return d
            }

            public func elapsed(&self) : Duration {
                var now_inst = Instant::now()
                return now_inst.duration_since(self)
            }

            public func duration_to(&self, later : &Instant) : Duration {
                return later.duration_since(self)
            }

            public func add_duration(&self, dur : &Duration) : Instant {
                var d = Duration::from_parts(self.secs, self.nanos)
                var sum = d.add(dur)
                return Instant { secs : sum.as_secs(), nanos : sum.subsec_nanos() }
            }

            public func sub_duration(&self, dur : &Duration) : Instant {
                var d = Duration::from_parts(self.secs, self.nanos)
                var diff = d.sub(dur)
                return Instant { secs : diff.as_secs(), nanos : diff.subsec_nanos() }
            }

            public func equals(&self, other : &Instant) : bool {
                return self.secs == other.secs && self.nanos == other.nanos
            }

            public func cmp(&self, other : &Instant) : int {
                if(self.secs < other.secs) return -1
                if(self.secs > other.secs) return 1
                if(self.nanos < other.nanos) return -1
                if(self.nanos > other.nanos) return 1
                return 0
            }

        }

        // -----------------------------------------------------------------------
        // SystemTime – a wall-clock timepoint (UTC). Backed by the platform's
        // realtime clock (CLOCK_REALTIME on POSIX, GetSystemTimeAsFileTime on Win).
        // ABI: two i64 fields = 16 bytes, POD.
        // -----------------------------------------------------------------------
        @direct_init
        public struct SystemTime {

            var secs : i64
            var nanos : i64

            @constructor
            public func init() : SystemTime {
                return SystemTime { secs : 0, nanos : 0 }
            }

            // The Unix Epoch (1970-01-01T00:00:00Z)
            // (provided as a namespace-level constant below)

            public func now() : SystemTime {
                var s : i64 = 0
                var n : i64 = 0
                // Call platform-specific realtime clock
                // now_realtime is in std namespace, defined in platform time.ch
                now_realtime(&mut s, &mut n)
                return SystemTime { secs : s, nanos : n }
            }

            public func from_unix_epoch(secs : i64) : SystemTime {
                return SystemTime { secs : secs, nanos : 0 }
            }

            public func from_unix_epoch_nanos(secs : i64, nanos : i64) : SystemTime {
                var d = Duration::from_parts(secs, nanos)
                return SystemTime { secs : d.as_secs(), nanos : d.subsec_nanos() }
            }

            public func as_unix_epoch_secs(&self) : i64 {
                return self.secs
            }

            public func as_unix_epoch_nanos(&self) : i64 {
                return self.secs * NANOS_PER_SEC + self.nanos
            }

            public func duration_since(&self, earlier : &SystemTime) : Duration {
                var d = Duration::from_parts(self.secs - earlier.secs, self.nanos - earlier.nanos)
                return d
            }

            public func elapsed(&self) : Duration {
                var now_st = SystemTime::now()
                return now_st.duration_since(self)
            }

            public func add_duration(&self, dur : &Duration) : SystemTime {
                var d = Duration::from_parts(self.secs, self.nanos)
                var sum = d.add(dur)
                return SystemTime { secs : sum.as_secs(), nanos : sum.subsec_nanos() }
            }

            public func sub_duration(&self, dur : &Duration) : SystemTime {
                var d = Duration::from_parts(self.secs, self.nanos)
                var diff = d.sub(dur)
                return SystemTime { secs : diff.as_secs(), nanos : diff.subsec_nanos() }
            }

            public func equals(&self, other : &SystemTime) : bool {
                return self.secs == other.secs && self.nanos == other.nanos
            }

        }

        // ---- SystemTime constants -------------------------------------------
        public comptime const UNIX_EPOCH = std::chrono::SystemTime { secs : 0, nanos : 0 }

    }

}
