using std::Result;
using std::vector;
using std::string;

// Focused tests for the additive safe image surface: `Image::load`,
// `from_png`/`from_bmp`/`from_ppm`, encoding, saving, pixel views, and
// transformation methods. Legacy `*mut Image` functions remain covered by
// the existing test files.

func image_safe_temp_pathbuf(name : *char) : path::PathBuf {
    var s = make_temp_test_path(name)
    return path::PathBuf.from(std::str.make_view(s.to_view()))
}

func image_safe_view(v : &vector<u8>) : [u8] {
    return unsafe(std::span<u8>(v.data(), v.size()))
}

func image_safe_fixed_view(p : *u8, n : size_t) : [u8] {
    return unsafe(std::span<u8>(p, n))
}

// ═══════════════════════════════════════════════════════════════
// Positive round-trips
// ═══════════════════════════════════════════════════════════════

@test
public func image_safe_png_roundtrip(env : &mut TestEnv) {
    var img = image::image_create_rgba(4, 3)
    var red = image::RGBA8.make(255, 0, 0, 255)
    var set = img.set(1, 1, red)
    if(set is Result.Err) { env.error("set should succeed"); return }

    var png = img.encode_png()
    if(png is Result.Err) { env.error("encode_png should succeed"); return }
    var Ok(bytes) = png else unsafe { unreachable }

    var loaded_result = image::Image.from_png(image_safe_view(&bytes))
    if(loaded_result is Result.Err) { env.error("from_png should succeed"); return }
    var Ok(loaded) = loaded_result else unsafe { unreachable }

    if(loaded.get_width() != 4) { env.error("width should be 4") }
    if(loaded.get_height() != 3) { env.error("height should be 3") }
    if(loaded.get_channels() != 4) { env.error("channels should be 4") }
    if(loaded.pixel_count() != 12) { env.error("pixel count should be 12") }

    var px = loaded.pixel(1, 1)
    if(px is Result.Err) { env.error("pixel should succeed"); return }
    var Ok(color) = px else unsafe { unreachable }
    if(color.r != 255 || color.g != 0 || color.b != 0 || color.a != 255) {
        env.error("round-tripped pixel mismatch")
    }
}

@test
public func image_safe_pixel_views(env : &mut TestEnv) {
    var img = image::image_create_rgba(2, 1)
    var view = img.pixels_view()
    if(view.size() != 8) { env.error("pixel view should have 8 bytes") }

    var mutable = img.pixels_mut_view()
    mutable.set(0, 123)
    mutable.set(1, 45)
    if(img.pixels.get(0) != 123) { env.error("pixels_mut_view write should be visible") }
    if(img.pixels.get(1) != 45) { env.error("pixels_mut_view second write") }
}

@test
public func image_safe_bmp_and_ppm_roundtrip(env : &mut TestEnv) {
    var img = image::image_create_rgb(3, 2)

    var bmp = img.encode_bmp()
    if(bmp is Result.Err) { env.error("encode_bmp should succeed"); return }
    var Ok(bmp_bytes) = bmp else unsafe { unreachable }
    var bmp_loaded = image::Image.from_bmp(image_safe_view(&bmp_bytes))
    if(bmp_loaded is Result.Err) { env.error("from_bmp should succeed"); return }
    var Ok(bmp_img) = bmp_loaded else unsafe { unreachable }
    if(bmp_img.get_width() != 3 || bmp_img.get_height() != 2) { env.error("BMP roundtrip dimensions") }

    var ppm = img.encode_ppm()
    if(ppm is Result.Err) { env.error("encode_ppm should succeed"); return }
    var Ok(ppm_bytes) = ppm else unsafe { unreachable }
    var ppm_loaded = image::Image.from_ppm(image_safe_view(&ppm_bytes))
    if(ppm_loaded is Result.Err) { env.error("from_ppm should succeed"); return }
    var Ok(ppm_img) = ppm_loaded else unsafe { unreachable }
    if(ppm_img.get_width() != 3 || ppm_img.get_channels() != 3) { env.error("PPM roundtrip dimensions") }
}

@test
public func image_safe_from_bytes_sniffs_formats(env : &mut TestEnv) {
    var img = image::image_create_rgba(2, 2)
    var png = img.encode_png()
    if(png is Result.Err) { env.error("encode_png"); return }
    var Ok(png_bytes) = png else unsafe { unreachable }
    var sniffed = image::Image.from_bytes(image_safe_view(&png_bytes))
    if(sniffed is Result.Err) { env.error("from_bytes should detect PNG") }

    var bmp = img.encode_bmp()
    if(bmp is Result.Err) { env.error("encode_bmp"); return }
    var Ok(bmp_bytes) = bmp else unsafe { unreachable }
    var sniffed_bmp = image::Image.from_bytes(image_safe_view(&bmp_bytes))
    if(sniffed_bmp is Result.Err) { env.error("from_bytes should detect BMP") }
}

@test
public func image_safe_load_and_save_png(env : &mut TestEnv) {
    var img = image::image_create_rgba(5, 4)
    var green = image::RGBA8.make(0, 255, 0, 255)
    var _ = img.set(2, 2, green)

    var p = image_safe_temp_pathbuf("p24_safe_image.png")
    var saved = img.save(&p)
    if(saved is Result.Err) { env.error("save(.png) should succeed"); return }

    var loaded = image::Image.load(&p)
    if(loaded is Result.Err) { env.error("load should succeed"); return }
    var Ok(loaded_img) = loaded else unsafe { unreachable }
    if(loaded_img.get_width() != 5 || loaded_img.get_height() != 4) { env.error("loaded dimensions") }

    var px = loaded_img.pixel(2, 2)
    if(px is Result.Err) { env.error("loaded pixel"); return }
    var Ok(color) = px else unsafe { unreachable }
    if(color.g != 255) { env.error("loaded pixel green channel") }
}

@test
public func image_safe_crop_flip_and_blit(env : &mut TestEnv) {
    var img = image::image_create_rgba(4, 4)
    var blue = image::RGBA8.make(0, 0, 255, 255)
    var _ = img.set(0, 0, blue)

    var cropped_result = img.crop(0, 0, 2, 2)
    if(cropped_result is Result.Err) { env.error("crop should succeed"); return }
    var Ok(cropped) = cropped_result else unsafe { unreachable }
    if(cropped.get_width() != 2 || cropped.get_height() != 2) { env.error("crop dimensions") }

    var before = img.pixel(0, 0)
    if(before is Result.Err) { env.error("pixel before flip"); return }
    var Ok(before_color) = before else unsafe { unreachable }

    img.flip_h()
    var after = img.pixel(3, 0)
    if(after is Result.Err) { env.error("pixel after flip_h"); return }
    var Ok(after_color) = after else unsafe { unreachable }
    if(after_color.b != before_color.b) { env.error("flip_h should move the blue pixel") }

    img.flip(true)
    img.flip_v()

    var dst = image::image_create_rgba(4, 4)
    var blitted = dst.blit(&cropped, 1, 1)
    if(blitted is Result.Err) { env.error("blit should succeed") }
}

// ═══════════════════════════════════════════════════════════════
// Misuse
// ═══════════════════════════════════════════════════════════════

@test
public func image_safe_bad_png_rejected(env : &mut TestEnv) {
    var garbage : [12]u8 = [0x89, 0x50, 0x4E, 0x47, 1, 2, 3, 4, 5, 6, 7, 8]
    var view = image_safe_fixed_view(&raw garbage[0], 12)
    var result = image::Image.from_png(view)
    if(result is Result.Ok) { env.error("truncated PNG must be rejected") }
}

@test
public func image_safe_bad_bmp_rejected(env : &mut TestEnv) {
    var garbage : [2]u8 = [0x42, 0x4D]
    var view = image_safe_fixed_view(&raw garbage[0], 2)
    var result = image::Image.from_bmp(view)
    if(result is Result.Ok) { env.error("truncated BMP must be rejected") }
}

@test
public func image_safe_bad_ppm_rejected(env : &mut TestEnv) {
    var garbage : [3]u8 = [0x50, 0x39, 0x0A]
    var view = image_safe_fixed_view(&raw garbage[0], 3)
    var result = image::Image.from_ppm(view)
    if(result is Result.Ok) { env.error("P9 PPM must be rejected") }
}

@test
public func image_safe_unknown_bytes_rejected(env : &mut TestEnv) {
    var garbage : [4]u8 = [1, 2, 3, 4]
    var view = image_safe_fixed_view(&raw garbage[0], 4)
    var result = image::Image.from_bytes(view)
    if(result is Result.Ok) { env.error("unknown format must be rejected") }
}

@test
public func image_safe_invalid_crop_and_pixel(env : &mut TestEnv) {
    var img = image::image_create_rgba(4, 4)

    var bad_crop = img.crop(0, 0, 99, 99)
    if(bad_crop is Result.Ok) { env.error("out-of-range crop must fail") }

    var bad_pixel = img.get(-1, 0)
    if(bad_pixel is Result.Ok) { env.error("negative pixel must fail") }

    var bad_set = img.set(4, 0, image::RGBA8.make(1, 2, 3, 4))
    if(bad_set is Result.Ok) { env.error("out-of-range set must fail") }
}

@test
public func image_safe_unsupported_save_extension(env : &mut TestEnv) {
    var img = image::image_create_rgba(2, 2)
    var p = image_safe_temp_pathbuf("p24_safe_image.xyz")
    var result = img.save(&p)
    if(result is Result.Ok) { env.error("unknown extension should fail") }
}
