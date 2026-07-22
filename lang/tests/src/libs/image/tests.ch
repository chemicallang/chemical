using std::Result;
using std::vector;
using std::string;

// ═══════════════════════════════════════════════════════════════
// Core image creation
// ═══════════════════════════════════════════════════════════════

@test
public func image_create_rgba_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(100, 50)
    if(image::image_width(&raw mut img) != 100) { env.error("width should be 100") }
    if(image::image_height(&raw mut img) != 50) { env.error("height should be 50") }
    if(image::image_channels(&raw mut img) != 4) { env.error("channels should be 4") }
    if(image::image_total_bytes(&raw mut img) != 20000) { env.error("total bytes should be 20000") }
}

@test
public func image_create_rgb_works(env : &mut TestEnv) {
    var img = image::image_create_rgb(80, 60)
    if(image::image_width(&raw mut img) != 80) { env.error("width") }
    if(image::image_height(&raw mut img) != 60) { env.error("height") }
    if(image::image_channels(&raw mut img) != 3) { env.error("channels") }
}

@test
public func image_create_gray_works(env : &mut TestEnv) {
    var img = image::image_create_gray(200, 100)
    if(image::image_channels(&raw mut img) != 1) { env.error("should be grayscale") }
}

@test
public func image_create_direct_works(env : &mut TestEnv) {
    var img = image::image_create(50, 30, 2)
    if(image::image_width(&raw mut img) != 50) { env.error("direct width") }
    if(image::image_height(&raw mut img) != 30) { env.error("direct height") }
    if(image::image_channels(&raw mut img) != 2) { env.error("direct channels") }
}

@test
public func image_pixel_count_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(10, 10)
    if(image::image_pixel_count(&raw mut img) != 100) { env.error("pixel count") }
}

@test
public func image_bytes_per_row_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(10, 10)
    if(image::image_bytes_per_row(&raw mut img) != 40) { env.error("bytes per row for RGBA") }
    var rgb = image::image_create_rgb(10, 10)
    if(image::image_bytes_per_row(&raw mut rgb) != 30) { env.error("bytes per row for RGB") }
}

// ═══════════════════════════════════════════════════════════════
// Core pixel operations
// ═══════════════════════════════════════════════════════════════

@test
public func image_set_get_rgba_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(10, 10)
    var red = image::RGBA8.make(255, 0, 0, 255)
    var set_result = image::image_set_rgba(&raw mut img, 5, 5, red)
    if(set_result is Result.Err) { env.error("set should succeed"); return }
    var get_result = image::image_get_rgba(&raw mut img, 5, 5)
    if(get_result is Result.Err) { env.error("get should succeed"); return }
    var Ok(px) = get_result else unreachable
    if(px.r != 255) { env.error("red channel") }
    if(px.g != 0) { env.error("green channel") }
    if(px.b != 0) { env.error("blue channel") }
    if(px.a != 255) { env.error("alpha channel") }
}

@test
public func image_set_rgba_on_rgb_should_error(env : &mut TestEnv) {
    var img = image::image_create_rgb(10, 10)
    var red = image::RGBA8.make(255, 0, 0, 255)
    var result = image::image_set_rgba(&raw mut img, 0, 0, red)
    if(result is Result.Ok) { env.error("should fail on RGB image") }
}

@test
public func image_get_rgba_on_rgb_should_error(env : &mut TestEnv) {
    var img = image::image_create_rgb(10, 10)
    var result = image::image_get_rgba(&raw mut img, 0, 0)
    if(result is Result.Ok) { env.error("should fail on RGB image") }
}

@test
public func image_pixels_pointer_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(10, 10)
    var pix = image::image_pixels(&raw mut img)
    pix[0] = 128
    if(image::image_pixels(&raw mut img)[0] != 128) { env.error("pixel pointer") }
}

// ═══════════════════════════════════════════════════════════════
// Image operations
// ═══════════════════════════════════════════════════════════════

@test
public func image_fill_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(10, 10)
    var white = image::RGBA8.make(255, 255, 255, 255)
    image::image_fill(&raw mut img, white)
    var px = image::image_get_rgba(&raw mut img, 0, 0)
    if(px is Result.Err) { env.error("get should succeed"); return }
    var Ok(c) = px else unreachable
    if(c.r != 255) { env.error("filled red") }
    if(c.g != 255) { env.error("filled green") }
}

@test
public func image_fill_rgb_works(env : &mut TestEnv) {
    var img = image::image_create_rgb(10, 10)
    var white = image::RGBA8.make(255, 128, 64, 255)
    image::image_fill(&raw mut img, white)
    var pix = image::image_pixels(&raw mut img)
    if(pix[0] != 255) { env.error("fill RGB red") }
    if(pix[1] != 128) { env.error("fill RGB green") }
    if(pix[2] != 64) { env.error("fill RGB blue") }
}

@test
public func image_fill_gray_works(env : &mut TestEnv) {
    var img = image::image_create_gray(10, 10)
    var white = image::RGBA8.make(200, 100, 50, 255)
    image::image_fill(&raw mut img, white)
    var pix = image::image_pixels(&raw mut img)
    if(pix[0] != 200) { env.error("fill gray value") }
}

@test
public func image_copy_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(10, 10)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_set_rgba(&raw mut img, 3, 3, red)
    var copy = image::image_copy(&raw mut img)
    if(image::image_width(&raw mut copy) != 10) { env.error("copy width") }
    var px = image::image_get_rgba(&raw mut copy, 3, 3)
    if(px is Result.Err) { env.error("copied pixel should match"); return }
    var Ok(c) = px else unreachable
    if(c.r != 255) { env.error("copied pixel should match") }
}

@test
public func image_copy_independent(env : &mut TestEnv) {
    var img = image::image_create_rgba(5, 5)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_set_rgba(&raw mut img, 0, 0, red)
    var copy = image::image_copy(&raw mut img)
    // Modify original
    var black = image::RGBA8.make(0, 0, 0, 0)
    image::image_set_rgba(&raw mut img, 0, 0, black)
    // Copy should still have red
    var px = image::image_get_rgba(&raw mut copy, 0, 0)
    if(px is Result.Ok) {
        var Ok(c) = px else unreachable
        if(c.r != 255) { env.error("copy should be independent") }
    }
}

@test
public func image_crop_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(20, 20)
    var cropped_result = image::image_crop(&raw mut img, 2, 2, 10, 10)
    if(cropped_result is Result.Err) { env.error("crop should succeed"); return }
    var Ok(cropped) = cropped_result else unreachable
    if(image::image_width(&raw mut cropped) != 10) { env.error("cropped width") }
    if(image::image_height(&raw mut cropped) != 10) { env.error("cropped height") }
}

@test
public func image_crop_negative_origin_fails(env : &mut TestEnv) {
    var img = image::image_create_rgba(20, 20)
    var result = image::image_crop(&raw mut img, -1, -1, 10, 10)
    if(result is Result.Ok) { env.error("crop with negative origin should fail") }
}

@test
public func image_crop_beyond_bounds_fails(env : &mut TestEnv) {
    var img = image::image_create_rgba(20, 20)
    var result = image::image_crop(&raw mut img, 15, 15, 10, 10)
    if(result is Result.Ok) { env.error("crop beyond bounds should fail") }
}

@test
public func image_flip_h_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(10, 10)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_set_rgba(&raw mut img, 0, 5, red)
    image::image_flip_h(&raw mut img)
    var px = image::image_get_rgba(&raw mut img, 9, 5)
    if(px is Result.Err) { env.error("flipped horizontally"); return }
    var Ok(c) = px else unreachable
    if(c.r != 255) { env.error("flipped horizontally") }
}

@test
public func image_flip_v_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(4, 4)
    var r = image::RGBA8.make(255, 0, 0, 255)
    image::image_set_rgba(&raw mut img, 0, 0, r)
    image::image_flip_v(&raw mut img)
    var px = image::image_get_rgba(&raw mut img, 0, 3)
    if(px is Result.Ok) {
        var Ok(color) = px else unreachable
        if(color.r != 255 as u8) { env.error("flipped pixel should be red") }
    }
}

@test
public func image_rotate90_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(20, 10)
    var rotated = image::image_rotate90(&raw mut img)
    if(image::image_width(&raw mut rotated) != 10) { env.error("rotated width") }
    if(image::image_height(&raw mut rotated) != 20) { env.error("rotated height") }
}

@test
public func image_rotate90_preserves_pixel(env : &mut TestEnv) {
    var img = image::image_create_rgba(5, 5)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_set_rgba(&raw mut img, 0, 0, red)
    var rotated = image::image_rotate90(&raw mut img)
    // Pixel at (0,0) should move to (0,4) after 90° rotation
    var px = image::image_get_rgba(&raw mut rotated, 0, 4)
    if(px is Result.Ok) {
        var Ok(c) = px else unreachable
        if(c.r != 255) { env.error("rotated pixel") }
    }
}

@test
public func image_line_draws(env : &mut TestEnv) {
    var img = image::image_create_rgba(100, 100)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_line(&raw mut img, 0, 0, 99, 99, red)
    var px = image::image_get_rgba(&raw mut img, 50, 50)
    if(px is Result.Err) { env.error("line should pass through center"); return }
    var Ok(c) = px else unreachable
    if(c.r != 255) { env.error("line should pass through center") }
}

@test
public func image_line_horizontal(env : &mut TestEnv) {
    var img = image::image_create_rgba(50, 1)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_line(&raw mut img, 0, 0, 49, 0, red)
    var px = image::image_get_rgba(&raw mut img, 25, 0)
    if(px is Result.Err) { env.error("hline middle"); return }
    var Ok(c) = px else unreachable
    if(c.r != 255) { env.error("hline should be red") }
}

@test
public func image_line_vertical(env : &mut TestEnv) {
    var img = image::image_create_rgba(1, 50)
    var blue = image::RGBA8.make(0, 0, 255, 255)
    image::image_line(&raw mut img, 0, 0, 0, 49, blue)
    var px = image::image_get_rgba(&raw mut img, 0, 25)
    if(px is Result.Err) { env.error("vline middle"); return }
    var Ok(c) = px else unreachable
    if(c.b != 255) { env.error("vline should be blue") }
}

@test
public func image_fill_rect_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(100, 100)
    var blue = image::RGBA8.make(0, 0, 255, 255)
    image::image_fill_rect(&raw mut img, 10, 10, 20, 20, blue)
    var px = image::image_get_rgba(&raw mut img, 15, 15)
    if(px is Result.Err) { env.error("filled rect should have blue"); return }
    var Ok(c) = px else unreachable
    if(c.b != 255) { env.error("filled rect should have blue") }
    var outside = image::image_get_rgba(&raw mut img, 5, 5)
    if(outside is Result.Err) { env.error("outside should be empty"); return }
    var Ok(o) = outside else unreachable
    if(o.b != 0) { env.error("outside should be empty") }
}

@test
public func image_rectangle_outline_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(20, 20)
    var green = image::RGBA8.make(0, 255, 0, 255)
    image::image_rectangle(&raw mut img, 2, 2, 10, 10, green)
    // Top edge should be green
    var px = image::image_get_rgba(&raw mut img, 5, 2)
    if(px is Result.Ok) {
        var Ok(c) = px else unreachable
        if(c.g != 255) { env.error("rectangle top edge") }
    }
    // Inside should NOT be green
    var inside = image::image_get_rgba(&raw mut img, 5, 5)
    if(inside is Result.Ok) {
        var Ok(c2) = inside else unreachable
        if(c2.g == 255) { env.error("rectangle inside should be empty") }
    }
}

@test
public func image_circle_draws(env : &mut TestEnv) {
    var img = image::image_create_rgba(100, 100)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_circle(&raw mut img, 50, 50, 20, red)
    var px = image::image_get_rgba(&raw mut img, 50, 30)
    if(px is Result.Err) { env.error("circle top should have pixel"); return }
    var Ok(c) = px else unreachable
    if(c.r != 255) { env.error("circle top should have pixel") }
}

@test
public func image_fill_circle_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(32, 32)
    var white = image::RGBA8.make(255, 255, 255, 255)
    image::image_fill_circle(&raw mut img, 16, 16, 10, white)
    var px = image::image_get_rgba(&raw mut img, 16, 16)
    if(px is Result.Ok) {
        var Ok(color) = px else unreachable
        if(color.r != 255 as u8) { env.error("circle center should be filled") }
    }
    var px2 = image::image_get_rgba(&raw mut img, 0, 0)
    if(px2 is Result.Ok) {
        var Ok(c2) = px2 else unreachable
        if(c2.r == 255 as u8 && c2.g == 255 as u8 && c2.b == 255 as u8) {
            env.error("corner should not be filled")
        }
    }
}

@test
public func image_blit_works(env : &mut TestEnv) {
    var dst = image::image_create_rgba(100, 100)
    var src = image::image_create_rgba(10, 10)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_set_rgba(&raw mut src, 0, 0, red)
    var result = image::image_blit(&raw mut dst, &raw mut src, 5, 5)
    if(result is Result.Err) { env.error("blit should succeed"); return }
    var px = image::image_get_rgba(&raw mut dst, 5, 5)
    if(px is Result.Err) { env.error("blitted pixel"); return }
    var Ok(c) = px else unreachable
    if(c.r != 255) { env.error("blitted pixel") }
}

@test
public func image_blit_channel_mismatch(env : &mut TestEnv) {
    var dst = image::image_create_rgba(32, 32)
    var src = image::image_create_rgb(10, 10)
    var result = image::image_blit(&raw mut dst, &raw mut src, 0, 0)
    if(result is Result.Ok) { env.error("blit with mismatched channels should fail") }
}

@test
public func image_blit_mismatched_sizes(env : &mut TestEnv) {
    var dst = image::image_create_rgba(32, 32)
    var src = image::image_create_rgba(64, 64)
    var result = image::image_blit(&raw mut dst, &raw mut src, 0, 0)
    if(result is Result.Err) { env.error("blit with larger src should succeed (clipped)") }
}

// ═══════════════════════════════════════════════════════════════
// Edge cases
// ═══════════════════════════════════════════════════════════════

@test
public func image_pixel_out_of_bounds(env : &mut TestEnv) {
    var img = image::image_create_rgba(8, 8)
    var white = image::RGBA8.make(255, 255, 255, 255)
    var result = image::image_set_rgba(&raw mut img, 100, 100, white)
    if(result is Result.Ok) { env.error("out-of-bounds should fail") }
}

@test
public func image_get_rgba_out_of_bounds(env : &mut TestEnv) {
    var img = image::image_create_rgba(4, 4)
    var result = image::image_get_rgba(&raw mut img, 99, 99)
    if(result is Result.Ok) { env.error("get out-of-bounds should fail") }
}

@test
public func image_crop_invalid_region(env : &mut TestEnv) {
    var img = image::image_create_rgba(32, 32)
    var result = image::image_crop(&raw mut img, 0, 0, 0, 10)
    if(result is Result.Ok) { env.error("crop with zero width should fail") }
}

@test
public func image_rectangle_on_non_rgba(env : &mut TestEnv) {
    var img = image::image_create_rgb(16, 16)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_rectangle(&raw mut img, 2, 2, 10, 10, red)
}

@test
public func image_error_messages_work(env : &mut TestEnv) {
    var err1 = image::ImageError.FileNotFound()
    var msg1 = err1.message()
    if(msg1.size() == 0) { env.error("FileNotFound message empty") }

    var err2 = image::ImageError.InvalidFormat(std::string("bad"))
    var msg2 = err2.message()
    if(msg2.size() == 0) { env.error("InvalidFormat message empty") }

    var err3 = image::ImageError.UnsupportedFormat(std::string("xyz"))
    var msg3 = err3.message()
    if(msg3.size() == 0) { env.error("UnsupportedFormat message empty") }

    var err4 = image::ImageError.IoError(std::string("disk"))
    var msg4 = err4.message()
    if(msg4.size() == 0) { env.error("IoError message empty") }

    var err5 = image::ImageError.InvalidDimensions(100, 200)
    var msg5 = err5.message()
    if(msg5.size() == 0) { env.error("InvalidDimensions message empty") }

    var err6 = image::ImageError.PixelOutOfBounds(10, 20)
    var msg6 = err6.message()
    if(msg6.size() == 0) { env.error("PixelOutOfBounds message empty") }
}

// ═══════════════════════════════════════════════════════════════
// Helper utilities (self-contained, don't depend on library internals)
// ═══════════════════════════════════════════════════════════════

func write_u32_le_util(out : *mut u8, offset : size_t, val : u32) {
    out[offset] = (val & 0xFFu32) as u8
    out[offset + 1] = ((val >> 8) & 0xFFu32) as u8
    out[offset + 2] = ((val >> 16) & 0xFFu32) as u8
    out[offset + 3] = ((val >> 24) & 0xFFu32) as u8
}

func write_u16_le_util(out : *mut u8, offset : size_t, val : u16) {
    out[offset] = (val & 0xFFu16) as u8
    out[offset + 1] = ((val >> 8) & 0xFFu16) as u8
}

// ═══════════════════════════════════════════════════════════════
// PNG infrastructure
// ═══════════════════════════════════════════════════════════════

func png_write_be32(out : *mut u8, off : size_t, val : u32) {
    out[off] = ((val >> 24) & 0xFFu32) as u8
    out[off + 1] = ((val >> 16) & 0xFFu32) as u8
    out[off + 2] = ((val >> 8) & 0xFFu32) as u8
    out[off + 3] = (val & 0xFFu32) as u8
}

func png_crc32_for_test(data : *u8, data_len : size_t) : u32 {
    var crc : u32 = 0xFFFFFFFFu32
    var i : size_t = 0
    while(i < data_len) {
        var idx = (crc ^ (data[i] as u32)) & 0xFFu32
        var c : u32 = idx
        var j : u32 = 0
        while(j < 8) {
            if(c & 1u32 != 0) { c = (c >> 1) ^ 0xEDB88320u32 } else { c = c >> 1 }
            j += 1
        }
        crc = c ^ (crc >> 8)
        i += 1
    }
    return crc ^ 0xFFFFFFFFu32
}

func build_test_png_chunk(out : *mut u8, pos : *mut size_t, type0 : u8, type1 : u8, type2 : u8, type3 : u8, data : *u8, data_len : size_t) {
    png_write_be32(out, pos[0], data_len as u32)
    pos[0] += 4
    out[pos[0]] = type0
    out[pos[0] + 1] = type1
    out[pos[0] + 2] = type2
    out[pos[0] + 3] = type3
    pos[0] += 4
    var i : size_t = 0
    while(i < data_len) {
        out[pos[0] + i] = data[i]
        i += 1
    }
    pos[0] += data_len
    var crc = png_crc32_for_test(out + pos[0] - data_len - 4, data_len + 4)
    png_write_be32(out, pos[0], crc)
    pos[0] += 4
}

func build_tiny_png_generic(width : int, height : int, bpp : int, color_type : u8, raw_data : *u8, raw_len : size_t, do_compress : bool) : vector<u8> {
    var raw_size = raw_len
    var deflate_cap = raw_size + (raw_size / 65535 + 1) * 5 + 10
    var compressed = vector<u8>()
    compressed.resize(deflate_cap as size_t)
    var cp = compressed.data() as *mut u8
    var cpos : size_t = 0
    cp[cpos] = 0x78u8; cp[cpos + 1] = 0x01u8; cpos += 2

    var src_pos : size_t = 0
    while(src_pos < raw_size as size_t) {
        var blk : size_t = (raw_size as size_t) - src_pos
        if(blk > 65535) { blk = 65535 }
        var last = (src_pos + blk >= raw_size)
        var bfinal : u8 = 0
        if(last) { bfinal = 1 }
        cp[cpos] = bfinal; cpos += 1
        cp[cpos] = (blk & 0xFFu16) as u8; cp[cpos + 1] = ((blk >> 8) & 0xFFu16) as u8
        var nlen = blk ^ 0xFFFFu16
        cp[cpos + 2] = (nlen & 0xFFu16) as u8; cp[cpos + 3] = ((nlen >> 8) & 0xFFu16) as u8
        cpos += 4
        var i : size_t = 0
        while(i < blk) { cp[cpos] = raw_data[src_pos + i]; cpos += 1; i += 1 }
        src_pos += blk
    }
    // adler32
    var a : u32 = 1u32; var b : u32 = 0u32
    src_pos = 0
    while(src_pos < raw_size) {
        a = (a + (raw_data[src_pos] as u32)) % 65521u32
        b = (b + a) % 65521u32
        src_pos += 1
    }
    var chk = (b << 16) | a
    png_write_be32(cp, cpos, chk); cpos += 4

    // IHDR
    var ihdr : [13]u8
    png_write_be32(&raw mut ihdr[0], 0, width as u32)
    png_write_be32(&raw mut ihdr[0], 4, height as u32)
    ihdr[8] = 8; ihdr[9] = color_type; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0

    var total = 8 + 25 + (8 + 4 + cpos + 4) + 12
    var png = vector<u8>()
    png.resize(total as size_t)
    var fp = png.data() as *mut u8
    var wp : size_t = 0

    var sig : [8]u8 = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]
    var si : int = 0
    while(si < 8) { fp[si] = sig[si]; si += 1 }
    wp = 8

    build_test_png_chunk(fp, &raw mut wp, 'I', 'H', 'D', 'R', &raw mut ihdr[0], 13)
    build_test_png_chunk(fp, &raw mut wp, 'I', 'D', 'A', 'T', cp, cpos)
    build_test_png_chunk(fp, &raw mut wp, 'I', 'E', 'N', 'D', null, 0)
    return png
}

func build_tiny_uncompressed_png(width : int, height : int, channels : int, color_type : u8) : vector<u8> {
    var bpp = channels
    var row_bytes = width * bpp
    var raw_size = (row_bytes + 1) * height
    var raw_scanlines = vector<u8>()
    raw_scanlines.resize(raw_size as size_t)
    var rp = raw_scanlines.data() as *mut u8
    var y : int = 0
    while(y < height) {
        var row_off = (y as size_t) * (row_bytes as size_t + 1)
        rp[row_off] = 0
        var c : size_t = 0
        while(c < row_bytes as size_t) {
            rp[row_off + 1 + c] = ((y * channels + c) % 256) as u8
            c += 1
        }
        y += 1
    }
    return build_tiny_png_generic(width, height, bpp, color_type, raw_scanlines.data(), raw_size as size_t, true)
}

// ═══════════════════════════════════════════════════════════════
// PNG parsing tests
// ═══════════════════════════════════════════════════════════════

@test
public func image_parse_tiny_png_rgb(env : &mut TestEnv) {
    var png_data = build_tiny_uncompressed_png(4, 4, 3, 2u8)
    var result = image::parse_png(png_data.data(), png_data.size())
    if(result is Result.Err) { env.error("parse_png should succeed for RGB"); return }
    var Ok(img) = result else unreachable
    if(image::image_width(&raw mut img) != 4) { env.error("width should be 4") }
    if(image::image_height(&raw mut img) != 4) { env.error("height should be 4") }
    if(image::image_channels(&raw mut img) != 3) { env.error("channels should be 3") }
}

@test
public func image_parse_tiny_png_rgba(env : &mut TestEnv) {
    var png_data = build_tiny_uncompressed_png(3, 3, 4, 6u8)
    var result = image::parse_png(png_data.data(), png_data.size())
    if(result is Result.Err) { env.error("parse_png should succeed for RGBA"); return }
    var Ok(img) = result else unreachable
    if(image::image_width(&raw mut img) != 3) { env.error("width should be 3") }
    if(image::image_height(&raw mut img) != 3) { env.error("height should be 3") }
    if(image::image_channels(&raw mut img) != 4) { env.error("channels should be 4") }
}

@test
public func image_parse_tiny_png_grayscale(env : &mut TestEnv) {
    var png_data = build_tiny_uncompressed_png(5, 2, 1, 0u8)
    var result = image::parse_png(png_data.data(), png_data.size())
    if(result is Result.Err) { env.error("parse_png should succeed for grayscale"); return }
    var Ok(img) = result else unreachable
    if(image::image_width(&raw mut img) != 5) { env.error("width should be 5") }
    if(image::image_height(&raw mut img) != 2) { env.error("height should be 2") }
    if(image::image_channels(&raw mut img) != 1) { env.error("channels should be 1") }
}

@test
public func image_parse_tiny_png_grayscale_alpha(env : &mut TestEnv) {
    // Color type 4 = grayscale + alpha (2 channels)
    var png_data = build_tiny_uncompressed_png(4, 3, 2, 4u8)
    var result = image::parse_png(png_data.data(), png_data.size())
    if(result is Result.Err) { env.error("parse_png should succeed for gray+alpha"); return }
    var Ok(img) = result else unreachable
    if(image::image_width(&raw mut img) != 4) { env.error("width should be 4") }
    if(image::image_height(&raw mut img) != 3) { env.error("height should be 3") }
    if(image::image_channels(&raw mut img) != 2) { env.error("channels should be 2") }
}

@test
public func image_parse_png_pixel_values(env : &mut TestEnv) {
    var channels = 3
    var width = 2
    var height = 1
    var raw_size = (width * channels + 1) * height
    var raw_buf = vector<u8>()
    raw_buf.resize(raw_size as size_t)
    var rp = raw_buf.data() as *mut u8
    rp[0] = 0
    rp[1] = 10; rp[2] = 20; rp[3] = 30
    rp[4] = 40; rp[5] = 50; rp[6] = 60

    var png = build_tiny_png_generic(width, height, channels, 2u8, raw_buf.data(), raw_size as size_t, true)
    var result = image::parse_png(png.data(), png.size())
    if(result is Result.Err) { env.error("parse should succeed"); return }
    var Ok(img) = result else unreachable
    var pix = image::image_pixels(&raw mut img)
    if(pix[0] != 10) { env.error("pixel 0 R should be 10") }
    if(pix[1] != 20) { env.error("pixel 0 G should be 20") }
    if(pix[2] != 30) { env.error("pixel 0 B should be 30") }
    if(pix[3] != 40) { env.error("pixel 1 R should be 40") }
    if(pix[4] != 50) { env.error("pixel 1 G should be 50") }
    if(pix[5] != 60) { env.error("pixel 1 B should be 60") }
}

@test
public func image_png_invalid_signature(env : &mut TestEnv) {
    var bad : [8]u8 = [0,0,0,0,0,0,0,0]
    var result = image::parse_png(&raw bad[0], 8)
    if(result is Result.Ok) { env.error("should fail on bad signature") }
}

@test
public func image_png_too_short(env : &mut TestEnv) {
    var bad : [4]u8 = [0,0,0,0]
    var result = image::parse_png(&raw bad[0], 4)
    if(result is Result.Ok) { env.error("should fail on too-short data") }
}

@test
public func image_png_no_ihdr_fails(env : &mut TestEnv) {
    var bad : [12]u8 = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0,0,0,0]
    var result = image::parse_png(&raw bad[0], 12)
    if(result is Result.Ok) { env.error("should fail without IHDR") }
}

// ═══════════════════════════════════════════════════════════════
// PNG save + load roundtrip
// ═══════════════════════════════════════════════════════════════

@test
public func image_png_save_load_roundtrip(env : &mut TestEnv) {
    var img = image::image_create_rgba(32, 32)
    var white = image::RGBA8.make(255, 255, 255, 255)
    image::image_fill(&raw mut img, white)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_set_rgba(&raw mut img, 10, 10, red)

    var save_result = image::save_png(&raw mut img, "/tmp/test_save.png")
    if(save_result is Result.Err) { env.error("PNG save failed"); return }

    var load_result = image::load_png("/tmp/test_save.png")
    if(load_result is Result.Err) { env.error("PNG load failed"); return }
    var Ok(loaded) = load_result else unreachable
    if(image::image_width(&raw mut loaded) != 32) { env.error("width should be 32") }
    if(image::image_height(&raw mut loaded) != 32) { env.error("height should be 32") }

    var px = image::image_get_rgba(&raw mut loaded, 10, 10)
    if(px is Result.Ok) {
        var Ok(color) = px else unreachable
        if(color.r != 255 as u8) { env.error("red channel should be 255") }
    }
}

@test
public func image_load_png_nonexistent(env : &mut TestEnv) {
    var result = image::load_png("/tmp/nonexistent_file_xyz.png")
    if(result is Result.Ok) { env.error("should fail on nonexistent file") }
}

// ═══════════════════════════════════════════════════════════════
// BMP tests
// ═══════════════════════════════════════════════════════════════

func build_minimal_bmp(width : int, height : int, bpp : int) : vector<u8> {
    var row_size = (((bpp * width + 31) / 32) * 4)
    var pixel_data_size = row_size * height
    var file_size = 54 + pixel_data_size

    var file_data = vector<u8>()
    file_data.resize(file_size as size_t)
    var fptr = file_data.data() as *mut u8

    fptr[0] = 'B' as u8; fptr[1] = 'M' as u8
    write_u32_le_util(fptr, 2, file_size as u32)
    write_u32_le_util(fptr, 10, 54)
    write_u32_le_util(fptr, 14, 40)
    write_u32_le_util(fptr, 18, width as u32)
    write_u32_le_util(fptr, 22, height as u32)
    write_u16_le_util(fptr, 26, 1)
    write_u16_le_util(fptr, 28, bpp as u16)
    write_u32_le_util(fptr, 30, 0)

    return file_data
}

@test
public func image_bmp_invalid_signature(env : &mut TestEnv) {
    var bad : [2]u8 = [0x41 as u8, 0x42 as u8]
    var result = image::parse_bmp(&raw bad[0], 2)
    if(result is Result.Ok) { env.error("should fail on bad signature") }
}

@test
public func image_bmp_save_load_roundtrip(env : &mut TestEnv) {
    var img = image::image_create_rgba(16, 16)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_fill(&raw mut img, red)

    var save_result = image::save_bmp(&raw mut img, "/tmp/test_bmp.bmp")
    if(save_result is Result.Err) { env.error("BMP save failed"); return }

    var load_result = image::load_bmp("/tmp/test_bmp.bmp")
    if(load_result is Result.Err) { env.error("BMP load failed"); return }
    var Ok(loaded) = load_result else unreachable
    if(image::image_width(&raw mut loaded) != 16) { env.error("width should be 16") }
    if(image::image_height(&raw mut loaded) != 16) { env.error("height should be 16") }
}

@test
public func image_bmp_invalid_bit_count(env : &mut TestEnv) {
    var buf : [54]u8
    buf[0] = 0x42 as u8; buf[1] = 0x4D as u8
    buf[28] = 1 as u8
    var result = image::parse_bmp(&raw buf[0], 54)
    if(result is Result.Ok) { env.error("should fail on 1bpp BMP") }
}

@test
public func image_bmp_parse_32bit_rgba(env : &mut TestEnv) {
    var bmp = build_minimal_bmp(4, 4, 32)
    var result = image::parse_bmp(bmp.data(), bmp.size())
    if(result is Result.Err) { env.error("32-bit BMP should succeed"); return }
    var Ok(img) = result else unreachable
    if(image::image_channels(&raw mut img) != 4) { env.error("channels should be 4") }
}

@test
public func image_bmp_parse_8bit_gray(env : &mut TestEnv) {
    var bmp = build_minimal_bmp(4, 4, 8)
    var result = image::parse_bmp(bmp.data(), bmp.size())
    if(result is Result.Err) { env.error("8-bit BMP should succeed"); return }
    var Ok(img) = result else unreachable
    if(image::image_channels(&raw mut img) != 1) { env.error("channels should be 1") }
}

@test
public func image_bmp_too_small(env : &mut TestEnv) {
    var small : [10]u8
    var result = image::parse_bmp(&raw small[0], 10)
    if(result is Result.Ok) { env.error("should fail on <54 bytes") }
}

@test
public func image_bmp_negative_height_top_down(env : &mut TestEnv) {
    var bmp = build_minimal_bmp(4, 4, 24)
    write_u32_le_util(bmp.data() as *mut u8, 22, 0xFFFFFFFCu32)  // -4 as u32
    var result = image::parse_bmp(bmp.data(), bmp.size())
    if(result is Result.Err) { env.error("negative height (top-down) should work"); return }
}

@test
public func image_load_bmp_nonexistent(env : &mut TestEnv) {
    var result = image::load_bmp("/tmp/nonexistent_file_xyz.bmp")
    if(result is Result.Ok) { env.error("should fail on nonexistent file") }
}

// ═══════════════════════════════════════════════════════════════
// PPM tests
// ═══════════════════════════════════════════════════════════════

@test
public func image_ppm_invalid_magic(env : &mut TestEnv) {
    var bad : [3]u8 = [0x50 as u8, 0x39 as u8, 0x0A as u8]
    var result = image::parse_ppm(&raw bad[0], 3)
    if(result is Result.Ok) { env.error("should fail on P9 magic") }
}

@test
public func image_ppm_save_load_p6_roundtrip(env : &mut TestEnv) {
    var img = image::image_create_rgb(8, 8)
    var i : int = 0
    while(i < 8 * 8) {
        var p = image::image_pixels(&raw mut img)
        p[i * 3] = 0 as u8
        p[i * 3 + 1] = 255 as u8
        p[i * 3 + 2] = 0 as u8
        i += 1
    }

    var save_result = image::save_ppm(&raw mut img, "/tmp/test_ppm.ppm")
    if(save_result is Result.Err) { env.error("PPM save failed"); return }

    var load_result = image::load_ppm("/tmp/test_ppm.ppm")
    if(load_result is Result.Err) { env.error("PPM load failed"); return }
    var Ok(loaded) = load_result else unreachable
    if(image::image_width(&raw mut loaded) != 8) { env.error("width should be 8") }
    if(image::image_channels(&raw mut loaded) != 3) { env.error("should be 3 channels") }
}

@test
public func image_ppm_parse_p3_ascii(env : &mut TestEnv) {
    var ppm = string("")
    ppm.append('P'); ppm.append('3'); ppm.append(' ')
    ppm.append('2'); ppm.append(' '); ppm.append('2'); ppm.append(' ')
    ppm.append('2'); ppm.append('5'); ppm.append('5'); ppm.append(' ')
    ppm.append('1'); ppm.append('0'); ppm.append('0'); ppm.append(' ')
    ppm.append('2'); ppm.append('0'); ppm.append('0'); ppm.append(' ')
    ppm.append('3'); ppm.append('0'); ppm.append('0'); ppm.append(' ')
    ppm.append('4'); ppm.append('0'); ppm.append('0'); ppm.append(' ')
    ppm.append('5'); ppm.append('0'); ppm.append('0'); ppm.append(' ')
    ppm.append('6'); ppm.append('0'); ppm.append('0'); ppm.append(' ')
    ppm.append('7'); ppm.append('0'); ppm.append('0'); ppm.append(' ')
    ppm.append('8'); ppm.append('0'); ppm.append('0'); ppm.append(' ')
    ppm.append('9'); ppm.append('0'); ppm.append('0'); ppm.append(' ')
    ppm.append('1'); ppm.append('0'); ppm.append('0'); ppm.append(' ')
    ppm.append('1'); ppm.append('1'); ppm.append('0'); ppm.append(' ')
    ppm.append('1'); ppm.append('2'); ppm.append('0'); ppm.append(' ')

    var vec = vector<u8>()
    vec.resize(ppm.size())
    var i : size_t = 0
    while(i < ppm.size()) {
        vec.set(i, ppm.get(i) as u8)
        i += 1
    }

    var result = image::parse_ppm(vec.data(), vec.size())
    if(result is Result.Err) { env.error("P3 ASCII PPM should succeed"); return }
    var Ok(img) = result else unreachable
    if(image::image_width(&raw mut img) != 2) { env.error("P3 width") }
    if(image::image_height(&raw mut img) != 2) { env.error("P3 height") }
}

@test
public func image_save_ppm_non_rgb_fails(env : &mut TestEnv) {
    var img = image::image_create_rgba(4, 4)
    var result = image::save_ppm(&raw mut img, "/tmp/test_invalid.ppm")
    if(result is Result.Ok) { env.error("saving RGBA as PPM should fail") }
}

@test
public func image_load_ppm_nonexistent(env : &mut TestEnv) {
    var result = image::load_ppm("/tmp/nonexistent_file_xyz.ppm")
    if(result is Result.Ok) { env.error("should fail on nonexistent file") }
}
