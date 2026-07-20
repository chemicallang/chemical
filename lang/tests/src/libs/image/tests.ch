using std::Result;
using std::vector;

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
public func image_crop_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(20, 20)
    var cropped_result = image::image_crop(&raw mut img, 2, 2, 10, 10)
    if(cropped_result is Result.Err) { env.error("crop should succeed"); return }
    var Ok(cropped) = cropped_result else unreachable
    if(image::image_width(&raw mut cropped) != 10) { env.error("cropped width") }
    if(image::image_height(&raw mut cropped) != 10) { env.error("cropped height") }
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
public func image_rotate90_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(20, 10)
    var rotated = image::image_rotate90(&raw mut img)
    if(image::image_width(&raw mut rotated) != 10) { env.error("rotated width") }
    if(image::image_height(&raw mut rotated) != 20) { env.error("rotated height") }
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

// --- PNG tests ---

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
        var j : u32 = 0
        var c = crc
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

func build_tiny_uncompressed_png(width : int, height : int, channels : int, color_type : u8) : vector<u8> {
    // Build a minimal PNG: signature + IHDR + IDAT (stored deflate, no filter) + IEND
    var bpp = channels
    var row_bytes = width * bpp
    // raw deflate data: each row = filter byte (0) + row pixels, stored as uncompressed blocks
    var raw_scanlines = vector<u8>()
    var raw_size = (row_bytes + 1) * height
    raw_scanlines.resize(raw_size as size_t)
    var rp = raw_scanlines.data() as *mut u8
    var y : int = 0
    while(y < height) {
        var row_off = (y as size_t) * (row_bytes as size_t + 1)
        rp[row_off] = 0  // filter None
        var c : size_t = 0
        while(c < row_bytes as size_t) {
            // simple pattern: gradient
            rp[row_off + 1 + c] = ((y * channels + c) % 256) as u8
            c += 1
        }
        y += 1
    }

    // zlib compress (stored mode): CMF=0x78 FLG=0x01, then deflate stored blocks, then adler32
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
        while(i < blk) { cp[cpos] = rp[src_pos + i]; cpos += 1; i += 1 }
        src_pos += blk
    }
    // adler32
    var a : u32 = 1u32; var b : u32 = 0u32
    src_pos = 0
    while(src_pos < raw_size) {
        a = (a + (rp[src_pos] as u32)) % 65521u32
        b = (b + a) % 65521u32
        src_pos += 1
    }
    var chk = (b << 16) | a
    png_write_be32(cp, cpos, chk); cpos += 4

    // IHDR data (13 bytes)
    var ihdr : [13]u8
    ihdr[0] = ((width as u32) >> 24) as u8; ihdr[1] = ((width as u32) >> 16) as u8
    ihdr[2] = ((width as u32) >> 8) as u8; ihdr[3] = (width as u32) as u8
    ihdr[4] = ((height as u32) >> 24) as u8; ihdr[5] = ((height as u32) >> 16) as u8
    ihdr[6] = ((height as u32) >> 8) as u8; ihdr[7] = (height as u32) as u8
    ihdr[8] = 8; ihdr[9] = color_type
    ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0

    // calculate total size
    var total = 8 + (4 + 4 + 13 + 4) + (4 + 4 + cpos + 4) + (4 + 4 + 0 + 4)
    var png = vector<u8>()
    png.resize(total as size_t)
    var fp = png.data() as *mut u8
    var wp : size_t = 0

    // signature
    var sig : [8]u8 = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]
    var si : int = 0
    while(si < 8) { fp[si] = sig[si]; si += 1 }
    wp = 8

    build_test_png_chunk(fp, &raw mut wp, 'I', 'H', 'D', 'R', &raw ihdr[0], 13)
    build_test_png_chunk(fp, &raw mut wp, 'I', 'D', 'A', 'T', cp, cpos)
    build_test_png_chunk(fp, &raw mut wp, 'I', 'E', 'N', 'D', null, 0)

    return png
}

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
public func image_parse_png_pixel_values(env : &mut TestEnv) {
    // Build a 2x1 RGB image: pixel 0 = (10,20,30), pixel 1 = (40,50,60)
    // Row 0: filter=0, [10,20,30,40,50,60]
    var channels = 3
    var width = 2
    var height = 1
    var raw_size = (width * channels + 1) * height
    var raw_buf = vector<u8>()
    raw_buf.resize(raw_size as size_t)
    var rp = raw_buf.data() as *mut u8
    rp[0] = 0  // filter none
    rp[1] = 10; rp[2] = 20; rp[3] = 30
    rp[4] = 40; rp[5] = 50; rp[6] = 60

    // zlib compress (stored mode)
    var deflate_cap = raw_size + 16
    var compressed = vector<u8>()
    compressed.resize(deflate_cap as size_t)
    var cp = compressed.data() as *mut u8
    var cpos : size_t = 0
    cp[cpos] = 0x78u8; cp[cpos + 1] = 0x01u8; cpos += 2
    cp[cpos] = 1u8; cpos += 1  // bfinal=1
    cp[cpos] = (raw_size & 0xFFu16) as u8; cp[cpos + 1] = ((raw_size >> 8) & 0xFFu16) as u8
    var nlen = raw_size ^ 0xFFFFu16
    cp[cpos + 2] = (nlen & 0xFFu16) as u8; cp[cpos + 3] = ((nlen >> 8) & 0xFFu16) as u8
    cpos += 4
    var i : size_t = 0
    while(i < raw_size as size_t) { cp[cpos] = rp[i]; cpos += 1; i += 1 }
    // adler32
    var a : u32 = 1u32; var b : u32 = 0u32
    i = 0
    while(i < raw_size as size_t) { a = (a + (rp[i] as u32)) % 65521u32; b = (b + a) % 65521u32; i += 1 }
    var chk = (b << 16) | a
    png_write_be32(cp, cpos, chk); cpos += 4

    var ihdr : [13]u8
    png_write_be32(&raw mut ihdr[0], 0, width as u32)
    png_write_be32(&raw mut ihdr[0], 4, height as u32)
    ihdr[8] = 8; ihdr[9] = 2u8; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0

    var total = 8 + 25 + (8 + 4 + cpos + 4) + 12
    var png = vector<u8>()
    png.resize(total as size_t)
    var fp = png.data() as *mut u8
    var wp : size_t = 0
    var sig : [8]u8 = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]
    var si : int = 0
    while(si < 8) { fp[si] = sig[si]; si += 1 }
    wp = 8
    build_test_png_chunk(fp, &raw mut wp, 'I', 'H', 'D', 'R', &raw ihdr[0], 13)
    build_test_png_chunk(fp, &raw mut wp, 'I', 'D', 'A', 'T', cp, cpos)
    build_test_png_chunk(fp, &raw mut wp, 'I', 'E', 'N', 'D', null, 0)

    var result = image::parse_png(png.data(), png.size())
    if(result is Result.Err) { env.error("parse should succeed"); return }
    var Ok(img) = result else unreachable
    var px0 = image::image_get_rgba(&raw mut img, 0, 0)
    if(px0 is Result.Err) { env.error("get pixel 0"); return }
    var Ok(c0) = px0 else unreachable
    // RGB PNG returns 3 channels, so image_get_rgba fails — use direct access
    // For 3-channel image, verify raw pixel data directly
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
