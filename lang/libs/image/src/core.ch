public namespace image {

using std::Result;
using std::string;
using std::vector;
using fs::File;
using fs::OpenOptions;

func read_u16_be(data : *u8, offset : size_t) : u16 {
    return ((data[offset] as u16) << 8) | (data[offset + 1] as u16)
}

func read_u32_be(data : *u8, offset : size_t) : u32 {
    return ((data[offset] as u32) << 24) | ((data[offset + 1] as u32) << 16) |
           ((data[offset + 2] as u32) << 8) | (data[offset + 3] as u32)
}

func read_u16_le(data : *u8, offset : size_t) : u16 {
    return (data[offset] as u16) | ((data[offset + 1] as u16) << 8)
}

func read_u32_le(data : *u8, offset : size_t) : u32 {
    return (data[offset] as u32) | ((data[offset + 1] as u32) << 8) |
           ((data[offset + 2] as u32) << 16) | ((data[offset + 3] as u32) << 24)
}

func read_i32_le(data : *u8, offset : size_t) : i32 {
    var val = (data[offset] as u32) | ((data[offset + 1] as u32) << 8) |
              ((data[offset + 2] as u32) << 16) | ((data[offset + 3] as u32) << 24)
    return val as i32
}

func write_u16_le(out : *mut u8, offset : size_t, val : u16) {
    out[offset] = (val & 0xFFu16) as u8
    out[offset + 1] = ((val >> 8) & 0xFFu16) as u8
}

func write_u32_le(out : *mut u8, offset : size_t, val : u32) {
    out[offset] = (val & 0xFFu32) as u8
    out[offset + 1] = ((val >> 8) & 0xFFu32) as u8
    out[offset + 2] = ((val >> 16) & 0xFFu32) as u8
    out[offset + 3] = ((val >> 24) & 0xFFu32) as u8
}

public func image_create(w : int, h : int, ch : int) : Image {
    var img = Image{pixels: vector<u8>(), width: 0, height: 0, channels: 0}
    img.width = w
    img.height = h
    img.channels = ch
    var total = (w as size_t) * (h as size_t) * (ch as size_t)
    img.pixels.resize(total)
    var ptr = img.pixels.data() as *mut u8
    var i : size_t = 0
    while(i < total) {
        ptr[i] = 0
        i += 1
    }
    return img
}

public func image_create_rgba(w : int, h : int) : Image {
    return image_create(w, h, 4)
}

public func image_create_rgb(w : int, h : int) : Image {
    return image_create(w, h, 3)
}

public func image_create_gray(w : int, h : int) : Image {
    return image_create(w, h, 1)
}

public func image_width(img : *mut Image) : int {
    return img.width
}

public func image_height(img : *mut Image) : int {
    return img.height
}

public func image_channels(img : *mut Image) : int {
    return img.channels
}

public func image_pixels(img : *mut Image) : *mut u8 {
    return img.pixels.data() as *mut u8
}

public func image_pixel_count(img : *mut Image) : size_t {
    return (img.width as size_t) * (img.height as size_t)
}

public func image_bytes_per_row(img : *mut Image) : size_t {
    return (img.width as size_t) * (img.channels as size_t)
}

public func image_total_bytes(img : *mut Image) : size_t {
    return image_bytes_per_row(img) * (img.height as size_t)
}

public func image_get_rgba(img : *mut Image, x : int, y : int) : std::Result<RGBA8, ImageError> {
    if(x < 0 || x >= img.width || y < 0 || y >= img.height) {
        return std.Result.Err(ImageError.PixelOutOfBounds(x, y))
    }
    if(img.channels < 4) {
        return std.Result.Err(ImageError.InvalidFormat(string("image does not have alpha channel")))
    }
    var offset = ((y as size_t) * (img.width as size_t) + (x as size_t)) * 4
    var px = img.pixels.data() + offset
    return std.Result.Ok(RGBA8.make(px[0], px[1], px[2], px[3]))
}

public func image_set_rgba(img : *mut Image, x : int, y : int, color : RGBA8) : std::Result<std::Unit, ImageError> {
    if(x < 0 || x >= img.width || y < 0 || y >= img.height) {
        return std.Result.Err(ImageError.PixelOutOfBounds(x, y))
    }
    if(img.channels < 4) {
        return std.Result.Err(ImageError.InvalidFormat(string("image does not have alpha channel")))
    }
    var offset = ((y as size_t) * (img.width as size_t) + (x as size_t)) * 4
    var ptr = img.pixels.data() as *mut u8
    ptr[offset] = color.r
    ptr[offset + 1] = color.g
    ptr[offset + 2] = color.b
    ptr[offset + 3] = color.a
    return std.Result.Ok(std::Unit{})
}

public func image_fill(img : *mut Image, color : RGBA8) {
    var bpp = img.channels as size_t
    var ptr = img.pixels.data() as *mut u8
    var i : size_t = 0
    while(i < img.pixels.size()) {
        if(bpp >= 1) { ptr[i] = color.r }
        if(bpp >= 3) { ptr[i + 1] = color.g; ptr[i + 2] = color.b }
        if(bpp >= 4) { ptr[i + 3] = color.a }
        i += bpp
    }
}

public func image_copy(src : *mut Image) : Image {
    var dst = image_create(src.width, src.height, src.channels)
    var total = image_total_bytes(src)
    var dst_ptr = dst.pixels.data() as *mut u8
    var src_ptr = src.pixels.data()
    var i : size_t = 0
    while(i < total) {
        dst_ptr[i] = src_ptr[i]
        i += 1
    }
    return dst
}

public func image_crop(img : *mut Image, x : int, y : int, w : int, h : int) : std::Result<Image, ImageError> {
    if(x < 0 || y < 0 || x + w > img.width || y + h > img.height) {
        return std.Result.Err(ImageError.InvalidDimensions(w, h))
    }
    var cropped = image_create(w, h, img.channels)
    var bpp = img.channels as size_t
    var row_bytes = (w as size_t) * bpp
    var src_ptr = img.pixels.data()
    var dst_ptr = cropped.pixels.data() as *mut u8
    var dst_y : int = 0
    while(dst_y < h) {
        var src_offset = (((y + dst_y) as size_t) * (img.width as size_t) + (x as size_t)) * bpp
        var dst_offset = (dst_y as size_t) * row_bytes
        var col : size_t = 0
        while(col < row_bytes) {
            dst_ptr[dst_offset + col] = src_ptr[src_offset + col]
            col += 1
        }
        dst_y += 1
    }
    return std.Result.Ok(cropped)
}

public func image_flip_h(img : *mut Image) {
    var bpp = img.channels as size_t
    var row_bytes = (img.width as size_t) * bpp
    var ptr = img.pixels.data() as *mut u8
    var row : int = 0
    while(row < img.height) {
        var row_start = (row as size_t) * row_bytes
        var left : size_t = 0
        var right = row_bytes - bpp
        while(left < right) {
            var i : size_t = 0
            while(i < bpp) {
                var tmp = ptr[row_start + left + i]
                ptr[row_start + left + i] = ptr[row_start + right + i]
                ptr[row_start + right + i] = tmp
                i += 1
            }
            left += bpp
            right -= bpp
        }
        row += 1
    }
}

public func image_flip_v(img : *mut Image) {
    var bpp = img.channels as size_t
    var row_bytes = (img.width as size_t) * bpp
    var half_h = img.height / 2
    var ptr = img.pixels.data() as *mut u8
    var row : int = 0
    while(row < half_h) {
        var top_start = (row as size_t) * row_bytes
        var bot_start = ((img.height - 1 - row) as size_t) * row_bytes
        var col : size_t = 0
        while(col < row_bytes) {
            var tmp = ptr[top_start + col]
            ptr[top_start + col] = ptr[bot_start + col]
            ptr[bot_start + col] = tmp
            col += 1
        }
        row += 1
    }
}

public func image_rotate90(img : *mut Image) : Image {
    var rotated = image_create(img.height, img.width, img.channels)
    var bpp = img.channels as size_t
    var src_w = img.width as size_t
    var src_h = img.height as size_t
    var src_ptr = img.pixels.data()
    var dst_ptr = rotated.pixels.data() as *mut u8
    var y : size_t = 0
    while(y < src_h) {
        var x : size_t = 0
        while(x < src_w) {
            var src_off = (y * src_w + x) * bpp
            var dst_off = (x * src_h + (src_h - 1 - y)) * bpp
            var i : size_t = 0
            while(i < bpp) {
                dst_ptr[dst_off + i] = src_ptr[src_off + i]
                i += 1
            }
            x += 1
        }
        y += 1
    }
    return rotated
}

public func image_blit(dst : *mut Image, src : *mut Image, dx : int, dy : int) : std::Result<std::Unit, ImageError> {
    var bpp = src.channels as size_t
    if(dst.channels != src.channels) {
        return std.Result.Err(ImageError.InvalidFormat(string("channel count mismatch for blit")))
    }
    var src_ptr = src.pixels.data()
    var dst_ptr = dst.pixels.data() as *mut u8
    var y : int = 0
    while(y < src.height) {
        var x : int = 0
        while(x < src.width) {
            var dst_x = dx + x
            var dst_y = dy + y
            if(dst_x >= 0 && dst_x < dst.width && dst_y >= 0 && dst_y < dst.height) {
                var src_off = ((y as size_t) * (src.width as size_t) + (x as size_t)) * bpp
                var dst_off = ((dst_y as size_t) * (dst.width as size_t) + (dst_x as size_t)) * bpp
                var i : size_t = 0
                while(i < bpp) {
                    dst_ptr[dst_off + i] = src_ptr[src_off + i]
                    i += 1
                }
            }
            x += 1
        }
        y += 1
    }
    return std.Result.Ok(std::Unit{})
}

public func image_line(img : *mut Image, x0 : int, y0 : int, x1 : int, y1 : int, color : RGBA8) {
    var dx = x1 - x0
    var dy = y1 - y0
    var sx = 1
    var sy = 1
    if(dx < 0) { dx = -dx; sx = -1 }
    if(dy < 0) { dy = -dy; sy = -1 }
    var err = dx - dy
    var x = x0
    var y = y0
    while(true) {
        if(x >= 0 && x < img.width && y >= 0 && y < img.height) {
            image_set_rgba(img, x, y, color)
        }
        if(x == x1 && y == y1) { break }
        var e2 = err * 2
        if(e2 > -dy) { err -= dy; x += sx }
        if(e2 < dx) { err += dx; y += sy }
    }
}

public func image_rectangle(img : *mut Image, x : int, y : int, w : int, h : int, color : RGBA8) {
    image_line(img, x, y, x + w - 1, y, color)
    image_line(img, x + w - 1, y, x + w - 1, y + h - 1, color)
    image_line(img, x + w - 1, y + h - 1, x, y + h - 1, color)
    image_line(img, x, y + h - 1, x, y, color)
}

public func image_fill_rect(img : *mut Image, x : int, y : int, w : int, h : int, color : RGBA8) {
    var bpp = img.channels as size_t
    var ptr = img.pixels.data() as *mut u8
    var row : int = y
    while(row < y + h) {
        if(row >= 0 && row < img.height) {
            var col : int = x
            while(col < x + w) {
                if(col >= 0 && col < img.width) {
                    var offset = ((row as size_t) * (img.width as size_t) + (col as size_t)) * bpp
                    if(bpp >= 1) { ptr[offset] = color.r }
                    if(bpp >= 3) { ptr[offset + 1] = color.g; ptr[offset + 2] = color.b }
                    if(bpp >= 4) { ptr[offset + 3] = color.a }
                }
                col += 1
            }
        }
        row += 1
    }
}

public func image_circle(img : *mut Image, cx : int, cy : int, radius : int, color : RGBA8) {
    var x = radius
    var y = 0
    var err = 1 - radius
    while(x >= y) {
        image_set_rgba(img, cx + x, cy + y, color)
        image_set_rgba(img, cx + y, cy + x, color)
        image_set_rgba(img, cx - y, cy + x, color)
        image_set_rgba(img, cx - x, cy + y, color)
        image_set_rgba(img, cx - x, cy - y, color)
        image_set_rgba(img, cx - y, cy - x, color)
        image_set_rgba(img, cx + y, cy - x, color)
        image_set_rgba(img, cx + x, cy - y, color)
        y += 1
        if(err < 0) {
            err += 2 * y + 1
        } else {
            x -= 1
            err += 2 * (y - x) + 1
        }
    }
}

public func image_fill_circle(img : *mut Image, cx : int, cy : int, radius : int, color : RGBA8) {
    var y = -radius
    while(y <= radius) {
        var x = -radius
        while(x <= radius) {
            if(x * x + y * y <= radius * radius) {
                image_set_rgba(img, cx + x, cy + y, color)
            }
            x += 1
        }
        y += 1
    }
}

} // end namespace image
