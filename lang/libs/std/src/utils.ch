public namespace std {
/**
 * safer way to replace and move members, however impacts runtime performance
 */
public func <T> replace(value : &mut T, repl : T) : T {
    var temp : T
    memcpy(&mut temp, &value, sizeof(T))
    memcpy(&mut value, &repl, sizeof(T))
    intrinsics::forget(repl)
    return temp;
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
        skip[nd[j] as uint] = (nlen - 1 - j) as uchar;
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

        pos = pos + skip[hay[pos + last] as uint];
    }

    return NPOS;
}

}