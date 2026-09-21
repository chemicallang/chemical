// Copyright (c) Chemical Language Foundation 2026.

// -------------------------------------------------------
// Regression: moving a variant payload out in a value-if
//
//     var t = if(var Some(v) = m) v else fallback
//
// must leave `m` moved-from, so `m`'s destructor must NOT destroy the payload
// that was moved out. The TCC backend used to emit the moved-from variant's
// destructor as well, destroying the payload twice — a double free for
// heap-owning payloads (the original symptom was heap corruption,
// 0xC0000374 on Windows / "double free or corruption" on glibc when a
// std::Option<std::string> was read this way).
//
// A global destructor counter makes the regression fail deterministically
// (count == 2 instead of 1) instead of relying on the process crashing.
// -------------------------------------------------------

var variant_move_delete_count = 0

struct VariantMoveTracked {
    var value : int

    @make
    func make(v : int) : VariantMoveTracked {
        return VariantMoveTracked { value : v }
    }

    @delete
    func delete(&mut self) {
        variant_move_delete_count = variant_move_delete_count + 1
    }
}

variant VariantMoveMaybe {
    Some(value : VariantMoveTracked)
    None()
}

public func test_variant_payload_move_value_if() {

    // The regression: the payload is destroyed twice (once as the moved value,
    // once again by the moved-from variant).
    test("variant payload moved out in a value-if is destroyed once", () => {
        variant_move_delete_count = 0
        var result = -1
        {
            var m = VariantMoveMaybe.Some(VariantMoveTracked.make(7))
            var t = if(var Some(v) = m) v else VariantMoveTracked.make(0)
            result = t.value
        }
        return result == 7 && variant_move_delete_count == 1
    })

    // Control: a plain variant value is destroyed exactly once. If this one
    // fails too, the counter itself (or the variant destructor) is broken and
    // the regression above is not meaningful.
    test("variant payload constructed and dropped is destroyed once", () => {
        variant_move_delete_count = 0
        {
            var m = VariantMoveMaybe.Some(VariantMoveTracked.make(3))
        }
        return variant_move_delete_count == 1
    })
}
