// Canonical safe image surface: `&[u8]` decoding, `&path::Path` loading/saving,
// `Result` errors, and owned `Image` values. Use `Image.load`, `Image.from_*`,
// `Image.save`, and the `Image` accessors; the C-string/`*mut Image` entry
// points live under `image::raw`.
public namespace image {

using std::Result;
using std::string;
using std::vector;

// ── loading ───────────────────────────────────────────────────────────────

func load_from_path(path : &path::Path) : Result<Image, ImageError> {
    var read = fs::read(path)
    if(read is Result.Err) {
        return Result.Err(ImageError.FileNotFound())
    }
    var Ok(data) = read else unsafe { unreachable }
    return decode_any(data.data(), data.size())
}

func decode_any(data : *u8, len : size_t) : Result<Image, ImageError> {
    unsafe {
        if(len >= 8) {
            if(data[0] == 0x89 as u8 && data[1] == 'P' as u8 && data[2] == 'N' as u8 && data[3] == 'G' as u8) {
                return decode_png(data, len)
            }
        }
        if(len >= 2) {
            if(data[0] == 'B' as u8 && data[1] == 'M' as u8) {
                return decode_bmp(data, len)
            }
            if(data[0] == 'P' as u8 && data[1] >= '1' as u8 && data[1] <= '6' as u8) {
                return decode_ppm(data, len)
            }
        }
    }
    return Result.Err(ImageError.InvalidFormat(string("unrecognized image format")))
}

func decode_png(data : *u8, len : size_t) : Result<Image, ImageError> {
    return raw::parse_png(data, len)
}

func decode_bmp(data : *u8, len : size_t) : Result<Image, ImageError> {
    return raw::parse_bmp(data, len)
}

func decode_ppm(data : *u8, len : size_t) : Result<Image, ImageError> {
    return raw::parse_ppm(data, len)
}

// ── encoding ──────────────────────────────────────────────────────────────

func encode_png_bytes(img : &Image) : Result<vector<u8>, ImageError> {
    var check = img.validate()
    if(check is Result.Err) { var Err(e) = check else unsafe { unreachable }; return Result.Err<vector<u8>, ImageError>(e) }
    return raw::encode_png(unsafe(img as *mut Image))
}

func encode_bmp_bytes(img : &Image) : Result<vector<u8>, ImageError> {
    var check = img.validate()
    if(check is Result.Err) { var Err(e) = check else unsafe { unreachable }; return Result.Err<vector<u8>, ImageError>(e) }
    return raw::encode_bmp(unsafe(img as *mut Image))
}

func encode_ppm_bytes(img : &Image) : Result<vector<u8>, ImageError> {
    var check = img.validate()
    if(check is Result.Err) { var Err(e) = check else unsafe { unreachable }; return Result.Err<vector<u8>, ImageError>(e) }
    return raw::encode_ppm(unsafe(img as *mut Image))
}

// ── saving ────────────────────────────────────────────────────────────────

func save_to_path(img : &Image, path : &path::Path) : Result<std::Unit, ImageError> {
    var ext_path = path.extension()
    if(ext_path is std::Option.None) {
        return Result.Err(ImageError.UnsupportedFormat(string("expected .png, .bmp, or .ppm")))
    }
    var Some(ext_buf) = ext_path else unsafe { unreachable }
    var ext = ext_buf.as_view()

    var encoded : Result<vector<u8>, ImageError>
    if(ext.equals(std::string_view("png"))) {
        encoded = encode_png_bytes(img)
    } else if(ext.equals(std::string_view("bmp"))) {
        encoded = encode_bmp_bytes(img)
    } else if(ext.equals(std::string_view("ppm"))) {
        encoded = encode_ppm_bytes(img)
    } else {
        return Result.Err(ImageError.UnsupportedFormat(string("expected .png, .bmp, or .ppm")))
    }

    if(encoded is Result.Err) {
        var Err(e) = encoded else unsafe { unreachable }
        return Result.Err(e)
    }
    var Ok(bytes) = encoded else unsafe { unreachable }

    var view : [u8] = &bytes
    var written = fs::write(path, view)
    if(written is Result.Err) {
        return Result.Err(ImageError.IoError(string("failed to write image file")))
    }
    return Result.Ok(std::Unit{})
}

} // end namespace image
