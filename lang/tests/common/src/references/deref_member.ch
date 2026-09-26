// Member access through an explicit dereference: `(*ptr).field`.
//
// These are the shapes every backend must get right:
//   * reading : `return (*ptr).field`
//   * writing : `(*ptr).field = value`
//   * nested  : `(*ptr).inner.field`
//
// The C translator used to write the deref's `*` *and* the access chain's
// `->` for the same pointer (`*ptr->field`). In C that parses as
// `*(ptr->field)`, which is not even valid when the field is not a pointer
// ("pointer expected: couldn't compile the program"), so programs using this
// spelling did not compile at all. The pointer has to be dereferenced exactly
// once: `ptr->field`.

struct DerefMemberHost {
    var value : int
    var other : int
}

struct DerefMemberOuter {
    var inner : DerefMemberHost
}

func deref_member_read(host : *mut DerefMemberHost) : int {
    return (*host).value
}

func deref_member_write(host : *mut DerefMemberHost, value : int) {
    (*host).value = value
}

func deref_member_read_nested(outer : *mut DerefMemberOuter) : int {
    return (*outer).inner.other
}

func test_deref_member_access() {

    test("(*ptr).field reads the member through the pointer", () => {
        var host = DerefMemberHost { value : 7, other : 11 }
        var ptr = &raw mut host
        return (*ptr).value == 7
    })

    test("(*ptr).field = value writes the member through the pointer", () => {
        var host = DerefMemberHost { value : 7, other : 11 }
        var ptr = &raw mut host
        (*ptr).value = 21
        return host.value == 21
    })

    test("(*ptr).field leaves the other members alone", () => {
        var host = DerefMemberHost { value : 7, other : 11 }
        var ptr = &raw mut host
        (*ptr).value = 21
        return (*ptr).other == 11
    })

    test("(*ptr).inner.member reads a nested member through the pointer", () => {
        var outer = DerefMemberOuter { inner : DerefMemberHost { value : 1, other : 33 } }
        var ptr = &raw mut outer
        return (*ptr).inner.other == 33
    })

    test("(*ptr).inner.member can be written through the pointer", () => {
        var outer = DerefMemberOuter { inner : DerefMemberHost { value : 1, other : 33 } }
        var ptr = &raw mut outer
        (*ptr).inner.other = 44
        return outer.inner.other == 44
    })

    test("reading (*ptr).field inside a function taking the pointer works", () => {
        var host = DerefMemberHost { value : 5, other : 6 }
        return deref_member_read(&raw mut host) == 5
    })

    test("writing (*ptr).field inside a function taking the pointer works", () => {
        var host = DerefMemberHost { value : 5, other : 6 }
        deref_member_write(&raw mut host, 42)
        return host.value == 42
    })

    test("reading (*ptr).inner.field inside a function works", () => {
        var outer = DerefMemberOuter { inner : DerefMemberHost { value : 2, other : 9 } }
        return deref_member_read_nested(&raw mut outer) == 9
    })
}
