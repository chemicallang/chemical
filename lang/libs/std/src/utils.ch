public namespace std {
/**
 * Compatibility alias for `std::mem.replace`.
 */
public func <T> replace(value : &mut T, repl : T) : T {
    return mem.replace(value, repl)
}

/** Compatibility alias for `std::mem.swap`. */
public func <T> swap(a : &mut T, b : &mut T) {
    mem.swap(a, b)
}

public comptime const NPOS = (0 as size_t) - 1;

func internal_view_find(me : &std::string_view, needle : &std::string_view) : size_t {
    const hay = me.data();
    const hay_len = me.size();

    const nd = needle.data();
    const nlen = needle.size();

    // edge cases
    if(nlen == 0) {
        return 0;
    }
    if(nlen > hay_len) {
        return NPOS;
    }

    // fast path: single char
    if(nlen == 1) {
        const c = nd[0];
        var i : size_t = 0;
        while(i < hay_len) {
            if(hay[i] == c) {
                return i;
            }
            i = i + 1;
        }
        return NPOS;
    }

    // ---- Boyer–Moore–Horspool ----

    var skip : [256]uchar;

    // default skip = needle length
    var i : int = 0;
    while(i < 256) {
        skip[i] = nlen as uchar;
        i = i + 1;
    }

    // fill skip table (ignore last char)
    var j : size_t = 0;
    while(j < nlen - 1) {
        skip[(nd[j] as u8) as uint] = (nlen - 1 - j) as uchar;
        j = j + 1;
    }

    var pos : size_t = 0;
    const last = nlen - 1;

    while(pos <= hay_len - nlen) {
        var k : size_t = last;

        // compare backwards
        while(hay[pos + k] == nd[k]) {
            if(k == 0) {
                return pos;
            }
            k = k - 1;
        }

        pos = pos + skip[(hay[pos + last] as u8) as uint];
    }

    return NPOS;
}

func internal_view_find_last(me : &std::string_view, needle : &std::string_view) : size_t {
    const hay = me.data();
    const hay_len = me.size();
    const nd = needle.data();
    const nlen = needle.size();

    // Edge cases
    if (nlen == 0) {
        return hay_len; // Standard behavior: empty string is found at the very end
    }
    if (nlen > hay_len) {
        return NPOS;
    }

    // Start at the furthest possible valid index
    var i : int = (hay_len - nlen) as int;

    while (i >= 0) {
        var match = true;
        var j : size_t = 0;

        // Check for substring match
        while (j < nlen) {
            if (hay[i as size_t + j] != nd[j]) {
                match = false;
                break;
            }
            j = j + 1;
        }

        if (match) {
            return i as size_t;
        }

        i = i - 1;
    }

    return NPOS;
}

}