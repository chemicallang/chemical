public namespace image {

using std::string;
using std::vector;

public variant ImageError {
    FileNotFound()
    InvalidFormat(msg : string)
    UnsupportedFormat(msg : string)
    IoError(msg : string)
    InvalidDimensions(w : int, h : int)
    PixelOutOfBounds(x : int, y : int)

    func message(&self) : string {
        switch(self) {
            FileNotFound() => return string("ImageError: file not found")
            InvalidFormat(msg) => {
                var s = string("ImageError: ")
                s.append_string(&msg)
                return s
            }
            UnsupportedFormat(msg) => {
                var s = string("ImageError: unsupported format: ")
                s.append_string(&msg)
                return s
            }
            IoError(msg) => {
                var s = string("ImageError: IO error: ")
                s.append_string(&msg)
                return s
            }
            InvalidDimensions(w, h) => {
                var s = string("ImageError: invalid dimensions ")
                s.append_integer(w)
                s.append('x')
                s.append_integer(h)
                return s
            }
            PixelOutOfBounds(x, y) => {
                var s = string("ImageError: pixel out of bounds (")
                s.append_integer(x)
                s.append(',')
                s.append_integer(y)
                s.append(')')
                return s
            }
        }
    }
}

public struct RGBA8 {
    var r : u8
    var g : u8
    var b : u8
    var a : u8

    @make
    func make(r_ : u8, g_ : u8, b_ : u8, a_ : u8 = 255) : RGBA8 {
        return RGBA8 { r : r_, g : g_, b : b_, a : a_ }
    }
}

public struct RGB8 {
    var r : u8
    var g : u8
    var b : u8
}

public struct Gray8 {
    var v : u8
}

@direct_init
public struct Image {
    var pixels : vector<u8>
    var width : int
    var height : int
    var channels : int

    @make
    func make() : Image {
        return Image {
            pixels : vector<u8>(),
            width : 0,
            height : 0,
            channels : 0
        }
    }

    // ── safe construction ────────────────────────────────────────────────

    func load(path : &path::Path) : std::Result<Image, ImageError> {
        return image_safe_load(path)
    }

    func from_bytes(data : &[u8]) : std::Result<Image, ImageError> {
        return image_safe_from_bytes(data.data(), data.size())
    }

    func from_png(data : &[u8]) : std::Result<Image, ImageError> {
        return image_safe_from_png(data.data(), data.size())
    }

    func from_bmp(data : &[u8]) : std::Result<Image, ImageError> {
        return image_safe_from_bmp(data.data(), data.size())
    }

    func from_ppm(data : &[u8]) : std::Result<Image, ImageError> {
        return image_safe_from_ppm(data.data(), data.size())
    }

    // ── safe metadata ────────────────────────────────────────────────────

    /// Check that the public dimension/channel fields describe exactly the
    /// owned pixel buffer. All safe operations call this before touching raw
    /// pixel memory, so a forged Image cannot cause an out-of-bounds access.
    public func validate(&self) : std::Result<std::Unit, ImageError> {
        if(self.width <= 0 || self.height <= 0) {
            return std::Result.Err< std::Unit, ImageError >(ImageError.InvalidDimensions(self.width, self.height))
        }
        if(self.channels != 1 && self.channels != 2 && self.channels != 3 && self.channels != 4) {
            return std::Result.Err< std::Unit, ImageError >(ImageError.InvalidFormat(string("unsupported channel count")))
        }
        const max_size = (0 as size_t) - 1
        const w = self.width as size_t
        const h = self.height as size_t
        if(w > max_size / h) {
            return std::Result.Err< std::Unit, ImageError >(ImageError.InvalidDimensions(self.width, self.height))
        }
        const pixels = w * h
        if(pixels > max_size / (self.channels as size_t)) {
            return std::Result.Err< std::Unit, ImageError >(ImageError.InvalidDimensions(self.width, self.height))
        }
        const expected = pixels * (self.channels as size_t)
        if(self.pixels.size() != expected) {
            return std::Result.Err< std::Unit, ImageError >(ImageError.InvalidFormat(string("pixel buffer size does not match dimensions")))
        }
        return std::Result.Ok< std::Unit, ImageError >(std::Unit{})
    }

    // `width`, `height`, and `channels` are already public fields; these
    // accessors exist for interface-style call sites.
    func get_width(&self) : int { return self.width }
    func get_height(&self) : int { return self.height }
    func get_channels(&self) : int { return self.channels }
    func pixel_count(&self) : size_t { return (self.width as size_t) * (self.height as size_t) }
    func total_bytes(&self) : size_t { return self.pixels.size() }

    // Borrowed pixel bytes. `pixels_mut_view` requires `&mut self`.
    func pixels_view(&self) : [u8] {
        return unsafe(std::span<u8>(self.pixels.data(), self.pixels.size()))
    }

    func pixels_mut_view(&mut self) : &mut [u8] {
        return unsafe(std::mut_span<u8>(self.pixels.mutable_data(), self.pixels.size()))
    }

    // ── safe pixel operations ────────────────────────────────────────────

    func pixel(&self, x : int, y : int) : std::Result<RGBA8, ImageError> {
        var check = self.validate()
        if(check is std::Result.Err) { var Err(e) = check else unsafe { unreachable }; return std::Result.Err<RGBA8, ImageError>(e) }
        return image_get_rgba(unsafe(self as *mut Image), x, y)
    }

    func get(&self, x : int, y : int) : std::Result<RGBA8, ImageError> {
        return self.pixel(x, y)
    }

    func set(&mut self, x : int, y : int, color : RGBA8) : std::Result<std::Unit, ImageError> {
        var check = self.validate()
        if(check is std::Result.Err) { return check }
        return image_set_rgba(unsafe(self as *mut Image), x, y, color)
    }

    func crop(&self, x : int, y : int, w : int, h : int) : std::Result<Image, ImageError> {
        var check = self.validate()
        if(check is std::Result.Err) { var Err(e) = check else unsafe { unreachable }; return std::Result.Err<Image, ImageError>(e) }
        return image_crop(unsafe(self as *mut Image), x, y, w, h)
    }

    func flip_h(&mut self) {
        if(self.validate() is std::Result.Ok) { image_flip_h(unsafe(self as *mut Image)) }
    }

    func flip_v(&mut self) {
        if(self.validate() is std::Result.Ok) { image_flip_v(unsafe(self as *mut Image)) }
    }

    // `horizontal == true` mirrors left/right; false mirrors top/bottom.
    func flip(&mut self, horizontal : bool) {
        if(self.validate() is std::Result.Ok) {
            if(horizontal) {
                image_flip_h(unsafe(self as *mut Image))
            } else {
                image_flip_v(unsafe(self as *mut Image))
            }
        }
    }

    func blit(&mut self, src : &Image, dx : int, dy : int) : std::Result<std::Unit, ImageError> {
        var check = self.validate()
        if(check is std::Result.Err) { return check }
        var src_check = src.validate()
        if(src_check is std::Result.Err) { return src_check }
        return image_blit(unsafe(self as *mut Image), unsafe(src as *mut Image), dx, dy)
    }

    // ── safe encoding / saving ───────────────────────────────────────────

    func encode_png(&self) : std::Result<vector<u8>, ImageError> {
        return image_safe_encode_png(self)
    }

    func encode_bmp(&self) : std::Result<vector<u8>, ImageError> {
        return image_safe_encode_bmp(self)
    }

    func encode_ppm(&self) : std::Result<vector<u8>, ImageError> {
        return image_safe_encode_ppm(self)
    }

    // Save using the extension of `path` (`.png`, `.bmp`, or `.ppm`).
    func save(&self, path : &path::Path) : std::Result<std::Unit, ImageError> {
        return image_safe_save(self, path)
    }
}

} // end namespace image
