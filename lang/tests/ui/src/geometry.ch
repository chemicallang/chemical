// P1.1 geometry/color property tests. Deterministic xorshift64, no allocation.
using std::Option;

public namespace geometry_tests {

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
    func fr(&mut self) : float {
        var n = self.u32() & 0xFFFFu32
        return (n as float) / 65535.0f
    }
    func range(&mut self, lo : float, hi : float) : float {
        return lo + (hi - lo) * self.fr()
    }
}

func absf(x : float) : float {
    if(x < 0.0f) { return 0.0f - x }
    return x
}

func near(a : float, b : float, eps : float) : bool {
    return absf(a - b) <= eps
}

func fail(name : *char, i : u64) : bool {
    unsafe { printf("geometry FAIL %s case=%lu\n", name, i) }
    return false
}

func check_rect_contains() : bool {
    var r = Rect.new(10.0f, 20.0f, 30.0f, 40.0f)
    if(!r.contains(Point.new(10.0f, 20.0f))) { return fail("contains-left-top", 0u64) }
    if(!r.contains(Point.new(39.999f, 59.999f))) { return fail("contains-inside", 0u64) }
    if(r.contains(Point.new(40.0f, 20.0f))) { return fail("contains-right-half-open", 0u64) }
    if(r.contains(Point.new(10.0f, 60.0f))) { return fail("contains-bottom-half-open", 0u64) }
    var i = r.inset(Insets.all(2.0f))
    if(!near(i.x, 12.0f, 0.001f) || !near(i.w, 26.0f, 0.001f)) { return fail("inset", 0u64) }
    var o = r.offset(5.0f, -5.0f)
    if(!near(o.x, 15.0f, 0.001f) || !near(o.y, 15.0f, 0.001f)) { return fail("offset", 0u64) }
    return true
}

func check_rect_properties() : bool {
    var rng = Rng { state : 0x9E3779B97F4A7C15u64 }
    var i = 0u64
    while(i < 10000u64) {
        var r1 = Rect.new(rng.range(-500.0f, 500.0f), rng.range(-500.0f, 500.0f),
                          rng.range(0.0f, 300.0f), rng.range(0.0f, 300.0f))
        var r2 = Rect.new(rng.range(-500.0f, 500.0f), rng.range(-500.0f, 500.0f),
                          rng.range(0.0f, 300.0f), rng.range(0.0f, 300.0f))
        var p1 = Point.new(r1.x + r1.w * 0.5f, r1.y + r1.h * 0.5f)
        var p2 = Point.new(r2.x + r2.w * 0.5f, r2.y + r2.h * 0.5f)
        if(r1.w > 0.0f && r1.h > 0.0f) {
            if(!r1.contains(p1)) { return fail("rect-contains-center", i) }
            if(r1.contains(Point.new(r1.x2(), r1.y))) { return fail("rect-right-edge", i) }
        }
        if(r1.w <= 0.0f || r1.h <= 0.0f) {
            if(r1.intersects(r2)) { return fail("empty-intersects", i) }
        }
        var inter = r1.intersection(r2)
        if(inter is Option.Some) {
            var Some(ri) = inter else return fail("intersection-pattern", i)
            if(!r1.intersects(r2)) { return fail("intersection-implies-intersects", i) }
            if(ri.w < 0.0f || ri.h < 0.0f) { return fail("intersection-negative", i) }
            if(ri.area() > r1.area() + 0.01f || ri.area() > r2.area() + 0.01f) {
                return fail("intersection-area", i)
            }
            if(ri.w > 0.0f && ri.h > 0.0f && !ri.contains(Point.new((ri.x + ri.x2()) * 0.5f, (ri.y + ri.y2()) * 0.5f))) {
                return fail("intersection-center", i)
            }
        } else {
            if(r1.intersects(r2)) { return fail("intersects-without-intersection", i) }
        }
        var u = r1.union(r2)
        if(r1.w > 0.0f && r1.h > 0.0f && !u.contains(p1)) { return fail("union-r1", i) }
        if(r2.w > 0.0f && r2.h > 0.0f && !u.contains(p2)) { return fail("union-r2", i) }
        if(u.area() + 0.01f < r1.area() || u.area() + 0.01f < r2.area()) {
            return fail("union-area", i)
        }
        i = i + 1u64
    }
    return true
}

func check_transform_properties() : bool {
    var rng = Rng { state : 0xD1B54A32D192ED03u64 }
    var i = 0u64
    while(i < 10000u64) {
        var a = Transform2D.translate(rng.range(-100.0f, 100.0f), rng.range(-100.0f, 100.0f)).multiply(Transform2D.rotate(rng.range(-3.0f, 3.0f))).multiply(Transform2D.scale(rng.range(0.1f, 3.0f), rng.range(0.1f, 3.0f)))
        var b = Transform2D.translate(rng.range(-100.0f, 100.0f), rng.range(-100.0f, 100.0f)).multiply(Transform2D.rotate(rng.range(-3.0f, 3.0f)))
        var p = Point.new(rng.range(-100.0f, 100.0f), rng.range(-100.0f, 100.0f))
        var composed = a.multiply(b)
        var direct = a.transform_point(b.transform_point(p))
        var combined = composed.transform_point(p)
        if(!near(direct.x, combined.x, 0.02f) || !near(direct.y, combined.y, 0.02f)) {
            return fail("transform-composition", i)
        }
        var unit = Point.new(1.0f, 0.0f)
        var r = Transform2D.rotate(0.0f).transform_point(unit)
        if(!near(r.x, 1.0f, 0.0001f) || !near(r.y, 0.0f, 0.0001f)) { return fail("rotate-zero", i) }
        i = i + 1u64
    }
    return true
}

func check_color_properties() : bool {
    var rng = Rng { state : 0x94D049BB133111EBu64 }
    var i = 0u64
    while(i < 10000u64) {
        var c = Color.rgba((rng.u32() & 0xFFu32) as u8, (rng.u32() & 0xFFu32) as u8,
                           (rng.u32() & 0xFFu32) as u8, (rng.u32() & 0xFFu32) as u8)
        if(Color.from_u32(c.to_u32()).to_u32() != c.to_u32()) { return fail("color-u32-roundtrip", i) }
        var pm = c.premultiply()
        var up = pm.unpremultiply()
        var tol = 1
        if(c.a == 0u8) { tol = 255 }
        else if(c.a < 255u8) { tol = (255 / (c.a as int)) + 1 }
        var dr = (c.r as int) - (up.r as int)
        var dg = (c.g as int) - (up.g as int)
        var db = (c.b as int) - (up.b as int)
        if(dr < 0) { dr = 0 - dr }
        if(dg < 0) { dg = 0 - dg }
        if(db < 0) { db = 0 - db }
        if(dr > tol || dg > tol || db > tol) { return fail("color-premul-roundtrip", i) }
        var dst = Color.rgba(10u8, 20u8, 30u8, 200u8)
        var opaque = Color.rgba(200u8, 100u8, 50u8, 255u8)
        if(opaque.over(dst).to_u32() != opaque.to_u32()) { return fail("color-over-opaque", i) }
        var transparent = Color.rgba(1u8, 2u8, 3u8, 0u8)
        if(transparent.over(dst).to_u32() != dst.to_u32()) { return fail("color-over-transparent", i) }
        var l0 = c.lerp(dst, 0.0f)
        var l1 = c.lerp(dst, 1.0f)
        if(l0.to_u32() != c.to_u32() || l1.to_u32() != dst.to_u32()) { return fail("color-lerp-endpoints", i) }
        if(Color.new(0u8, 0u8, 0u8).relative_luminance() != 0.0) { return fail("black-luminance", i) }
        if(!near(Color.new(0u8, 0u8, 0u8).contrast_ratio(Color.new(255u8, 255u8, 255u8)) as float, 21.0f, 0.01f)) {
            return fail("bw-contrast", i)
        }
        i = i + 1u64
    }
    return true
}


func check_value_methods() : bool {
    var a = Point.new(1.0f, 2.0f)
    var b = Point.new(3.0f, 5.0f)
    var sum = a.add(b)
    var diff = b.sub(a)
    var scaled = a.scale(2.0f)
    var mid = a.lerp(b, 0.5f)
    if(!near(sum.x, 4.0f, 0.001f) || !near(diff.y, 3.0f, 0.001f)) { return fail("point-ops", 0u64) }
    if(!near(scaled.x, 2.0f, 0.001f) || !near(mid.x, 2.0f, 0.001f)) { return fail("point-lerp", 0u64) }
    var s1 = Size.new(10.0f, 20.0f)
    var s2 = Size.new(3.0f, 4.0f)
    if(!near(s1.sub(s2).width, 7.0f, 0.001f) || !near(s1.max(s2).height, 20.0f, 0.001f)) { return fail("size-ops", 0u64) }
    if(!near(s1.min(s2).width, 3.0f, 0.001f) || !near(s1.area(), 200.0f, 0.001f)) { return fail("size-area", 0u64) }
    var i = Insets.symmetric(4.0f, 6.0f)
    if(!near(i.left, 4.0f, 0.001f) || !near(i.bottom, 6.0f, 0.001f)) { return fail("insets-symmetric", 0u64) }
    var h = Insets.horizontal(3.0f)
    var v = Insets.vertical(5.0f)
    if(!near(h.right, 3.0f, 0.001f) || !near(v.top, 5.0f, 0.001f)) { return fail("insets-axis", 0u64) }
    var c = Color.rgba(1u8, 2u8, 3u8, 200u8)
    if(c.is_opaque() || !c.with_alpha(255u8).is_opaque()) { return fail("color-alpha", 0u64) }
    if(c.to_premul_u32() != c.premultiply().to_u32()) { return fail("color-premul-u32", 0u64) }
    var id = Transform2D.identity()
    var p = id.transform_point(a)
    if(!near(p.x, a.x, 0.001f) || !near(p.y, a.y, 0.001f)) { return fail("transform-identity", 0u64) }
    return true
}

func check_clamps() : bool {
    var inf = 1.0f / 0.0f
    var p = Point.new(inf, 0.0f - inf)
    if(!p.is_finite() || p.x != 0.0f || p.y != 0.0f) { return fail("point-nonfinite-clamp", 0u64) }
    var s = Size.new(-5.0f, inf)
    if(s.width != 0.0f || s.height != 0.0f) { return fail("size-clamp", 0u64) }
    var r = Rect.new(-1.0f, -2.0f, -3.0f, -4.0f)
    if(r.w != 0.0f || r.h != 0.0f) { return fail("rect-negative-clamp", 0u64) }
    var l = Rect.from_ltrb(10.0f, 10.0f, 2.0f, 4.0f)
    if(!near(l.x, 2.0f, 0.001f) || !near(l.w, 8.0f, 0.001f)) { return fail("from-ltrb", 0u64) }
    return true
}

public func run() : bool {
    var ok = true
    if(!check_rect_contains()) { ok = false }
    if(!check_rect_properties()) { ok = false }
    if(!check_transform_properties()) { ok = false }
    if(!check_color_properties()) { ok = false }
    if(!check_value_methods()) { ok = false }
    if(!check_clamps()) { ok = false }
    return ok
}

} // namespace geometry_tests
