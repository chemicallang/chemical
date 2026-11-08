using namespace std;

public namespace uuid {

    // A UUID parse or randomness failure. Typed so callers can react without
    // string matching.
    public variant UuidError {
        RandomSourceFailed()
        InvalidLength(expected : size_t, actual : size_t)
        InvalidHyphen(position : size_t)
        InvalidHex(position : size_t, character : char)

        public func message(&self) : std::string {
            switch(self) {
                RandomSourceFailed() => {
                    return std::string("failed to obtain secure random bytes")
                }
                InvalidLength(expected, actual) => {
                    var s = std::string("UUID string must be exactly ")
                    s.append_uinteger(expected as ubigint)
                    s.append_view(" characters long (got ")
                    s.append_uinteger(actual as ubigint)
                    s.append(')')
                    return s
                }
                InvalidHyphen(position) => {
                    var s = std::string("UUID must have a hyphen at position ")
                    s.append_uinteger(position as ubigint)
                    return s
                }
                InvalidHex(position, character) => {
                    var s = std::string("Invalid hex character '")
                    s.append(character)
                    s.append_view("' at index ")
                    s.append_uinteger(position as ubigint)
                    return s
                }
                default => {
                    return std::string("UUID error")
                }
            }
        }
    }

    public struct UUID {
        internal var bytes : [16]u8;

        @constructor
        public func new() {
            return UUID {
                bytes : []
            }
        }

        @constructor
        public func from_bytes(b : [16]u8) {
            var u = UUID();
            for(var i = 0u; i < 16u; i++) {
                u.bytes[i] = b[i];
            }
            return u;
        }

        // Owning copy of the 16 raw bytes (legacy form, kept for callers that
        // need an independent buffer).
        public func to_bytes(&self) : std::vector<u8> {
            var v = std::vector<u8>();
            v.reserve(16u);
            for(var i = 0u; i < 16u; i++) {
                v.push(bytes[i]);
            }
            return v;
        }

        // Non-allocating borrowed view of the 16 raw bytes. The view is valid
        // for as long as `self`.
        public func as_bytes(&self) : [u8] {
            return unsafe(std::span<u8>(&raw self.bytes[0], 16u));
        }


        public func equals(&self, other : &UUID) : bool {
            for(var i = 0u; i < 16u; i++) {
                if(bytes[i] != other.bytes[i]) return false;
            }
            return true;
        }

        public func compare(&self, other : &UUID) : int {
            for(var i = 0u; i < 16u; i++) {
                if(bytes[i] < other.bytes[i]) return -1;
                if(bytes[i] > other.bytes[i]) return 1;
            }
            return 0;
        }

        public func format_to(&self, s : &mut std::string) {
            s.clear();
            s.reserve(36u);
            const hex_chars = "0123456789abcdef";
            for(var i = 0u; i < 16u; i++) {
                if(i == 4u || i == 6u || i == 8u || i == 10u) {
                    s.append('-');
                }
                var b = bytes[i];
                s.append(hex_chars[(b >> 4u) as uint]);
                s.append(hex_chars[(b & 0x0Fu) as uint]);
            }
        }

        public func to_string(&self) : std::string {
            var s = std::string();
            format_to(&mut s);
            return s;
        }
    }

    func hex_to_val(c : char) : int {
        if(c >= '0' && c <= '9') return (c - '0') as int;
        if(c >= 'a' && c <= 'f') return (c - 'a' + 10) as int;
        if(c >= 'A' && c <= 'F') return (c - 'A' + 10) as int;
        return -1;
    }

    public func parse(str : std::string_view) : std::Result<UUID, UuidError> {
        if(str.size() != 36u) {
            return std::Result.Err<UUID, UuidError>(UuidError.InvalidLength(36u, str.size()));
        }
        if(str.get(8) != '-') { return std::Result.Err<UUID, UuidError>(UuidError.InvalidHyphen(8u)) }
        if(str.get(13) != '-') { return std::Result.Err<UUID, UuidError>(UuidError.InvalidHyphen(13u)) }
        if(str.get(18) != '-') { return std::Result.Err<UUID, UuidError>(UuidError.InvalidHyphen(18u)) }
        if(str.get(23) != '-') { return std::Result.Err<UUID, UuidError>(UuidError.InvalidHyphen(23u)) }

        var bytes : [16]u8;
        var byte_idx = 0;

        for(var i = 0u; i < 36u; i++) {
            if(i == 8u || i == 13u || i == 18u || i == 23u) {
                continue;
            }

            var high_char = str.get(i);
            var low_char = str.get(i + 1u);
            i++;

            var high = hex_to_val(high_char);
            if(high < 0) {
                return std::Result.Err<UUID, UuidError>(UuidError.InvalidHex(i - 1u, high_char));
            }

            var low = hex_to_val(low_char);
            if(low < 0) {
                return std::Result.Err<UUID, UuidError>(UuidError.InvalidHex(i, low_char));
            }

            bytes[byte_idx] = ((high << 4) | low) as u8;
            byte_idx++;
        }

        return std::Result.Ok(UUID.from_bytes(bytes));
    }

    func get_random_bytes(buf : *mut u8, len : uint) : bool {
        return unsafe(osrand::raw::random_fill(buf, len as size_t)) == 0
    }

    // Fallible UUIDv4 generation: the OS entropy source can fail, so the
    // failure is reported instead of panicking or returning a sentinel.
    public func v4() : std::Result<UUID, UuidError> {
        var bytes : [16]u8;
        if(!get_random_bytes(&raw mut bytes[0], 16u)) {
            return std::Result.Err<UUID, UuidError>(UuidError.RandomSourceFailed());
        }

        bytes[6] = (0x40u | (bytes[6] & 0x0Fu)) as u8;
        bytes[8] = (0x80u | (bytes[8] & 0x3Fu)) as u8;

        return std::Result.Ok<UUID, UuidError>(UUID.from_bytes(bytes));
    }

    var g_v7_lock : u32 = 0;
    var g_v7_last_ts_ms : u64 = 0;
    var g_v7_counter : u16 = 0;

    public func v7() : std::Result<UUID, UuidError> {
        unsafe {
            var ts_ms = std::now_milli() as u64;

            var expected : u32 = 0;
            while(!atomic_compare_exchange_strong_u32(&raw mut g_v7_lock, &raw mut expected, 1u, memory_order.acquire, memory_order.relaxed)) {
                expected = 0;
            }

            if (ts_ms <= g_v7_last_ts_ms) {
                ts_ms = g_v7_last_ts_ms;
                g_v7_counter++;
                if (g_v7_counter > 0x0FFFu) {
                    ts_ms++;
                    g_v7_counter = 0;
                }
            } else {
                var rand_val : u16 = 0;
                if(!get_random_bytes(&mut rand_val as *mut u8, 2u)) {
                    atomic_store_u32(&raw mut g_v7_lock, 0u, memory_order.release);
                    return std::Result.Err<UUID, UuidError>(UuidError.RandomSourceFailed());
                }
                g_v7_counter = rand_val & 0x0FFFu;
            }
            g_v7_last_ts_ms = ts_ms;
            var local_counter = g_v7_counter;

            atomic_store_u32(&raw mut g_v7_lock, 0u, memory_order.release);

            var bytes : [16]u8;
            if(!get_random_bytes(&raw mut bytes[0], 16u)) {
                return std::Result.Err<UUID, UuidError>(UuidError.RandomSourceFailed());
            }

            bytes[0] = ((ts_ms >> 40) & 0xFFu) as u8;
            bytes[1] = ((ts_ms >> 32) & 0xFFu) as u8;
            bytes[2] = ((ts_ms >> 24) & 0xFFu) as u8;
            bytes[3] = ((ts_ms >> 16) & 0xFFu) as u8;
            bytes[4] = ((ts_ms >> 8) & 0xFFu) as u8;
            bytes[5] = (ts_ms & 0xFFu) as u8;

            bytes[6] = (0x70u | ((local_counter >> 8) & 0x0Fu) as u32) as u8;
            bytes[7] = (local_counter & 0xFFu) as u8;

            bytes[8] = (0x80u | (bytes[8] & 0x3Fu)) as u8;

            return std::Result.Ok<UUID, UuidError>(UUID.from_bytes(bytes));
    
        }
    }

impl Copy for UUID {}
}
