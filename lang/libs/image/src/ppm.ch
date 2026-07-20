public namespace image {

using std::Result;
using std::string;
using std::vector;

public func load_ppm(path : *char) : std::Result<Image, ImageError> {
    var read_result = fs::read_entire_file(path)
    if(read_result is Result.Err) {
        return std.Result.Err(ImageError.FileNotFound())
    }
    var Ok(data) = read_result else unreachable
    return parse_ppm(data.data(), data.size())
}

func is_digit(c : u8) : bool {
    return c >= '0' as u8 && c <= '9' as u8
}

func parse_ppm_number(data : *u8, pos : *mut size_t, data_len : size_t) : std::Result<int, ImageError> {
    var val : int = 0
    var found = false
    while(pos[0] < data_len) {
        var c = data[pos[0]]
        if(is_digit(c)) {
            val = val * 10 + ((c - '0' as u8) as int)
            found = true
            pos[0] += 1
        } else if(found) {
            break
        } else {
            pos[0] += 1
        }
    }
    if(!found) {
        return std.Result.Err(ImageError.InvalidFormat(string("expected number in PPM")))
    }
    return std.Result.Ok(val)
}

public func parse_ppm(data : *u8, data_len : size_t) : std::Result<Image, ImageError> {
    if(data_len < 3) {
        return std.Result.Err(ImageError.InvalidFormat(string("PPM file too small")))
    }

    if(data[0] != 'P' as u8) {
        return std.Result.Err(ImageError.InvalidFormat(string("invalid PPM signature")))
    }
    var format = data[1]
    if(format != '6' as u8 && format != '3' as u8) {
        return std.Result.Err(ImageError.UnsupportedFormat(string("only P3 and P6 PPM supported")))
    }

    var pos : size_t = 2
    var width = parse_ppm_number(data, &raw mut pos, data_len)
    if(width is Result.Err) {
        return std.Result.Err(ImageError.InvalidFormat(string("invalid width in PPM")))
    }
    var height = parse_ppm_number(data, &raw mut pos, data_len)
    if(height is Result.Err) {
        return std.Result.Err(ImageError.InvalidFormat(string("invalid height in PPM")))
    }
    var max_val = parse_ppm_number(data, &raw mut pos, data_len)
    if(max_val is Result.Err) {
        return std.Result.Err(ImageError.InvalidFormat(string("invalid max_val in PPM")))
    }

    var Ok(w) = width else unreachable
    var Ok(h) = height else unreachable

    if(format == '6' as u8) {
        pos += 1
        var img = image_create(w, h, 3)
        var total = (w as size_t) * (h as size_t) * 3
        var ptr = img.pixels.data() as *mut u8
        var i : size_t = 0
        while(i < total && pos + i < data_len) {
            ptr[i] = data[pos + i]
            i += 1
        }
        return std.Result.Ok(img)
    } else {
        var img = image_create(w, h, 3)
        var pixel_count = (w as size_t) * (h as size_t)
        var ptr = img.pixels.data() as *mut u8
        var Ok(mv) = max_val else unreachable
        var i : size_t = 0
        while(i < pixel_count) {
            var r = parse_ppm_number(data, &raw mut pos, data_len)
            var g = parse_ppm_number(data, &raw mut pos, data_len)
            var b = parse_ppm_number(data, &raw mut pos, data_len)
            if(r is Result.Err || g is Result.Err || b is Result.Err) {
                return std.Result.Err(ImageError.InvalidFormat(string("truncated PPM data")))
            }
            var Ok(rv) = r else unreachable
            var Ok(gv) = g else unreachable
            var Ok(bv) = b else unreachable
            if(mv > 255) {
                rv = (rv * 255) / mv
                gv = (gv * 255) / mv
                bv = (bv * 255) / mv
            }
            ptr[i * 3] = rv as u8
            ptr[i * 3 + 1] = gv as u8
            ptr[i * 3 + 2] = bv as u8
            i += 1
        }
        return std.Result.Ok(img)
    }
}

public func save_ppm(img : *mut Image, path : *char) : std::Result<std::Unit, ImageError> {
    if(img.channels != 3) {
        return std.Result.Err(ImageError.InvalidFormat(string("PPM requires 3 channels (RGB)")))
    }

    var header = string("")
    header.append('P')
    header.append('6')
    header.append(' ')
    header.append_integer(img.width)
    header.append(' ')
    header.append_integer(img.height)
    header.append(' ')
    header.append_integer(255)
    header.append('\n')

    var total_size = header.size() + img.pixels.size()
    var file_data = vector<u8>()
    file_data.resize(total_size)

    var fptr = file_data.data() as *mut u8
    var i : size_t = 0
    while(i < header.size()) {
        fptr[i] = header.get(i) as u8
        i += 1
    }

    var pix_bytes = image_total_bytes(img)
    var pix_ptr = img.pixels.data()
    var j : size_t = 0
    while(j < pix_bytes) {
        fptr[header.size() + j] = pix_ptr[j]
        j += 1
    }

    var write_result = fs::write_text_file(path, file_data.data(), file_data.size())
    if(write_result is Result.Err) {
        return std.Result.Err(ImageError.IoError(string("failed to write PPM file")))
    }

    return std.Result.Ok(std::Unit{})
}

} // end namespace image
