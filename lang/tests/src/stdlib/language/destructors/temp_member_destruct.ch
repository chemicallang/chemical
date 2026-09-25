// ============================================================
// Regression tests: reading a *destructible* member off a
// struct-returning temporary used to destroy that member twice.
//
// The C (2c) backend used to emit, for make_owner().inner
// (or any expression that materializes the member, e.g.
//  make_owner().inner.get_tag()):
//
//   struct Inner __chx__lv__5 = ({
//       struct Owner* __chx__lv__6 = &(*({ struct Owner __chx__lv__7; make_owner(&__chx__lv__7); &__chx__lv__7; }));
//       struct Inner __chx__lv__8 = __chx__lv__6->inner;  // shallow copy of the member
//       Ownerdelete(__chx__lv__6);                        // destroys __chx__lv__7->inner  (1st)
//       __chx__lv__8;
//   });
//   Ownerdelete(&__chx__lv__7);                           // destroys it again            (2nd)
//
// The member ended up destroyed twice. When it owns heap memory
// (std::string / std::vector) the second destruction was a double
// free and aborted the process (see AccessChain::is_alias_into_destroyed_temp,
// and the destructible-call-chain handling in
// preprocess/2c/2cASTVisitor.cpp).
//
// Backend matrix (destruction count; 1 is correct), all fixed:
//
//   expression                              | TCC (2c) | LLVM
//   ----------------------------------------|----------|------
//   var x = f().inner.get_tag()             |  1       |  1
//   f().inner.get_tag()      (bare stmt)    |  1       |  1
//   f().inner                (bare stmt)    |  1       |  1
//   var x = f().inner.tag   (int submember) |  1       |  1
//   var o = f(); x = o.inner.get_tag()      |  1       |  1
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

// the member owns heap memory, so destroying it twice was a double free
// rather than a wrong counter
struct TempMemberHeapOuter {

    var name : std::string

    @constructor
    func constructor(n : std::string) {
        return TempMemberHeapOuter { name : n }
    }

}

func make_temp_member_heap_outer() : TempMemberHeapOuter {
    return TempMemberHeapOuter(std::string("hello-world"))
}

func test_temp_member_destruct() {

    // used to destroy the member twice on the C (2c) backend (count == 2)
    test("method call on a member of a struct-returning temporary destroys it once", () => {
        temp_member_destruct_count = 0
        if(true) {
            var x = make_temp_member_outer().inner.get_tag()
        }
        return temp_member_destruct_count == 1
    })

    // same double destruction, via a bare method call
    test("bare method call on a member of a struct-returning temporary destroys it once", () => {
        temp_member_destruct_count = 0
        if(true) {
            make_temp_member_outer().inner.get_tag()
        }
        return temp_member_destruct_count == 1
    })

    // the bare member expression used to destroy the member twice on
    // BOTH backends (the value is an alias into the temporary the chain
    // already destroyed)
    test("bare member expression off a struct-returning temporary destroys it once", () => {
        temp_member_destruct_count = 0
        if(true) {
            make_temp_member_outer().inner
        }
        return temp_member_destruct_count == 1
    })

    // the member owns heap memory, so destroying it twice was a double free.
    // the value must also still be intact when it is read
    test("heap owning member of a struct-returning temporary is read correctly", () => {
        return (make_temp_member_heap_outer().name.size() as int) == 11
    })

    // control: binding the temporary to a local first is correct
    test("member read off a local struct destroys it once", () => {
        temp_member_destruct_count = 0
        if(true) {
            var o = make_temp_member_outer()
            var x = o.inner.get_tag()
        }
        return temp_member_destruct_count == 1
    })

    // control: reading a non-destructible sub-member never materializes
    // the member and is correct on both backends
    test("int sub-member read off a struct-returning temporary is correct", () => {
        temp_member_destruct_count = 0
        if(true) {
            var x = make_temp_member_outer().inner.tag
        }
        return temp_member_destruct_count == 1
    })

}
