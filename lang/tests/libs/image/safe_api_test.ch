using std::Result;
using std::vector;
using std::string;

// Focused tests for the additive safe image surface: `Image::load`,
// `from_png`/`from_bmp`/`from_ppm`, encoding, saving, pixel views, and
// transformation methods. Legacy `*mut Image` functions remain covered by
// the existing test files.

func image_temp_pathbuf(name : *char) : path::PathBuf {
    var s = make_temp_test_path(name)
    return path::PathBuf.from(std::str.make_view(s.to_view()))
}

func image_view_of(v : &vector<u8>) : [u8] {
    return unsafe(std::span<u8>(v.data(), v.size()))
}

func image_fixed_view(p : *u8, n : size_t) : [u8] {
    return unsafe(std::span<u8>(p, n))
}

func image_new_image(width : int, height : int, channels : int) : image::Image {
    var created = image::Image.new(width, height, channels)
    var Ok(img) = created else unsafe { unreachable }
    return img
}

func image_new_rgba_image(width : int, height : int) : image::Image {
    var created = image::Image.new_rgba(width, height)
    var Ok(img) = created else unsafe { unreachable }
    return img
}

func image_new_rgb_image(width : int, height : int) : image::Image {
    var created = image::Image.new_rgb(width, height)
    var Ok(img) = created else unsafe { unreachable }
    return img
}

// ═══════════════════════════════════════════════════════════════
// Positive round-trips
// ═══════════════════════════════════════════════════════════════

@test
public func image_api_png_roundtrip(env : &mut TestEnv) {
    var img = image_new_rgba_image(4, 3)
    var red = image::RGBA8.make(255, 0, 0, 255)
    var set = img.set(1, 1, red)
    if(set is Result.Err) { env.error("set should succeed"); return }

    var png = img.encode_png()
    if(png is Result.Err) { env.error("encode_png should succeed"); return }
    var Ok(bytes) = png else unsafe { unreachable }

    var loaded_result = image::Image.from_png(image_view_of(&bytes))
    if(loaded_result is Result.Err) { env.error("from_png should succeed"); return }
    var Ok(loaded) = loaded_result else unsafe { unreachable }

    if(loaded.width() != 4) { env.error("width should be 4") }
    if(loaded.height() != 3) { env.error("height should be 3") }
    if(loaded.channels() != 4) { env.error("channels should be 4") }
    if(loaded.pixel_count() != 12) { env.error("pixel count should be 12") }

    var px = loaded.pixel(1, 1)
    if(px is Result.Err) { env.error("pixel should succeed"); return }
    var Ok(color) = px else unsafe { unreachable }
    if(color.r != 255 || color.g != 0 || color.b != 0 || color.a != 255) {
        env.error("round-tripped pixel mismatch")
    }
}

@test
public func image_api_pixel_views(env : &mut TestEnv) {
    var img = image_new_rgba_image(2, 1)
    var view = img.pixels_view()
    if(view.size() != 8) { env.error("pixel view should have 8 bytes") }

    var mutable = img.pixels_mut_view()
    mutable.set(0, 123)
    mutable.set(1, 45)
    if(img.pixels()[0] != 123) { env.error("pixels_mut_view write should be visible") }
    if(img.pixels()[1] != 45) { env.error("pixels_mut_view second write") }
}

@test
public func image_api_pixel_get_set_all_channel_counts(env : &mut TestEnv) {
    // 1 channel: RGB are synthesized from the gray value, alpha is 255.
    var gray = image_new_image(2, 1, 1)
    if(gray.set(0, 0, image::RGBA8.make(10, 20, 30, 40)) is Result.Err) { env.error("gray set should succeed") }
    var gp = gray.pixel(0, 0)
    if(gp is Result.Err) { env.error("gray get should succeed"); return }
    var Ok(gcol) = gp else unsafe { unreachable }
    if(gcol.r != 10 || gcol.g != 10 || gcol.b != 10 || gcol.a != 255) { env.error("gray expansion") }

    // 2 channels: gray + alpha.
    var ga = image_new_image(2, 1, 2)
    if(ga.set(0, 0, image::RGBA8.make(50, 60, 70, 80)) is Result.Err) { env.error("GA set should succeed") }
    var gap = ga.pixel(0, 0)
    if(gap is Result.Err) { env.error("GA get should succeed"); return }
    var Ok(gacol) = gap else unsafe { unreachable }
    if(gacol.r != 50 || gacol.g != 50 || gacol.b != 50 || gacol.a != 80) { env.error("GA expansion") }

    // 3 channels: RGB with synthesized opaque alpha.
    var rgb = image_new_image(2, 1, 3)
    if(rgb.set(0, 0, image::RGBA8.make(1, 2, 3, 4)) is Result.Err) { env.error("RGB set should succeed") }
    var rp = rgb.pixel(0, 0)
    if(rp is Result.Err) { env.error("RGB get should succeed"); return }
    var Ok(rcol) = rp else unsafe { unreachable }
    if(rcol.r != 1 || rcol.g != 2 || rcol.b != 3 || rcol.a != 255) { env.error("RGB expansion") }

    // 4 channels: exact RGBA.
    var rgba = image_new_image(2, 1, 4)
    if(rgba.set(0, 0, image::RGBA8.make(9, 8, 7, 6)) is Result.Err) { env.error("RGBA set should succeed") }
    var vap = rgba.pixel(0, 0)
    if(vap is Result.Err) { env.error("RGBA get should succeed"); return }
    var Ok(vcol) = vap else unsafe { unreachable }
    if(vcol.r != 9 || vcol.g != 8 || vcol.b != 7 || vcol.a != 6) { env.error("RGBA exact") }
}

@test
public func image_api_bmp_and_ppm_roundtrip(env : &mut TestEnv) {
    var img = image_new_rgb_image(3, 2)

    var bmp = img.encode_bmp()
    if(bmp is Result.Err) { env.error("encode_bmp should succeed"); return }
    var Ok(bmp_bytes) = bmp else unsafe { unreachable }
    var bmp_loaded = image::Image.from_bmp(image_view_of(&bmp_bytes))
    if(bmp_loaded is Result.Err) { env.error("from_bmp should succeed"); return }
    var Ok(bmp_img) = bmp_loaded else unsafe { unreachable }
    if(bmp_img.width() != 3 || bmp_img.height() != 2) { env.error("BMP roundtrip dimensions") }

    var ppm = img.encode_ppm()
    if(ppm is Result.Err) { env.error("encode_ppm should succeed"); return }
    var Ok(ppm_bytes) = ppm else unsafe { unreachable }
    var ppm_loaded = image::Image.from_ppm(image_view_of(&ppm_bytes))
    if(ppm_loaded is Result.Err) { env.error("from_ppm should succeed"); return }
    var Ok(ppm_img) = ppm_loaded else unsafe { unreachable }
    if(ppm_img.width() != 3 || ppm_img.channels() != 3) { env.error("PPM roundtrip dimensions") }
}

@test
public func image_api_from_bytes_sniffs_formats(env : &mut TestEnv) {
    var img = image_new_rgba_image(2, 2)
    var png = img.encode_png()
    if(png is Result.Err) { env.error("encode_png"); return }
    var Ok(png_bytes) = png else unsafe { unreachable }
    var sniffed = image::Image.from_bytes(image_view_of(&png_bytes))
    if(sniffed is Result.Err) { env.error("from_bytes should detect PNG") }

    var bmp = img.encode_bmp()
    if(bmp is Result.Err) { env.error("encode_bmp"); return }
    var Ok(bmp_bytes) = bmp else unsafe { unreachable }
    var sniffed_bmp = image::Image.from_bytes(image_view_of(&bmp_bytes))
    if(sniffed_bmp is Result.Err) { env.error("from_bytes should detect BMP") }
}

@test
public func image_api_load_and_save_png(env : &mut TestEnv) {
    var img = image_new_rgba_image(5, 4)
    var green = image::RGBA8.make(0, 255, 0, 255)
    var _ = img.set(2, 2, green)

    var p = image_temp_pathbuf("p24_safe_image.png")
    var saved = img.save(&p.as_path())
    if(saved is Result.Err) { env.error("save(.png) should succeed"); return }

    var loaded = image::Image.load(&p.as_path())
    if(loaded is Result.Err) { env.error("load should succeed"); return }
    var Ok(loaded_img) = loaded else unsafe { unreachable }
    if(loaded_img.width() != 5 || loaded_img.height() != 4) { env.error("loaded dimensions") }

    var px = loaded_img.pixel(2, 2)
    if(px is Result.Err) { env.error("loaded pixel"); return }
    var Ok(color) = px else unsafe { unreachable }
    if(color.g != 255) { env.error("loaded pixel green channel") }
}

@test
public func image_api_crop_flip_and_blit(env : &mut TestEnv) {
    var img = image_new_rgba_image(4, 4)
    var blue = image::RGBA8.make(0, 0, 255, 255)
    var _ = img.set(0, 0, blue)

    var cropped_result = img.crop(0, 0, 2, 2)
    if(cropped_result is Result.Err) { env.error("crop should succeed"); return }
    var Ok(cropped) = cropped_result else unsafe { unreachable }
    if(cropped.width() != 2 || cropped.height() != 2) { env.error("crop dimensions") }

    var before = img.pixel(0, 0)
    if(before is Result.Err) { env.error("pixel before flip"); return }
    var Ok(before_color) = before else unsafe { unreachable }

    var flipped = img.flip_h()
    if(flipped is Result.Err) { env.error("flip_h should succeed"); return }
    var after = img.pixel(3, 0)
    if(after is Result.Err) { env.error("pixel after flip_h"); return }
    var Ok(after_color) = after else unsafe { unreachable }
    if(after_color.b != before_color.b) { env.error("flip_h should move the blue pixel") }

    if(img.flip(true) is Result.Err) { env.error("flip(true) should succeed") }
    if(img.flip_v() is Result.Err) { env.error("flip_v should succeed") }

    var dst = image_new_rgba_image(4, 4)
    var blitted = dst.blit(&cropped, 1, 1)
    if(blitted is Result.Err) { env.error("blit should succeed") }
}

// ═══════════════════════════════════════════════════════════════
// Misuse
// ═══════════════════════════════════════════════════════════════

@test
public func image_api_bad_png_rejected(env : &mut TestEnv) {
    var garbage : [12]u8 = [0x89, 0x50, 0x4E, 0x47, 1, 2, 3, 4, 5, 6, 7, 8]
    var view = image_fixed_view(&raw garbage[0], 12)
    var result = image::Image.from_png(view)
    if(result is Result.Ok) { env.error("truncated PNG must be rejected") }
}

@test
public func image_api_bad_bmp_rejected(env : &mut TestEnv) {
    var garbage : [2]u8 = [0x42, 0x4D]
    var view = image_fixed_view(&raw garbage[0], 2)
    var result = image::Image.from_bmp(view)
    if(result is Result.Ok) { env.error("truncated BMP must be rejected") }
}

@test
public func image_api_bad_ppm_rejected(env : &mut TestEnv) {
    var garbage : [3]u8 = [0x50, 0x39, 0x0A]
    var view = image_fixed_view(&raw garbage[0], 3)
    var result = image::Image.from_ppm(view)
    if(result is Result.Ok) { env.error("P9 PPM must be rejected") }
}

@test
public func image_api_unknown_bytes_rejected(env : &mut TestEnv) {
    var garbage : [4]u8 = [1, 2, 3, 4]
    var view = image_fixed_view(&raw garbage[0], 4)
    var result = image::Image.from_bytes(view)
    if(result is Result.Ok) { env.error("unknown format must be rejected") }
}

@test
public func image_api_invalid_crop_and_pixel(env : &mut TestEnv) {
    var img = image_new_rgba_image(4, 4)

    var bad_crop = img.crop(0, 0, 99, 99)
    if(bad_crop is Result.Ok) { env.error("out-of-range crop must fail") }

    var bad_pixel = img.get(-1, 0)
    if(bad_pixel is Result.Ok) { env.error("negative pixel must fail") }

    var bad_set = img.set(4, 0, image::RGBA8.make(1, 2, 3, 4))
    if(bad_set is Result.Ok) { env.error("out-of-range set must fail") }
}

@test
public func image_api_unsupported_save_extension(env : &mut TestEnv) {
    var img = image_new_rgba_image(2, 2)
    var p = image_temp_pathbuf("p24_safe_image.xyz")
    var result = img.save(&p.as_path())
    if(result is Result.Ok) { env.error("unknown extension should fail") }
}

@test
public func new_image_rejects_invalid_dimensions(env : &mut TestEnv) {
    var zero_width = image::Image.new(0, 4, 4)
    if(zero_width is Result.Ok) { env.error("zero width must be rejected") }

    var zero_height = image::Image.new(4, 0, 4)
    if(zero_height is Result.Ok) { env.error("zero height must be rejected") }

    var negative = image::Image.new(-3, 4, 4)
    if(negative is Result.Ok) { env.error("negative dimensions must be rejected") }

    var bad_channels = image::Image.new(4, 4, 5)
    if(bad_channels is Result.Ok) { env.error("channel count outside 1..4 must be rejected") }

    var good = image::Image.new(4, 4, 4)
    if(good is Result.Err) { env.error("valid dimensions must be accepted"); return }
    var Ok(img) = good else unsafe { unreachable }
    if(img.width() != 4 || img.height() != 4 || img.channels() != 4) { env.error("constructor metadata") }
    if(img.pixels().size() != 64) { env.error("constructor buffer size") }
    if(img.validate() is Result.Err) { env.error("constructed image must validate") }
}
