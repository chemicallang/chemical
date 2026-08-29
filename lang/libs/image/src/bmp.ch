public namespace image {

using std::Result;
using std::string;
using std::vector;
using fs::File;
using fs::OpenOptions;

public func load_bmp(path : *char) : std::Result<Image, ImageError> {
    var read_result = fs::read_entire_file(path)
    if(read_result is Result.Err) {
        return std.Result.Err(ImageError.FileNotFound())
    }
    var Ok(data) = read_result else unreachable
    return parse_bmp(data.data(), data.size())
}

public func parse_bmp(data : *u8, data_len : size_t) : std::Result<Image, ImageError> {
    if(data_len < 54) {
        return std.Result.Err(ImageError.InvalidFormat(string("BMP file too small")))
    }

    if(data[0] != 'B' as u8 || data[1] != 'M' as u8) {
        return std.Result.Err(ImageError.InvalidFormat(string("invalid BMP signature")))
    }

    var pixel_offset = read_u32_le(data, 10) as size_t
    var dib_size = read_u32_le(data, 14) as size_t

    if(dib_size < 40) {
        return std.Result.Err(ImageError.UnsupportedFormat(string("BMP DIB header too small")))
    }

    var width = read_i32_le(data, 18)
    var height = read_i32_le(data, 22)
    var bits_per_pixel = read_u16_le(data, 28)
    var compression = read_u32_le(data, 30)

    if(compression != 0) {
        return std.Result.Err(ImageError.UnsupportedFormat(string("compressed BMP not supported")))
    }

    var abs_height : int
    var top_down : bool
    if(height < 0) {
        abs_height = -height
        top_down = true
    } else {
        abs_height = height
        top_down = false
    }

    var channels : int = 0
    if(bits_per_pixel == 24) {
        channels = 3
    } else if(bits_per_pixel == 32) {
        channels = 4
    } else if(bits_per_pixel == 8) {
        channels = 1
    } else {
        return std.Result.Err(ImageError.UnsupportedFormat(string("unsupported bits per pixel")))
    }

    var img = image_create(width, abs_height, channels)
    var row_size = (((bits_per_pixel as int) * width + 31) / 32) * 4

    var row : int = 0
    while(row < abs_height) {
        var src_row : int
        if(top_down) {
            src_row = row
        } else {
            src_row = abs_height - 1 - row
        }
        var src_offset = pixel_offset + (src_row as size_t) * (row_size as size_t)
        var dst_offset = (row as size_t) * (width as size_t) * (channels as size_t)

        var col : int = 0
        while(col < width) {
            var px_src = src_offset + (col as size_t) * (bits_per_pixel as size_t / 8)
            var px_dst = dst_offset + (col as size_t) * (channels as size_t)
            if(px_src + 2 < data_len && px_dst + 2 < img.pixels.size()) {
                var ptr = img.pixels.data() as *mut u8
                if(channels == 3) {
                    ptr[px_dst] = data[px_src + 2]
                    ptr[px_dst + 1] = data[px_src + 1]
                    ptr[px_dst + 2] = data[px_src]
                } else if(channels == 4) {
                    ptr[px_dst] = data[px_src + 2]
                    ptr[px_dst + 1] = data[px_src + 1]
                    ptr[px_dst + 2] = data[px_src]
                    ptr[px_dst + 3] = data[px_src + 3]
                } else if(channels == 1) {
                    ptr[px_dst] = data[px_src]
                }
            }
            col += 1
        }
        row += 1
    }

    return std.Result.Ok(img)
}

public func save_bmp(img : *mut Image, path : *char) : std::Result<std::Unit, ImageError> {
    var bpp : int
    if(img.channels == 4) {
        bpp = 32
    } else if(img.channels == 3) {
        bpp = 24
    } else {
        bpp = 8
    }
    var row_size = (((bpp * img.width + 31) / 32) * 4)
    var pixel_data_size = row_size * img.height
    var file_size = 54 + pixel_data_size

    var file_data = vector<u8>()
    file_data.resize(file_size as size_t)

    var fptr = file_data.data() as *mut u8

    fptr[0] = 'B' as u8
    fptr[1] = 'M' as u8
    write_u32_le(fptr, 2, file_size as u32)
    write_u32_le(fptr, 10, 54)
    write_u32_le(fptr, 14, 40)
    write_u32_le(fptr, 18, img.width as u32)
    write_u32_le(fptr, 22, img.height as u32)
    write_u16_le(fptr, 26, 1)
    write_u16_le(fptr, 28, bpp as u16)
    write_u32_le(fptr, 30, 0)

    var pix_ptr = img.pixels.data()
    var row : int = 0
    while(row < img.height) {
        var src_row = img.height - 1 - row
        var src_offset = (src_row as size_t) * (img.width as size_t) * (img.channels as size_t)
        var dst_offset = 54 + (row as size_t) * (row_size as size_t)

        var col : int = 0
        while(col < img.width) {
            var px_src = src_offset + (col as size_t) * (img.channels as size_t)
            var px_dst = dst_offset + (col as size_t) * (bpp as size_t / 8)

            if(img.channels >= 3) {
                fptr[px_dst] = pix_ptr[px_src + 2]
                fptr[px_dst + 1] = pix_ptr[px_src + 1]
                fptr[px_dst + 2] = pix_ptr[px_src]
                if(img.channels == 4) {
                    fptr[px_dst + 3] = pix_ptr[px_src + 3]
                }
            } else {
                fptr[px_dst] = pix_ptr[px_src]
            }
            col += 1
        }
        row += 1
    }

    var write_result = fs::write_text_file(path, file_data.data(), file_data.size())
    if(write_result is Result.Err) {
        return std.Result.Err(ImageError.IoError(string("failed to write BMP file")))
    }

    return std.Result.Ok(std::Unit{})
}

} // end namespace image
