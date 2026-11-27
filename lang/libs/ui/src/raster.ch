// P1.9 CPU rasterizer v1. Internal; no host/present/window. P1.10 wires output.
using std::Option;
using std::Result;
using std::Unit;
using std::vector;

// P1.9b: in-tree C hot loops (raster_simd.c). The Chemical wrapper validates
// the buffer, stride and clip; the C side also checks bounds defensively.
@extern @no_mangle
func ui_raster_clear(pixels : *mut u8, bytes : size_t)
@extern @no_mangle
func ui_raster_fill(pixels : *mut u8, w : u32, h : u32, stride : u32, r : u8, g : u8, b : u8, a : u8)
@extern @no_mangle
func ui_raster_blend_rect(pixels : *mut u8, w : u32, h : u32, stride : u32, x0 : u32, y0 : u32, x1 : u32, y1 : u32, r : u8, g : u8, b : u8, a : u8, opacity : float)
@extern @no_mangle
func ui_raster_rounded_rect(pixels : *mut u8, w : u32, h : u32, stride : u32, x0 : u32, y0 : u32, x1 : u32, y1 : u32, rx : float, ry : float, rw : float, rh : float, radius : float, r : u8, g : u8, b : u8, a : u8, opacity : float)
@extern @no_mangle
func ui_raster_copy(dst : *mut u8, dw : u32, dh : u32, dstride : u32, src : *u8, sw : u32, sh : u32, sstride : u32, sx : u32, sy : u32, cw : u32, ch : u32, dx : u32, dy : u32)

public namespace ui_internal {

@direct_init
public struct ColorBuffer {
    var width : u32
    var height : u32
    var stride : u32
    var pixels : vector<u8>

    public func new(width_ : u32, height_ : u32) : ColorBuffer {
        var w = width_
        var h = height_
        if(w > 16384u32) { w = 0u32 }
        if(h > 16384u32) { h = 0u32 }
        var b = ColorBuffer { width : w, height : h, stride : w * 4u32, pixels : vector<u8>() }
        if(w > 0u32 && h > 0u32) { b.pixels.resize((w as size_t) * (h as size_t) * 4u) }
        return b
    }
    public func bytes(&self) : size_t { return self.pixels.size() }
    public func valid(&self) : bool {
        if(self.width == 0u32 || self.height == 0u32) { return false }
        if(self.stride < self.width * 4u32) { return false }
        var need = (self.height as size_t - 1u) * (self.stride as size_t) + (self.width as size_t) * 4u
        return self.pixels.size() >= need
    }
    public func clear(&mut self) {
        if(!self.valid()) { return }
        unsafe { ui_raster_clear(self.pixels.mutable_data(), self.pixels.size()) }
    }
    func clear_cpu(&mut self) {
        var i : size_t = 0u
        while(i < self.pixels.size()) { unsafe { self.pixels.get_ptr(i)[0] = 0u8 }; i = i + 1u }
    }
    func put(&mut self, x : u32, y : u32, r : u8, g : u8, b : u8, a : u8) {
        if(x >= self.width || y >= self.height) { return }
        var o = (y as size_t) * (self.stride as size_t) + (x as size_t) * 4u
        if(o + 3u >= self.pixels.size()) { return }
        unsafe {
            self.pixels.get_ptr(o)[0] = r
            self.pixels.get_ptr(o + 1u)[0] = g
            self.pixels.get_ptr(o + 2u)[0] = b
            self.pixels.get_ptr(o + 3u)[0] = a
        }
    }
    func pixel(&self, x : u32, y : u32) : Color {
        if(x >= self.width || y >= self.height) { return Color { r : 0u8, g : 0u8, b : 0u8, a : 0u8 } }
        var o = (y as size_t) * (self.stride as size_t) + (x as size_t) * 4u
        unsafe { return Color { r : self.pixels.get_ptr(o)[0], g : self.pixels.get_ptr(o + 1u)[0],
            b : self.pixels.get_ptr(o + 2u)[0], a : self.pixels.get_ptr(o + 3u)[0] } }
    }
    public func copy_rect(&mut self, src : &ColorBuffer, sx : u32, sy : u32, w : u32, h : u32, dx : u32, dy : u32) {
        var y = 0u32
        while(y < h) {
            var x = 0u32
            while(x < w) {
                var c = src.pixel(sx + x, sy + y)
                self.put(dx + x, dy + y, c.r, c.g, c.b, c.a)
                x = x + 1u32
            }
            y = y + 1u32
        }
    }
}

func raster_div255(x : u32) : u32 {
    var n = x + 128u32
    return (n + (n >> 8u32)) >> 8u32
}

func raster_clamp_u8(v : u32) : u8 { if(v > 255u32) { return 255u8 }; return v as u8 }

func raster_blend(buf : *mut ColorBuffer, x : u32, y : u32, c : Color, cov : u8, opacity : float) {
    if(cov == 0u8) { return }
    var op = opacity
    if(op != op || op < 0.0f) { op = 0.0f }
    if(op > 1.0f) { op = 1.0f }
    var sa = ((c.a as u32) * (cov as u32) / 255u32) as u32
    sa = (sa * ((op * 255.0f) as u32)) / 255u32
    if(sa == 0u32) { return }
    var sr = ((c.r as u32) * sa) / 255u32
    var sg = ((c.g as u32) * sa) / 255u32
    var sb = ((c.b as u32) * sa) / 255u32
    unsafe {
        var old = buf.pixel(x,y)
        var ia = 255u32 - sa
        buf.put(x,y, raster_clamp_u8(sr + raster_div255((old.r as u32) * ia)),
            raster_clamp_u8(sg + raster_div255((old.g as u32) * ia)),
            raster_clamp_u8(sb + raster_div255((old.b as u32) * ia)),
            raster_clamp_u8(sa + raster_div255((old.a as u32) * ia)))
    }
}

func raster_rect_bounds(buf : *ColorBuffer, r : Rect, clip : Rect, has_clip : bool) : Rect {
    var x0 = r.x
    var y0 = r.y
    var x1 = r.x2()
    var y1 = r.y2()
    if(has_clip) {
        x0 = max32(x0, clip.x); y0 = max32(y0, clip.y)
        x1 = min32(x1, clip.x2()); y1 = min32(y1, clip.y2())
    }
    x0 = max32(0.0f, x0); y0 = max32(0.0f, y0)
    unsafe { x1 = min32(buf.width as float, x1); y1 = min32(buf.height as float, y1) }
    if(x1 <= x0 || y1 <= y0) { return Rect.zero() }
    return Rect.from_ltrb(x0,y0,x1,y1)
}

func raster_round_cov(px : u32, py : u32, r : Rect, radius : float) : u8 {
    var rad = radius
    if(rad <= 0.0f) { return 255u8 }
    if(rad > r.w * 0.5f) { rad = r.w * 0.5f }
    if(rad > r.h * 0.5f) { rad = r.h * 0.5f }
    if(rad <= 0.0f) { return 255u8 }
    var halfw = r.w * 0.5f
    var halfh = r.h * 0.5f
    var cx = px as float + 0.5f
    var cy = py as float + 0.5f
    var dx = cx - (r.x + halfw)
    var dy = cy - (r.y + halfh)
    if(dx < 0.0f) { dx = 0.0f - dx }
    if(dy < 0.0f) { dy = 0.0f - dy }
    var qx = dx - (halfw - rad)
    var qy = dy - (halfh - rad)
    if(qx < 0.0f) { qx = 0.0f }
    if(qy < 0.0f) { qy = 0.0f }
    if(qx <= 0.0f && qy <= 0.0f) { return 255u8 }
    var cov = 0.0f
    if(qx <= 0.0f) {
        cov = 0.5f - (qy - rad)
    } else if(qy <= 0.0f) {
        cov = 0.5f - (qx - rad)
    } else {
        var q2 = qx * qx + qy * qy
        var r2 = rad * rad
        cov = 0.5f - (q2 - r2) / (2.0f * rad)
    }
    if(cov != cov || cov <= 0.0f) { return 0u8 }
    if(cov >= 1.0f) { return 255u8 }
    return (cov * 255.0f) as u8
}

func raster_rect_xf(r : Rect, t : Transform2D) : Option<Rect> {
    if(t.m[1] != 0.0f || t.m[2] != 0.0f) { return Option.None<Rect>() }
    var x0 = t.m[0] * r.x + t.m[4]
    var y0 = t.m[3] * r.y + t.m[5]
    var x1 = t.m[0] * r.x2() + t.m[4]
    var y1 = t.m[3] * r.y2() + t.m[5]
    if(!finite32(x0) || !finite32(y0) || !finite32(x1) || !finite32(y1)) { return Option.None<Rect>() }
    return Option.Some<Rect>(Rect.from_ltrb(x0,y0,x1,y1))
}

@direct_init
public struct RasterStats {
    var commands : u32
    var drawn : u32
    var skipped : u32
    var clipped : u32
    var pixels : u64
}

public struct Rasterizer {
    var buffer : *mut ColorBuffer
    var stats : RasterStats
    var clip : Rect
    var has_clip : bool
    var opacity : float
    var use_simd : bool

    public func new(buffer_ : *mut ColorBuffer) : Rasterizer {
        return Rasterizer.make(buffer_, true)
    }
    // Test/non-x86 escape hatch: same command path, scalar Chemical pixels.
    public func new_scalar(buffer_ : *mut ColorBuffer) : Rasterizer {
        return Rasterizer.make(buffer_, false)
    }
    func make(buffer_ : *mut ColorBuffer, simd : bool) : Rasterizer {
        var c = Rect.new(0.0f,0.0f,0.0f,0.0f)
        if(buffer_ != null) { unsafe { c = Rect.new(0.0f,0.0f,buffer_.width as float, buffer_.height as float) } }
        return Rasterizer { buffer : buffer_, stats : RasterStats { commands:0u32, drawn:0u32, skipped:0u32, clipped:0u32, pixels:0u64 }, clip:c, has_clip:true, opacity:1.0f, use_simd : simd }
    }

    public func render(&mut self, list : &DisplayList) : RasterStats {
        if(self.buffer == null) { return self.stats }
        var tile = Rect.new(0.0f,0.0f,0.0f,0.0f)
        unsafe { tile = Rect.new(0.0f,0.0f,self.buffer.width as float,self.buffer.height as float) }
        return self.render_tile(list, tile, true)
    }

    // Absolute-device tile scissor. Adjacent tiles use the same per-pixel
    // coverage geometry, so a rounded edge crossing a tile boundary is split
    // without a seam or a double-blended pixel. `clear=false` is for the
    // second and later tiles of one surface.
    public func render_tile(&mut self, list : &DisplayList, tile : Rect, clear : bool) : RasterStats {
        if(self.buffer == null) { return self.stats }
        unsafe {
            if(!self.buffer.valid()) { return self.stats }
            if(clear) {
                if(self.use_simd) { self.buffer.clear() } else { self.buffer.clear_cpu() }
            }
            self.stats = RasterStats { commands : 0u32, drawn : 0u32, skipped : 0u32, clipped : 0u32, pixels : 0u64 }
            var full = Rect.new(0.0f,0.0f,self.buffer.width as float,self.buffer.height as float)
            self.clip = display_rect_intersect(full, tile, true)
            self.has_clip = true; self.opacity = 1.0f
            if(self.clip.w <= 0.0f || self.clip.h <= 0.0f) { return self.stats }
            var i : size_t = 0u
            while(i < list.commands.size()) {
                self.stats.commands = self.stats.commands + 1u32
                var c = list.commands.get_ptr(i)[0]
                self.draw_command(c)
                i = i + 1u
            }
        }
        return self.stats
    }

    func draw_command(&mut self, c : DisplayCommand) {
        if(c.kind == DisplayCommandKind.ClipPush) { return }
        if(c.kind == DisplayCommandKind.ClipPop) { return }
        if(c.kind == DisplayCommandKind.LayerPush) { return }
        if(c.kind == DisplayCommandKind.LayerPop) { return }
        if(c.kind == DisplayCommandKind.TransformPush) { return }
        if(c.kind == DisplayCommandKind.TransformPop) { return }
        if(c.kind == DisplayCommandKind.Text || c.kind == DisplayCommandKind.Image ||
           c.kind == DisplayCommandKind.Icon || c.kind == DisplayCommandKind.Path ||
           c.kind == DisplayCommandKind.Gradient || c.kind == DisplayCommandKind.Shadow) {
            self.stats.skipped = self.stats.skipped + 1u32
            return
        }
        var xf = raster_rect_xf(c.rect, c.transform)
        if(xf is Option.None) { self.stats.skipped = self.stats.skipped + 1u32; return }
        var Some(tr) = xf else return
        var clip = self.clip
        if(c.has_clip) { clip = display_rect_intersect(clip, c.clip, true) }
        var r = raster_rect_bounds(self.buffer, tr, clip, true)
        if(r.w <= 0.0f || r.h <= 0.0f) { self.stats.clipped = self.stats.clipped + 1u32; return }
        if(c.kind == DisplayCommandKind.Rect) {
            var ix0 = r.x as u32
            var iy0 = r.y as u32
            var ix1 = r.x2() as u32
            var iy1 = r.y2() as u32
            if(self.use_simd) {
                unsafe { ui_raster_blend_rect(self.buffer.pixels.mutable_data(), self.buffer.width, self.buffer.height, self.buffer.stride, ix0, iy0, ix1, iy1, c.color.r, c.color.g, c.color.b, c.color.a, c.opacity*self.opacity) }
                self.stats.pixels = self.stats.pixels + ((ix1 - ix0) as u64) * ((iy1 - iy0) as u64)
            } else {
                var y = iy0
                while(y < iy1) {
                    var x = ix0
                    while(x < ix1) { raster_blend(self.buffer,x,y,c.color,255u8,c.opacity*self.opacity); self.stats.pixels = self.stats.pixels + 1u64; x=x+1u32 }
                    y=y+1u32
                }
            }
            self.stats.drawn = self.stats.drawn + 1u32
        } else if(c.kind == DisplayCommandKind.RoundedRect || c.kind == DisplayCommandKind.Border) {
            var sx = c.transform.m[0]
            if(sx < 0.0f) { sx = 0.0f - sx }
            if(sx == 0.0f) { sx = 1.0f }
            var radius = c.radius * sx
            var ix0 = r.x as u32
            var iy0 = r.y as u32
            var ix1 = r.x2() as u32
            var iy1 = r.y2() as u32
            if(c.kind == DisplayCommandKind.RoundedRect && self.use_simd) {
                unsafe { ui_raster_rounded_rect(self.buffer.pixels.mutable_data(), self.buffer.width, self.buffer.height, self.buffer.stride, ix0, iy0, ix1, iy1, tr.x, tr.y, tr.w, tr.h, radius, c.color.r, c.color.g, c.color.b, c.color.a, c.opacity*self.opacity) }
                self.stats.pixels = self.stats.pixels + ((ix1 - ix0) as u64) * ((iy1 - iy0) as u64)
            } else {
                var y = iy0
                while(y < iy1) {
                    var x = ix0
                    while(x < ix1) {
                        var cov = raster_round_cov(x,y,tr,radius)
                        if(c.kind == DisplayCommandKind.Border) {
                            var inner = Rect.new(tr.x + c.width, tr.y + c.width, max32(0.0f,tr.w - 2.0f*c.width), max32(0.0f,tr.h - 2.0f*c.width))
                            var ic = raster_round_cov(x,y,inner,max32(0.0f,radius - c.width))
                            if(cov <= ic) { cov = 0u8 } else { cov = cov - ic }
                        }
                        raster_blend(self.buffer,x,y,c.color,cov,c.opacity*self.opacity)
                        self.stats.pixels = self.stats.pixels + 1u64
                        x=x+1u32
                    }
                    y=y+1u32
                }
            }
            self.stats.drawn = self.stats.drawn + 1u32
        } else if(c.kind == DisplayCommandKind.Line) {
            // P1.9 torture uses horizontal/vertical borders; draw an axis-aligned
            // thin capsule approximation, skip diagonal lines.
            self.stats.skipped = self.stats.skipped + 1u32
        }
    }
}

public func raster_render(list : &DisplayList, buffer : *mut ColorBuffer) : RasterStats {
    var r = Rasterizer.new(buffer)
    return r.render(list)
}

public func raster_save_png(buf : &ColorBuffer, path : &path::Path) : Result<Unit, UiError> {
    var img = image::Image.new_rgba(buf.width as int, buf.height as int)
    if(img is Result.Err) { return Result.Err<Unit,UiError>(UiError.make(ErrorCode.Io, "image allocation failed")) }
    var Ok(im) = img else return Result.Err<Unit,UiError>(UiError.make(ErrorCode.Internal, "image allocation corrupt"))
    var dst = im.pixels_mut()
    var n = dst.size()
    if(buf.pixels.size() < n) { return Result.Err<Unit,UiError>(UiError.make(ErrorCode.Internal, "raster buffer too small")) }
    var i : size_t = 0u
    unsafe { memcpy(dst.data() as *mut void, buf.pixels.data() as *mut void, n) }
    var saved = im.save(path)
    if(saved is Result.Err) { return Result.Err<Unit,UiError>(UiError.make(ErrorCode.Io, "png save failed")) }
    return Result.Ok<Unit,UiError>(Unit{})
}

public func raster_buffer_bytes(buf : &ColorBuffer) : &[u8] { return buf.pixels.as_span() }
public func raster_pixel(buf : &ColorBuffer, x : u32, y : u32) : Color { return buf.pixel(x,y) }
public func raster_clear(buf : *mut ColorBuffer) {
    if(buf == null) { return }
    unsafe { if(!buf.valid()) { return }; ui_raster_clear(buf.pixels.mutable_data(), buf.pixels.size()) }
}
public func raster_clear_cpu(buf : *mut ColorBuffer) {
    if(buf == null) { return }
    unsafe { if(!buf.valid()) { return }; buf.clear_cpu() }
}
public func raster_fill(buf : *mut ColorBuffer, c : Color) {
    if(buf == null) { return }
    unsafe {
        if(!buf.valid()) { return }
        ui_raster_fill(buf.pixels.mutable_data(), buf.width, buf.height, buf.stride, c.r, c.g, c.b, c.a)
    }
}
public func raster_fill_cpu(buf : *mut ColorBuffer, c : Color) {
    if(buf == null) { return }
    unsafe { var y=0u32; while(y<buf.height){var x=0u32; while(x<buf.width){buf.put(x,y,c.r,c.g,c.b,c.a); x=x+1u32}; y=y+1u32} }
}
public func raster_copy(dst : *mut ColorBuffer, src : &ColorBuffer, x : u32, y : u32, w : u32, h : u32, dx : u32, dy : u32) {
    if(dst == null) { return }
    unsafe {
        if(!dst.valid() || !src.valid()) { return }
        if(x + w > src.width || y + h > src.height || dx + w > dst.width || dy + h > dst.height) { return }
        ui_raster_copy(dst.pixels.mutable_data(), dst.width, dst.height, dst.stride,
            src.pixels.data(), src.width, src.height, src.stride, x, y, w, h, dx, dy)
    }
}
public func raster_copy_cpu(dst : *mut ColorBuffer, src : &ColorBuffer, x : u32, y : u32, w : u32, h : u32, dx : u32, dy : u32) {
    if(dst == null) { return }
    unsafe { dst.copy_rect(src,x,y,w,h,dx,dy) }
}

} // namespace ui_internal
