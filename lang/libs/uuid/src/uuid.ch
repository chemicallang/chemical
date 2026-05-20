using namespace std;

public namespace uuid {

    public struct UUID {
        public var bytes : [16]u8;

        @constructor
        public func new() {
            return UUID {
                bytes : []
            }
        }

        @constructor
        public func from_bytes(b : [16]u8) {
            return UUID {
                bytes : b
            }
        }

        public func to_bytes(&self) : std::vector<u8> {
            var v = std::vector<u8>();
            v.reserve(16u);
            for(var i = 0u; i < 16u; i++) {
                v.push(bytes[i]);
            }
            return v;
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
            format_to(s);
            return s;
        }
    }

    func hex_to_val(c : char) : int {
        if(c >= '0' && c <= '9') return (c - '0') as int;
        if(c >= 'a' && c <= 'f') return (c - 'a' + 10) as int;
        if(c >= 'A' && c <= 'F') return (c - 'A' + 10) as int;
        return -1;
    }

    public func parse(str : std::string_view) : std::Result<UUID, std::string> {
        if(str.size() != 36u) {
            return std::Result.Err(std::string("UUID string must be exactly 36 characters long"));
        }
        if(str.get(8) != '-' || str.get(13) != '-' || str.get(18) != '-' || str.get(23) != '-') {
            return std::Result.Err(std::string("UUID must have hyphens at positions 8, 13, 18, and 23"));
        }

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
                var err = std::string("Invalid hex character '");
                err.append(high_char);
                err.append_view("' at index ");
                err.append_uinteger((i - 1u) as ubigint);
                return std::Result.Err(err);
            }

            var low = hex_to_val(low_char);
            if(low < 0) {
                var err = std::string("Invalid hex character '");
                err.append(low_char);
                err.append_view("' at index ");
                err.append_uinteger(i as ubigint);
                return std::Result.Err(err);
            }

            bytes[byte_idx] = ((high << 4) | low) as u8;
            byte_idx++;
        }

        return std::Result.Ok(UUID.from_bytes(bytes));
    }

    if(def.windows) {
        @extern public func CryptAcquireContextA(phProv : *mut usize, pszContainer : *char, pszProvider : *char, dwProvType : uint, dwFlags : uint) : bool
        @extern public func CryptGenRandom(hProv : usize, dwLen : uint, pbBuffer : *mut u8) : bool
        @extern public func CryptReleaseContext(hProv : usize, dwFlags : uint) : bool
    }

    func get_random_bytes(buf : *mut u8, len : uint) : bool {
        comptime if(def.windows) {
            var hProv : usize = 0u;
            if(!CryptAcquireContextA(&mut hProv, null, null, 1u, 0xF0000000u)) return false;
            var ok = CryptGenRandom(hProv, len, buf);
            CryptReleaseContext(hProv, 0u);
            return ok;
        } else {
            var fd = open("/dev/urandom", O_RDONLY, 0);
            if(fd < 0) return false;
            var bytes_read = read(fd, buf as *mut void, len as ulong);
            close(fd);
            return bytes_read == (len as ssize_t);
        }
    }

    public func v4() : UUID {
        var bytes : [16]u8;
        if(!get_random_bytes(&mut bytes[0], 16u)) {
            panic("Failed to generate secure random bytes for UUIDv4");
        }

        bytes[6] = (0x40u | (bytes[6] & 0x0Fu)) as u8;
        bytes[8] = (0x80u | (bytes[8] & 0x3Fu)) as u8;

        return UUID.from_bytes(bytes);
    }

    var g_v7_lock : u32 = 0;
    var g_v7_last_ts_ms : u64 = 0;
    var g_v7_counter : u16 = 0;

    public func v7() : UUID {
        var ts_ms = std::now_milli() as u64;

        var expected : u32 = 0;
        while(!atomic_compare_exchange_strong_u32(&mut g_v7_lock, &mut expected, 1u, memory_order.acquire, memory_order.relaxed)) {
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
            get_random_bytes(&mut rand_val as *mut u8, 2u);
            g_v7_counter = rand_val & 0x0FFFu;
        }
        g_v7_last_ts_ms = ts_ms;
        var local_counter = g_v7_counter;

        atomic_store_u32(&mut g_v7_lock, 0u, memory_order.release);

        var bytes : [16]u8;
        if(!get_random_bytes(&mut bytes[0], 16u)) {
            panic("Failed to generate secure random bytes for UUIDv7");
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

        return UUID.from_bytes(bytes);
    }
}
