// cursor_stride.ch — a cursor that advances inside a loop must not be combined
// with the loop index.
//
// WHY THIS EXISTS
//
// A CBOR decoder in `lang/libs/cbor` read bytes with:
//
//     for(var i = 0u64; i < len; i++) {
//         v = (v << 8) | (self.data.get(self.pos + (i as size_t)) as u64)
//         self.pos += 1u
//     }
//
// and produced "IT" for the input "IETF", and -769 instead of -1000 for
// 0x3903e7. It looked like a compiler reordering: the read appeared to see the
// ALREADY-INCREMENTED cursor, so it looked like every second byte was skipped.
//
// It was not a compiler bug. `self.pos` advances AND `i` advances, so
// `self.pos + i` denotes indices 0, 2, 4, 6 — a stride of 2. The compiler read
// exactly the position the source named. The loop was wrong, and the out-of-
// range `get` past the end returned 0, which is what turned a stride bug into
// "IT\0\0" instead of an obvious failure.
//
// So these tests pin the SEMANTICS rather than a fix: a read at `cursor + i`
// must see the index the source denotes, in a struct field and in a local, and
// a mutating cursor must compose with an index the way arithmetic says it does.
// That is a real invariant — if a future codegen change ever reordered the read
// against the increment, every such loop in the language would silently break,
// and this is where it would be caught.
//
// `import core` only: no std, no library. Arrays rather than `vector`, because
// `vector` is std and these are the common tests.

struct StrideCursor {
    var data : [8]u8
    var pos : int
}

// The exact shape that produced "IT": read at `pos + i`, then advance `pos`.
func stride_double_counted() : int {
    var c = StrideCursor { data : [] , pos : 0 }
    // "IETF" = 0x49 0x45 0x54 0x46
    c.data[0] = 0x49
    c.data[1] = 0x45
    c.data[2] = 0x54
    c.data[3] = 0x46
    // Collect what was read, as a nibble-packed value, so one int can carry it.
    var seen = 0
    for(var i = 0; i < 4; i++) {
        seen = (seen << 8) | (c.data[c.pos + i] as int)
        c.pos += 1
    }
    return seen
}

// The correct form: bind the index to a local, or drop `i` entirely.
func stride_cursor_only() : int {
    var c = StrideCursor { data : [] , pos : 0 }
    c.data[0] = 0x49
    c.data[1] = 0x45
    c.data[2] = 0x54
    c.data[3] = 0x46
    var seen = 0
    for(var i = 0; i < 4; i++) {
        seen = (seen << 8) | (c.data[c.pos] as int)
        c.pos += 1
    }
    return seen
}

// The same thing with a plain local cursor instead of a struct field, to show
// it is arithmetic and not something about member access.
//
// The array is [8]u8 for the same reason the struct's is: `pos + i` walks off
// the end after four iterations, and an out-of-bounds read is undefined — a
// [4] array makes this test meaningless rather than testing anything.
func stride_local_cursor() : int {
    var data : [8]u8 = []
    data[0] = 0x49
    data[1] = 0x45
    data[2] = 0x54
    data[3] = 0x46
    var pos = 0
    var seen = 0
    for(var i = 0; i < 4; i++) {
        seen = (seen << 8) | (data[pos + i] as int)
        pos += 1
    }
    return seen
}

// A fixed start plus the index: the other correct way to write it.
func stride_fixed_start() : int {
    var data : [4]u8 = []
    data[0] = 0x49
    data[1] = 0x45
    data[2] = 0x54
    data[3] = 0x46
    var seen = 0
    for(var i = 0; i < 4; i++) {
        seen = (seen << 8) | (data[i] as int)
    }
    return seen
}

func test_cursor_stride() {
    test("a read at cursor + i sees the index the source names (stride 2, not 1)", () => {
        // 0x49540000: index 0 ('I'), index 2 ('T'), then out of range -> 0, 0.
        return stride_double_counted() == 0x49540000;
    })
    test("a cursor alone walks consecutive elements", () => {
        // 0x49455446 == "IETF"
        return stride_cursor_only() == 0x49455446;
    })
    test("the same arithmetic with a local cursor, not a struct field", () => {
        return stride_local_cursor() == 0x49540000;
    })
    test("indexing by i alone from a fixed start walks consecutive elements", () => {
        return stride_fixed_start() == 0x49455446;
    })
    test("a cursor advanced in the loop reaches the end exactly once per step", () => {
        // After four iterations of `pos += 1` from 0, the cursor is 4 — not 8.
        // The stride-2 form above lands on 4 too, because it also steps once per
        // iteration; what differs is only the index each read lands on. Pinning
        // the cursor's own value keeps the two concerns separate.
        var c = StrideCursor { data : [] , pos : 0 }
        for(var i = 0; i < 4; i++) {
            c.pos += 1
        }
        return c.pos == 4;
    })
}
