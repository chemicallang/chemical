public type uint32_t = uint
public type uint8_t = uchar

if(def.big_endian) {
    func htole32(value : uint32_t) : uint32_t {
        return (((value & 0xFF000000) >> 24) | ((value & 0x00FF0000) >> 8)  | ((value & 0x0000FF00) << 8)  | ((value & 0x000000FF) << 24));
    }
}

public func murmurhash (key : *char, len : uint32_t, seed : uint32_t) : uint32_t {
    const c1 : uint32_t = 0xcc9e2d51 as uint32_t;
    const c2 : uint32_t = 0x1b873593 as uint32_t;
    const r1 : uint32_t = 15 as uint32_t;
    const r2 : uint32_t = 13 as uint32_t;
    const m : uint32_t = 5 as uint32_t;
    const n : uint32_t = 0xe6546b64 as uint32_t;
    var h : uint32_t = 0 as uint32_t;
    var k : uint32_t = 0 as uint32_t;
    const d : *uint8_t = key as *uint8_t; // 32 bit extract from `key'
    var chunks : *uint32_t = null;
    var tail : *uint8_t = null; // tail - last 8 bytes
    var i = 0;
    var l = len / 4; // chunk length
    h = seed;
    chunks = (d + l * 4) as *uint32_t; // body
    tail = (d + l * 4) as *uint8_t; // last 8 byte chunk of `key'
    // for each 4 byte chunk of `key'
    for (i = -l; i != 0; ++i) {
        // next 4 byte chunk of `key'
        if(def.big_endian) {
            k = htole32(chunks[i]);
        } else {
            k = chunks[i];
        }
        // encode next 4 byte chunk of `key'
        k *= c1;
        k = (k << r1) | (k >> (32 - r1));
        k *= c2;
        // append to hash
        h ^= k;
        h = (h << r2) | (h >> (32 - r2));
        h = h * m + n;
    }
    k = 0;
    // remainder
    switch (len & 3) {
        3 => {
            k ^= (tail[2] << 16);
            k ^= (tail[1] << 8);
            k ^= tail[0];
            k *= c1;
            k = (k << r1) | (k >> (32 - r1));
            k *= c2;
            h ^= k;
        }
        2 => {
            k ^= (tail[1] << 8);
            k ^= tail[0];
            k *= c1;
            k = (k << r1) | (k >> (32 - r1));
            k *= c2;
            h ^= k;
        }
        1 => {
            k ^= tail[0];
            k *= c1;
            k = (k << r1) | (k >> (32 - r1));
            k *= c2;
            h ^= k;
        }
    }
    h ^= len;
    h ^= (h >> 16);
    h *= 0x85ebca6b;
    h ^= (h >> 13);
    h *= 0xc2b2ae35;
    h ^= (h >> 16);
    return h;
}