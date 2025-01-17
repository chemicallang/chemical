import "../std.ch"

public func fnv1a_hash_32(str : *char) : uint {
    var ptr = str;
    var hash : uint = 2166136261U;
    while (true) {
        const c = *ptr;
        if(c == '\0') {
            return hash
        } else {
            hash ^= c as uchar;
            hash *= 16777619U;
            ptr++;
        }
    }
    return hash;
}

public func fnv1_hash(s : *char) : size_t {
    var ptr = s;
    var hash : size_t = 0xcbf29ce484222325;
    while(true) {
        const c = *ptr;
        if(c == '\0') {
            return hash;
        } else {
            hash ^= c as size_t;
            hash *= 0x100000001b3;
            ptr++
        }
    }
    return hash;
}

@comptime
public func comptime_fnv1a_hash_32(s : *char) : uint {
    return fnv1a_hash_32(s);
}

@comptime
public func comptime_fnv1_hash(s : *char) : size_t {
    return fnv1_hash(s)
}