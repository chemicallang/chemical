// Safe image surface: `&[u8]` decoding, `&path::Path` loading/saving,
// `Result` errors, and owned `Image` values. The legacy raw functions in
// `png.ch` / `bmp.ch` / `ppm.ch` remain available; new code should use the
// methods on `Image` or the helpers here (no `*mut Image` in the new surface).
public namespace image {

using std::Result;
using std::string;
using std::vector;

// ── loading ───────────────────────────────────────────────────────────────

func image_safe_load(path : &path::Path) : Result<Image, ImageError> {
    var ps = path.as_string()
    var read = fs::read_entire_file(ps.c_str())
    if(read is Result.Err) {
        return Result.Err(ImageError.FileNotFound())
    }
    var Ok(data) = read else unsafe { unreachable }
    return image_safe_from_bytes(data.data(), data.size())
}

func image_safe_from_bytes(data : *u8, len : size_t) : Result<Image, ImageError> {
    unsafe {
        if(len >= 8) {
            if(data[0] == 0x89 as u8 && data[1] == 'P' as u8 && data[2] == 'N' as u8 && data[3] == 'G' as u8) {
                return image_safe_from_png(data, len)
            }
        }
        if(len >= 2) {
            if(data[0] == 'B' as u8 && data[1] == 'M' as u8) {
                return image_safe_from_bmp(data, len)
            }
            if(data[0] == 'P' as u8 && data[1] >= '1' as u8 && data[1] <= '6' as u8) {
                return image_safe_from_ppm(data, len)
            }
        }
    }
    return Result.Err(ImageError.InvalidFormat(string("unrecognized image format")))
}

func image_safe_from_png(data : *u8, len : size_t) : Result<Image, ImageError> {
    return parse_png(data, len)
}

func image_safe_from_bmp(data : *u8, len : size_t) : Result<Image, ImageError> {
    return parse_bmp(data, len)
}

func image_safe_from_ppm(data : *u8, len : size_t) : Result<Image, ImageError> {
    return parse_ppm(data, len)
}

// ── encoding ──────────────────────────────────────────────────────────────

func image_safe_encode_png(img : &Image) : Result<vector<u8>, ImageError> {
    var check = img.validate()
    if(check is Result.Err) { var Err(e) = check else unsafe { unreachable }; return Result.Err<vector<u8>, ImageError>(e) }
    return encode_png(unsafe(img as *mut Image))
}

func image_safe_encode_bmp(img : &Image) : Result<vector<u8>, ImageError> {
    var check = img.validate()
    if(check is Result.Err) { var Err(e) = check else unsafe { unreachable }; return Result.Err<vector<u8>, ImageError>(e) }
    return encode_bmp(unsafe(img as *mut Image))
}

func image_safe_encode_ppm(img : &Image) : Result<vector<u8>, ImageError> {
    var check = img.validate()
    if(check is Result.Err) { var Err(e) = check else unsafe { unreachable }; return Result.Err<vector<u8>, ImageError>(e) }
    return encode_ppm(unsafe(img as *mut Image))
}

// ── saving ────────────────────────────────────────────────────────────────

func image_safe_save(img : &Image, path : &path::Path) : Result<std::Unit, ImageError> {
    var ext_buf = path.extension()
    var ext = ext_buf.as_view()

    var encoded : Result<vector<u8>, ImageError>
    if(ext.equals(std::string_view(".png"))) {
        encoded = image_safe_encode_png(img)
    } else if(ext.equals(std::string_view(".bmp"))) {
        encoded = image_safe_encode_bmp(img)
    } else if(ext.equals(std::string_view(".ppm"))) {
        encoded = image_safe_encode_ppm(img)
    } else {
        return Result.Err(ImageError.UnsupportedFormat(string("expected .png, .bmp, or .ppm")))
    }

    if(encoded is Result.Err) {
        var Err(e) = encoded else unsafe { unreachable }
        return Result.Err(e)
    }
    var Ok(bytes) = encoded else unsafe { unreachable }

    var ps = path.as_string()
    var written = fs::write_text_file(ps.c_str(), bytes.data(), bytes.size())
    if(written is Result.Err) {
        return Result.Err(ImageError.IoError(string("failed to write image file")))
    }
    return Result.Ok(std::Unit{})
}

} // end namespace image
