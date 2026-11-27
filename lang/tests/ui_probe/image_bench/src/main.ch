// P0.1: PNG encode speed for the screenshot path (image.encode_png).
using std::Result;
using std::chrono::Instant;

func fill_gradient(img : &mut image::Image) {
    var w = img.width()
    var h = img.height()
    var px = img.pixels_mut()
    var y = 0
    while(y < h) {
        var x = 0
        while(x < w) {
            var o = ((y as size_t) * (w as size_t) + (x as size_t)) * 4u
            px[o + 0u] = ((x * 255) / w) as u8
            px[o + 1u] = ((y * 255) / h) as u8
            px[o + 2u] = (((x + y) * 127) / (w + h)) as u8
            px[o + 3u] = 255u8
            x = x + 1
        }
        y = y + 1
    }
}

func bench(w : int, h : int, iters : int) : int {
    var created = image::Image.new_rgba(w, h)
    if(created is Result.Err) {
        println("P0.1 image_png FAIL create")
        return 1
    }
    var Ok(img) = created else unsafe { unreachable }
    fill_gradient(&mut img)

    var best : i64 = 0
    var bytes : size_t = 0
    var i = 0
    while(i < iters) {
        var t = Instant.now()
        var encoded = img.encode_png()
        var elapsed = t.elapsed()
        if(encoded is Result.Err) {
            println("P0.1 image_png FAIL encode")
            return 1
        }
        var Ok(out) = encoded else unsafe { unreachable }
        bytes = out.size()
        var ms = elapsed.as_millis()
        if(best == 0 || ms < best) { best = ms }
        i = i + 1
    }
    var mpx = ((w as i64) * (h as i64)) / 1000000i64
    unsafe {
        printf("P0.1 image_png %dx%d mpx=%ld best_ms=%ld bytes=%ld\n", w, h, mpx, best, bytes as i64)
    }
    return 0
}

public func main() : int {
    var rc = bench(1920, 1080, 3)
    if(rc != 0) { return rc }
    rc = bench(3840, 2160, 3)
    if(rc != 0) { return rc }
    println("P0.1 image_png PASS")
    return 0
}
