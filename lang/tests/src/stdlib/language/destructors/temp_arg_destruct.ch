// ============================================================
// Tests for a compiler bug: a temporary struct passed as a
// method argument is destroyed on the wrong (uninitialized)
// stack slot instead of the temporary itself.
//
// Reproduced with the C (TCC) backend. For
//     a.equals(TrackedTemp("x"))
// the generated C declares a fresh local for the destruction job
// but never initializes it, then deletes that local while the real
// temporary leaks:
//
//   struct TrackedTemp __chx__lv__1;            // never initialized
//   ... equals(a, &(*({ struct TrackedTemp __chx__lv__3; ctor(&__chx__lv__3, "x", 1, 0); ... })))
//   TrackedTempdelete(&__chx__lv__1);           // BUG: should be __chx__lv__3
//
// TrackedTemp mirrors std::string's @implicit @constructor pattern,
// so this exercises the same code path as
//     some_string.equals(string("literal"))
// which double-frees in real programs (the stray delete lands on a
// leftover heap string in the reused stack frame).
// ============================================================

var temp_arg_destruct_count : int = 0
var temp_arg_destruct_values : [8]int

@retained
struct TrackedTemp {

    var value : int
    var length : size_t

    @constructor
    comptime func make(value : %literal_string) {
        return %runtime_value(constructor2(value, intrinsics::size(value), false))
    }

    @constructor
    func constructor2(value : *char, length : size_t, ensure : bool) {
        var s = TrackedTemp { value : length as int, length : length }
        return s
    }

    @delete
    func delete(&mut self) {
        if(temp_arg_destruct_count < 8) {
            temp_arg_destruct_values[temp_arg_destruct_count] = self.value
        }
        temp_arg_destruct_count = temp_arg_destruct_count + 1
    }

    func equals(&self, other : &TrackedTemp) : bool {
        return self.value == other.value
    }

}

func consume_tracked_temp(a : &TrackedTemp) : bool {
    // TrackedTemp("x") has length 1, so its destructor must record value 1.
    // With the bug an uninitialized slot is destroyed instead and value 1
    // never reaches the destructor.
    return a.equals(TrackedTemp("x"))
}

func test_temp_arg_destruct() {

    test("temporary passed as a method argument is destructed, not a stray stack slot", () => {
        temp_arg_destruct_count = 0
        if(true) {
            var a = TrackedTemp("aaaa")   // length 4 -> value 4
            var r = consume_tracked_temp(&a)
        }
        // Expect exactly two destructions: the temporary (value 1) and `a`
        // (value 4). With the bug the temporary's value is missing.
        var found_temp = false
        var found_a = false
        var i = 0
        while(i < temp_arg_destruct_count && i < 8) {
            if(temp_arg_destruct_values[i] == 1) { found_temp = true }
            if(temp_arg_destruct_values[i] == 4) { found_a = true }
            i = i + 1
        }
        return temp_arg_destruct_count == 2 && found_temp && found_a
    })

}
