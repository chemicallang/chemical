// P0.1 baseline: a minimal "window-like" app without any platform host: one
// 960x640 RGBA surface, 120 frames of clear + quad fill + damage copy, and a
// checksum so the work cannot be optimized away. Used to compare TCC vs LLVM
// compile time and binary size before a real host exists.
using std::chrono::Instant;
using std::vector;

@direct_init
struct Surface {
    var w : int
    var h : int
    var pixels : vector<u8>
}

func surface_new(w : int, h : int) : Surface {
    var s = Surface { w : w, h : h, pixels : vector<u8>() }
    s.pixels.resize((w as size_t) * (h as size_t) * 4u)
    return s
}

func surface_fill(s : &mut Surface, r : u8, g : u8, b : u8) {
    var px = s.pixels.as_mut_span()
    var n = px.size() / 4u
    var i : size_t = 0u
    while(i < n) {
        px[i * 4u + 0u] = r
        px[i * 4u + 1u] = g
        px[i * 4u + 2u] = b
        px[i * 4u + 3u] = 255u8
        i = i + 1u
    }
}

func surface_quad(s : &mut Surface, x : int, y : int, w : int, h : int) {
    var px = s.pixels.as_mut_span()
    var yy = 0
    while(yy < h) {
        var xx = 0
        while(xx < w) {
            var o = (((y + yy) as size_t) * (s.w as size_t) + ((x + xx) as size_t)) * 4u
            px[o + 0u] = 240u8
            px[o + 1u] = 200u8
            px[o + 2u] = 80u8
            xx = xx + 1
        }
        yy = yy + 1
    }
}

func surface_copy(dst : &mut Surface, src : &Surface) {
    var d = dst.pixels.as_mut_span()
    var s = src.pixels.as_span()
    var n = d.size()
    var i : size_t = 0u
    while(i < n) {
        d[i] = s[i]
        i = i + 1u
    }
}

public func main() : int {
    var back = surface_new(960, 640)
    var front = surface_new(960, 640)
    var checksum : u64 = 0u64
    var frames = 120
    var t = Instant.now()
    var f = 0
    while(f < frames) {
        surface_fill(&mut back, 24u8, 26u8, 32u8)
        surface_quad(&mut back, 40 + (f % 80), 60, 320, 48)
        surface_copy(&mut front, &back)
        checksum = checksum + (front.pixels.as_span()[0] as u64)
        f = f + 1
    }
    var elapsed = t.elapsed()
    var ms = elapsed.as_millis()
    unsafe {
        printf("P0.1 baseline frames=%d checksum=%lu elapsed_ms=%ld\n", frames, checksum, ms)
    }
    if(checksum == 0u64) { return 1 }
    println("P0.1 baseline PASS")
    return 0
}
