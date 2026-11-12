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

// Owned image with an invariant pixel buffer: `img_width`, `img_height`,
// `img_channels`, and `pixel_data.size()` always agree once the image is
// constructed through `new` or a decoder. Fields are internal so safe code
// cannot desynchronise them; use the accessors and `pixels()`/`pixels_mut()`.
@direct_init
public struct Image {
    internal var pixel_data : vector<u8>
    internal var img_width : int
    internal var img_height : int
    internal var img_channels : int

    @make
    internal func make() : Image {
        return Image {
            pixel_data : vector<u8>(),
            img_width : 0,
            img_height : 0,
            img_channels : 0
        }
    }

    // ── safe construction ────────────────────────────────────────────────

    /// Allocate a zero-filled image. Rejects non-positive dimensions,
    /// unsupported channel counts (only 1/2/3/4 are valid), and sizes that
    /// overflow the target address space. The buffer is always exactly
    /// `width * height * channels` bytes on success.
    public func new(width : int, height : int, channels : int) : std::Result<Image, ImageError> {
        if(width <= 0 || height <= 0) {
            return std::Result.Err<Image, ImageError>(ImageError.InvalidDimensions(width, height))
        }
        if(channels < 1 || channels > 4) {
            return std::Result.Err<Image, ImageError>(ImageError.InvalidFormat(string("unsupported channel count")))
        }

        const max_size = (0 as size_t) - 1
        var w = width as size_t
        var h = height as size_t
        var c = channels as size_t
        if(w > max_size / h) {
            return std::Result.Err<Image, ImageError>(ImageError.InvalidDimensions(width, height))
        }
        var pixel_count = w * h
        if(pixel_count > max_size / c) {
            return std::Result.Err<Image, ImageError>(ImageError.InvalidDimensions(width, height))
        }

        var img = Image.make()
        img.img_width = width
        img.img_height = height
        img.img_channels = channels
        img.pixel_data.resize(pixel_count * c)
        return std::Result.Ok<Image, ImageError>(img)
    }

    public func new_rgba(width : int, height : int) : std::Result<Image, ImageError> {
        return Image.new(width, height, 4)
    }

    public func new_rgb(width : int, height : int) : std::Result<Image, ImageError> {
        return Image.new(width, height, 3)
    }

    public func new_gray(width : int, height : int) : std::Result<Image, ImageError> {
        return Image.new(width, height, 1)
    }

    func load(path : &path::Path) : std::Result<Image, ImageError> {
        return load_from_path(path)
    }

    func from_bytes(data : &[u8]) : std::Result<Image, ImageError> {
        return decode_any(data.data(), data.size())
    }

    func from_png(data : &[u8]) : std::Result<Image, ImageError> {
        return decode_png(data.data(), data.size())
    }

    func from_bmp(data : &[u8]) : std::Result<Image, ImageError> {
        return decode_bmp(data.data(), data.size())
    }

    func from_ppm(data : &[u8]) : std::Result<Image, ImageError> {
        return decode_ppm(data.data(), data.size())
    }

    // ── safe metadata ────────────────────────────────────────────────────

    public func width(&self) : int { return self.img_width }
    public func height(&self) : int { return self.img_height }
    public func channels(&self) : int { return self.img_channels }

    /// Compatibility aliases for the original accessor names.
    func get_width(&self) : int { return self.img_width }
    func get_height(&self) : int { return self.img_height }
    func get_channels(&self) : int { return self.img_channels }

    func pixel_count(&self) : size_t { return (self.img_width as size_t) * (self.img_height as size_t) }
    func total_bytes(&self) : size_t { return self.pixel_data.size() }

    /// Check that the dimensions, channel count, and owned buffer agree.
    /// Every safe operation validates first, so a forged/empty image cannot
    /// cause an out-of-bounds access.
    public func validate(&self) : std::Result<std::Unit, ImageError> {
        if(self.img_width <= 0 || self.img_height <= 0) {
            return std::Result.Err< std::Unit, ImageError >(ImageError.InvalidDimensions(self.img_width, self.img_height))
        }
        if(self.img_channels != 1 && self.img_channels != 2 && self.img_channels != 3 && self.img_channels != 4) {
            return std::Result.Err< std::Unit, ImageError >(ImageError.InvalidFormat(string("unsupported channel count")))
        }
        const max_size = (0 as size_t) - 1
        const w = self.img_width as size_t
        const h = self.img_height as size_t
        if(w > max_size / h) {
            return std::Result.Err< std::Unit, ImageError >(ImageError.InvalidDimensions(self.img_width, self.img_height))
        }
        const pixels = w * h
        if(pixels > max_size / (self.img_channels as size_t)) {
            return std::Result.Err< std::Unit, ImageError >(ImageError.InvalidDimensions(self.img_width, self.img_height))
        }
        const expected = pixels * (self.img_channels as size_t)
        if(self.pixel_data.size() != expected) {
            return std::Result.Err< std::Unit, ImageError >(ImageError.InvalidFormat(string("pixel buffer size does not match dimensions")))
        }
        return std::Result.Ok< std::Unit, ImageError >(std::Unit{})
    }

    /// Borrowed pixel bytes in row-major order. Length is exactly
    /// `width * height * channels` for a validated image.
    public func pixels(&self) : &[u8] {
        return unsafe(std::span<u8>(self.pixel_data.data(), self.pixel_data.size()))
    }

    /// Mutable borrowed pixel bytes. Bulk codec work uses this instead of a
    /// raw `*mut u8`.
    public func pixels_mut(&mut self) : &mut [u8] {
        return unsafe(std::mut_span<u8>(self.pixel_data.mutable_data(), self.pixel_data.size()))
    }

    // Compatibility aliases for the original view names.
    func pixels_view(&self) : &[u8] { return self.pixels() }
    func pixels_mut_view(&mut self) : &mut [u8] { return self.pixels_mut() }

    // ── safe pixel operations ────────────────────────────────────────────

    /// Read one pixel as RGBA. Grayscale/GA/RGB images are expanded:
    /// 1ch -> (v,v,v,255), 2ch -> (v,v,v,a), 3ch -> (r,g,b,255).
    func pixel(&self, x : int, y : int) : std::Result<RGBA8, ImageError> {
        var check = self.validate()
        if(check is std::Result.Err) { var Err(e) = check else unsafe { unreachable }; return std::Result.Err<RGBA8, ImageError>(e) }
        return raw::image_get_rgba(unsafe(self as *mut Image), x, y)
    }

    func get(&self, x : int, y : int) : std::Result<RGBA8, ImageError> {
        return self.pixel(x, y)
    }

    /// Write one pixel, storing only the channels the image actually has:
    /// 1ch stores `color.r`; 2ch stores `color.r` + `color.a`; 3ch stores
    /// `r,g,b`; 4ch stores all four.
    func set(&mut self, x : int, y : int, color : RGBA8) : std::Result<std::Unit, ImageError> {
        var check = self.validate()
        if(check is std::Result.Err) { return check }
        return raw::image_set_rgba(unsafe(self as *mut Image), x, y, color)
    }

    func crop(&self, x : int, y : int, w : int, h : int) : std::Result<Image, ImageError> {
        var check = self.validate()
        if(check is std::Result.Err) { var Err(e) = check else unsafe { unreachable }; return std::Result.Err<Image, ImageError>(e) }
        return raw::image_crop(unsafe(self as *mut Image), x, y, w, h)
    }

    /// Mirror left/right. Returns `Err` instead of silently doing nothing when
    /// the image invariant is invalid.
    func flip_h(&mut self) : std::Result<std::Unit, ImageError> {
        var check = self.validate()
        if(check is std::Result.Err) { return check }
        raw::image_flip_h(unsafe(self as *mut Image))
        return std::Result.Ok<std::Unit, ImageError>(std::Unit{})
    }

    /// Mirror top/bottom. Returns `Err` instead of silently doing nothing when
    /// the image invariant is invalid.
    func flip_v(&mut self) : std::Result<std::Unit, ImageError> {
        var check = self.validate()
        if(check is std::Result.Err) { return check }
        raw::image_flip_v(unsafe(self as *mut Image))
        return std::Result.Ok<std::Unit, ImageError>(std::Unit{})
    }

    // `horizontal == true` mirrors left/right; false mirrors top/bottom.
    func flip(&mut self, horizontal : bool) : std::Result<std::Unit, ImageError> {
        var check = self.validate()
        if(check is std::Result.Err) { return check }
        if(horizontal) {
            raw::image_flip_h(unsafe(self as *mut Image))
        } else {
            raw::image_flip_v(unsafe(self as *mut Image))
        }
        return std::Result.Ok<std::Unit, ImageError>(std::Unit{})
    }

    func blit(&mut self, src : &Image, dx : int, dy : int) : std::Result<std::Unit, ImageError> {
        var check = self.validate()
        if(check is std::Result.Err) { return check }
        var src_check = src.validate()
        if(src_check is std::Result.Err) { return src_check }
        return raw::image_blit(unsafe(self as *mut Image), unsafe(src as *mut Image), dx, dy)
    }

    // ── safe encoding / saving ───────────────────────────────────────────

    func encode_png(&self) : std::Result<vector<u8>, ImageError> {
        return encode_png_bytes(self)
    }

    func encode_bmp(&self) : std::Result<vector<u8>, ImageError> {
        return encode_bmp_bytes(self)
    }

    func encode_ppm(&self) : std::Result<vector<u8>, ImageError> {
        return encode_ppm_bytes(self)
    }

    // Save using the extension of `path` (`.png`, `.bmp`, or `.ppm`).
    func save(&self, path : &path::Path) : std::Result<std::Unit, ImageError> {
        return save_to_path(self, path)
    }
}

} // end namespace image
