// ============================================================
// Tests for a compiler bug: reading a *destructible* member off a
// struct-returning temporary destroys that member twice.
//
// For
//     make_owner().inner
// (or any expression that materializes the member, e.g.
//  make_owner().inner.get_tag()) the C (TCC) backend emits:
//
//   struct Inner __chx__lv__5 = ({
//       struct Owner* __chx__lv__6 = &(*({ struct Owner __chx__lv__7; make_owner(&__chx__lv__7); &__chx__lv__7; }));
//       struct Inner __chx__lv__8 = __chx__lv__6->inner;  // shallow copy of the member
//       Ownerdelete(__chx__lv__6);                        // destroys __chx__lv__7->inner  (1st)
//       __chx__lv__8;
//   });
//   Innerdelete(&__chx__lv__5);                           // destroys the copy             (2nd)
//
// The member ends up destroyed twice. When it owns heap memory
// (std::string / std::vector) the second destruction is a double
// free and aborts the process.
//
// Backend matrix (destruction count; 1 is correct):
//
//   expression                              | TCC (2c) | LLVM
//   ----------------------------------------|----------|------
//   var x = f().inner.get_tag()             |  2 BUG   |  1 ok
//   f().inner.get_tag()      (bare stmt)    |  2 BUG   |  1 ok
//   f().inner                (bare stmt)    |  2 BUG   |  2 BUG
//   var x = f().inner.tag   (int submember) |  1 ok    |  1 ok
//   var o = f(); x = o.inner.get_tag()      |  1 ok    |  1 ok
//
// The first two rows are a C (2c) backend bug (LLVM lowers them
// correctly). The bare `f().inner` statement is a second, related
// defect that both backends share.
//
// TempMemberInner mirrors a destructible member but owns no memory,
// so the destruction count can be asserted without crashing.
// ============================================================

var temp_member_destruct_count : int = 0

@retained
struct TempMemberInner {

    var tag : int

    @constructor
    func constructor(t : int) {
        return TempMemberInner { tag : t }
    }

    @delete
    func delete(&mut self) {
        temp_member_destruct_count = temp_member_destruct_count + 1
    }

    func get_tag(&self) : int {
        return self.tag
    }

}

struct TempMemberOuter {

    var inner : TempMemberInner

    @constructor
    func constructor(t : int) {
        return TempMemberOuter { inner : TempMemberInner(t) }
    }

}

func make_temp_member_outer() : TempMemberOuter {
    return TempMemberOuter(7)
}

func test_temp_member_destruct() {

    // C (2c) backend bug: the member is destroyed twice (count == 2).
    // Passes on LLVM.
    test("method call on a member of a struct-returning temporary destroys it once", () => {
        temp_member_destruct_count = 0
        if(true) {
            var x = make_temp_member_outer().inner.get_tag()
        }
        return temp_member_destruct_count == 1
    })

    // Same C (2c) bug via a bare method call.
    test("bare method call on a member of a struct-returning temporary destroys it once", () => {
        temp_member_destruct_count = 0
        if(true) {
            make_temp_member_outer().inner.get_tag()
        }
        return temp_member_destruct_count == 1
    })

    // Second, related defect: the bare member expression destroys the
    // member twice on BOTH backends.
    test("bare member expression off a struct-returning temporary destroys it once", () => {
        temp_member_destruct_count = 0
        if(true) {
            make_temp_member_outer().inner
        }
        return temp_member_destruct_count == 1
    })

    // Control: binding the temporary to a local first is correct.
    test("member read off a local struct destroys it once", () => {
        temp_member_destruct_count = 0
        if(true) {
            var o = make_temp_member_outer()
            var x = o.inner.get_tag()
        }
        return temp_member_destruct_count == 1
    })

    // Control: reading a non-destructible sub-member never materializes
    // the member and is correct on both backends.
    test("int sub-member read off a struct-returning temporary is correct", () => {
        temp_member_destruct_count = 0
        if(true) {
            var x = make_temp_member_outer().inner.tag
        }
        return temp_member_destruct_count == 1
    })

}
