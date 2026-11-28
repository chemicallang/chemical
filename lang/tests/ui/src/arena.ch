// P1.2 NodeArena/FrameArena deterministic fuzz and frame-arena invariants.
using std::Option;
using std::Result;
using std::vector;
using ui_internal::NodeArena;
using ui_internal::FrameArena;

public namespace arena_tests {

struct Rng {
    var state : u64
    func next(&mut self) : u64 {
        var x = self.state
        x = x ^ (x << 13u64)
        x = x ^ (x >> 7u64)
        x = x ^ (x << 17u64)
        self.state = x
        return x
    }
    func u32(&mut self) : u32 { return (self.next() >> 32u64) as u32 }
}

func fail(name : *char, n : u64) : bool {
    unsafe { printf("arena FAIL %s case=%lu\n", name, n) }
    return false
}

func check_wrap() : bool {
    var a = NodeArena.new(2u32)
    var r = a.insert()
    if(r is Result.Err) { return fail("wrap-insert", 0u64) }
    var Ok(id) = r else return false
    var rm = a.remove(id)
    if(rm is Result.Err) { return fail("wrap-remove", 0u64) }
    var sg = a.debug_set_generation(id.index(), 0xFFFFFFFEu32)
    if(sg is Result.Err) { return fail("wrap-set-generation", 0u64) }
    var r2 = a.insert()
    if(r2 is Result.Err) { return fail("wrap-last-insert", 0u64) }
    var Ok(last) = r2 else return false
    if(last.generation() != 0xFFFFFFFFu32) { return fail("wrap-generation", 0u64) }
    var rw = a.remove(last)
    if(rw is Result.Ok) { return fail("wrap-remove-should-exhaust", 0u64) }
    if(rw is Result.Err) {
        var Err(we) = rw else return false
        if(we.code() != ErrorCode.ResourceExhausted) { return fail("wrap-code", 0u64) }
    }
    if(a.contains(last)) { return fail("wrap-still-live", 0u64) }
    var r3 = a.insert()
    if(r3 is Result.Err) { return fail("wrap-new-slot", 0u64) }
    var Ok(fresh) = r3 else return false
    if(fresh.index() == last.index() && fresh.generation() == last.generation()) {
        return fail("wrap-alias", 0u64)
    }
    return true
}

func check_static() : bool {
    var a = NodeArena.new(4u32)
    var rr = a.reserve(4u32)
    if(rr is Result.Err) { return fail("static-reserve", 0u64) }
    var rr2 = a.reserve(5u32)
    if(rr2 is Result.Ok) { return fail("static-reserve-bound", 0u64) }
    var invalid = zeroed<NodeId>()
    if(invalid.is_valid() || a.contains(invalid)) { return fail("static-invalid-handle", 0u64) }
    var full = NodeArena.new(1u32)
    var f1 = full.insert()
    if(f1 is Result.Err) { return fail("static-full-first", 0u64) }
    var f2 = full.insert()
    if(f2 is Result.Ok) { return fail("static-full-bound", 0u64) }
    if(f2 is Result.Err) {
        var Err(fe) = f2 else return false
        if(fe.code() != ErrorCode.ResourceExhausted) { return fail("static-full-code", 0u64) }
    }
    var r = a.insert()
    var Ok(first) = r else return fail("static-insert", 0u64)
    if(!first.is_valid() || first.index() == 0u32 || first.generation() == 0u32) { return fail("static-id", 0u64) }
    if(!a.contains(first) || a.get(first) is Option.None) { return fail("static-contains", 0u64) }
    var rm = a.remove(first)
    if(rm is Result.Err) { return fail("static-remove", 0u64) }
    if(a.contains(first)) { return fail("static-stale", 0u64) }
    var rbad = a.remove(first)
    if(rbad is Result.Ok) { return fail("static-stale-remove", 0u64) }
    if(rbad is Result.Err) {
        var Err(e) = rbad else return false
        if(e.code() != ErrorCode.StaleNode) { return fail("static-stale-code", 0u64) }
    }
    var r2 = a.insert()
    var Ok(reused) = r2 else return fail("static-reuse", 0u64)
    if(reused.index() != first.index()) { return fail("static-free-reuse", 0u64) }
    if(reused.generation() == first.generation()) { return fail("static-generation-bump", 0u64) }
    var clear_arena = NodeArena.new(2u32)
    var c = clear_arena.insert()
    var Ok(cid) = c else return fail("static-clear-insert", 0u64)
    clear_arena.clear()
    if(clear_arena.contains(cid)) { return fail("static-clear-stale", 0u64) }
    var c2 = clear_arena.insert()
    var Ok(cid2) = c2 else return fail("static-clear-reuse", 0u64)
    if(cid2.index() == cid.index() && cid2.generation() == cid.generation()) { return fail("static-clear-alias", 0u64) }
    return true
}

func check_fuzz() : bool {
    var arena = NodeArena.new(8192u32)
    var rng = Rng { state : 0x9E3779B97F4A7C15u64 }
    var live = vector<NodeId>()
    var i = 0u64
    while(i < 1000000u64) {
        var op = rng.u32() % 3u32
        if(live.size() == 0u || op == 0u32) {
            var r = arena.insert()
            if(r is Result.Err) { return fail("fuzz-insert", i) }
            var Ok(id) = r else return false
            live.push(id)
        } else if(op == 1u32) {
            var at = (rng.u32() as size_t) % live.size()
            var id = unsafe(live.get_ptr(at)[0])
            var rm = arena.remove(id)
            if(rm is Result.Err) { return fail("fuzz-remove", i) }
            live.remove(at)
            if(arena.contains(id)) { return fail("fuzz-removed-live", i) }
            var again = arena.remove(id)
            if(again is Result.Ok) { return fail("fuzz-stale-alias", i) }
        } else {
            var at = (rng.u32() as size_t) % live.size()
            var id = unsafe(live.get_ptr(at)[0])
            if(!arena.contains(id)) { return fail("fuzz-live-missing", i) }
            if(arena.get(id) is Option.None) { return fail("fuzz-get", i) }
        }
        if((i & 1023u64) == 0u64 && arena.len() != live.size() as u32) { return fail("fuzz-live-count", i) }
        i = i + 1u64
    }
    if(arena.len() != live.size() as u32) { return fail("fuzz-final-live", 0u64) }
    var j : size_t = 0u
    while(j < live.size()) {
        var id = unsafe(live.get_ptr(j)[0])
        var rm = arena.remove(id)
        if(rm is Result.Err) { return fail("fuzz-drain", j as u64) }
        j = j + 1u
    }
    if(arena.len() != 0u32) { return fail("fuzz-leak", 0u64) }
    return true
}

func check_frame_arena() : bool {
    var f = FrameArena.new(256u)
    var r1 = f.alloc_offset(3u, 8u)
    var Ok(off1) = r1 else return fail("frame-alloc-1", 0u64)
    var r2 = f.alloc_offset(5u, 16u)
    var Ok(off2) = r2 else return fail("frame-alloc-2", 0u64)
    if(off1 % 8u != 0u || off2 % 16u != 0u) { return fail("frame-align", 0u64) }
    if(f.used() != off2 + 5u) { return fail("frame-used", 0u64) }
    if(f.bytes().size() != f.used()) { return fail("frame-bytes", 0u64) }
    var s1 = f.slice(off1, 3u)
    if(s1 is Option.None) { return fail("frame-slice", 0u64) }
    var s2 = f.slice(off2, 6u)
    if(s2 is Option.Some) { return fail("frame-slice-bounds", 0u64) }
    var rp = f.alloc(1u, 1u)
    if(rp is Result.Ok) {
        var Ok(ptr) = rp else return false
        if(ptr == null) { return fail("frame-alloc-null", 0u64) }
    } else {
        return fail("frame-alloc", 0u64)
    }
    var over = f.alloc_offset(1000u, 8u)
    if(over is Result.Ok) { return fail("frame-overflow", 0u64) }
    if(over is Result.Err) {
        var Err(e) = over else return false
        if(e.code() != ErrorCode.ResourceExhausted) { return fail("frame-overflow-code", 0u64) }
    }
    var high = f.high_water()
    f.reset()
    if(f.used() != 0u || f.allocations() != 0u32 || f.capacity() != 256u) { return fail("frame-reset", 0u64) }
    if(f.high_water() != high) { return fail("frame-high-water", 0u64) }
    var r3 = f.alloc_offset(4u, 1u)
    var Ok(off3) = r3 else return fail("frame-reuse", 0u64)
    if(off3 != 0u) { return fail("frame-reuse-offset", 0u64) }
    var big = FrameArena.new(4096u)
    var i = 0u32
    while(i < 1000u32) {
        var a = big.alloc_offset(1u, 1u)
        if(a is Result.Err) { return fail("frame-fill", i as u64) }
        i = i + 1u32
    }
    if(big.capacity() != 4096u || big.allocations() != 1000u32) { return fail("frame-no-growth", 0u64) }
    if(big.used() != 1000u) { return fail("frame-fill-used", 0u64) }
    return true
}

public func run() : bool {
    var ok = true
    if(!check_static()) { ok = false }
    if(!check_wrap()) { ok = false }
    if(!check_fuzz()) { ok = false }
    if(!check_frame_arena()) { ok = false }
    if(ok) { unsafe { printf("arena ops=1000000 capacity=8192\n") } }
    return ok
}

}
