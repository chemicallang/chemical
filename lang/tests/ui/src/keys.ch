// P1.2 key identity, bounded debug table, duplicate scope, and 10M tuple check.
using std::Option;
using std::Result;
using std::string_view;
using ui_internal::KeyScope;
using ui_internal::KeyedSlots;
using ui_internal::key_with_hash;

public namespace keys_tests {

func fail(name : *char, n : u64) : bool {
    unsafe { printf("keys FAIL %s case=%lu\n", name, n) }
    return false
}

func check_basics() : bool {
    var a = key("alpha")
    var b = key("alpha")
    var c = key("beta")
    if(!a.equals_key(b) || a.equals_key(c)) { return fail("hash-equals", 0u64) }
    if(!a.equals("alpha") || a.equals("beta")) { return fail("label-equals", 0u64) }
    var i = key_of("alpha", 9u64)
    if(i.id() != 9u64 || i.tag() != 0u64) { return fail("key-of", 0u64) }
    var p = key_pair("row", 1u64, 2u64)
    var q = key_pair("row", 2u64, 1u64)
    if(p.equals_key(q)) { return fail("pair-order", 0u64) }
    if(p.id() != 1u64 || p.tag() != 2u64) { return fail("pair-id-tag", 0u64) }
    if(!p.equals_key(key_pair("row", 1u64, 2u64))) { return fail("pair-repeat", 0u64) }
    var label : string_view = "dynamic-label"
    var d = key(label)
    if(!d.equals(label)) { return fail("dynamic-label", 0u64) }
    return true
}

func check_scope() : bool {
    var scope = KeyScope.make()
    var a = key("same")
    var r1 = scope.insert(a)
    var r2 = scope.insert(key("same"))
    if(r1 is Result.Err || r2 is Result.Ok) { return fail("duplicate-key", 0u64) }
    if(r2 is Result.Err) {
        var Err(e) = r2 else return false
        if(e.code() != ErrorCode.DuplicateKey) { return fail("duplicate-code", 0u64) }
    }
    if(scope.size() != 1u) { return fail("duplicate-size", 0u64) }
    var c1 = key_with_hash(0x1234u64, 0u64, 1u64)
    var c2 = key_with_hash(0x1234u64, 0u64, 2u64)
    var r3 = scope.insert(c1)
    var r4 = scope.insert(c2)
    if(r3 is Result.Err || r4 is Result.Err) { return fail("collision-rejected", 0u64) }
    if(scope.size() != 3u || !scope.contains(c1) || !scope.contains(c2)) { return fail("collision-scope", 0u64) }
    var slots = KeyedSlots.make()
    slots.set(a, 11i64)
    slots.set(c2, 22i64)
    var g = slots.get(a)
    if(g is Option.Some) {
        var Some(v) = g else return false
        if(v != 11i64) { return fail("slot-value", 0u64) }
    } else {
        return fail("slot-present", 0u64)
    }
    if(slots.get(c1) is Option.Some) { return fail("slot-collision", 0u64) }
    return true
}

func check_debug_overflow() : bool {
    var before = key("stable")
    var i = 0u32
    while(i < 128u32) {
        var k = key_of("overflow", i as u64)
        if(k.id() != i as u64) { return fail("debug-id", i as u64) }
        i = i + 1u32
    }
    var after = key("stable")
    if(!before.equals_key(after)) { return fail("debug-identity", 0u64) }
    if(ui_internal::key_debug_count() > 64u32) { return fail("debug-bound", 0u64) }
    return true
}

func check_10m() : bool {
    // 10M distinct tuples; no large table is retained (the tuple id is the
    // uniqueness witness, and the 64-bit hash is deterministic).
    var i = 0u64
    while(i < 10000000u64) {
        var k = key_of("row", i)
        if(k.id() != i || k.tag() != 0u64) { return fail("10m-id", i) }
        if(!k.equals_key(key_of("row", i))) { return fail("10m-repeat", i) }
        i = i + 1u64
    }
    return true
}

public func run() : bool {
    var ok = true
    if(!check_basics()) { ok = false }
    if(!check_scope()) { ok = false }
    if(!check_debug_overflow()) { ok = false }
    if(!check_10m()) { ok = false }
    if(ok) { unsafe { printf("keys 10m distinct=10000000 debug_cap=64\n") } }
    return ok
}

}
