// Safe font surface: `&[u8]` parsing, `&path::Path` loading, `&str` text
// measurement, `Result` glyph lookups, and owned `Font` values. The legacy raw
// functions in `ttf.ch` remain available; new code should use the methods on
// `Font` or the helpers here (no `*mut Font` or `*char` in the new surface).
public namespace font {

using std::Result;
using std::string;
using std::vector;

// Scaled font metrics. All pixel values are relative to `units_per_em`.
@direct_init
public struct FontMetrics {
    var units_per_em : u16
    var ascent : i32
    var descent : i32
    var line_height : i32
    var num_glyphs : u16
}

// ── loading ───────────────────────────────────────────────────────────────

func font_safe_from_path(path : &path::Path) : Result<Font, FontError> {
    var ps = path.as_string()
    var read = fs::read_entire_file(ps.c_str())
    if(read is Result.Err) {
        return Result.Err(FontError.FileNotFound())
    }
    var Ok(data) = read else unsafe { unreachable }
    return font_safe_load(data.data(), data.size())
}

func font_safe_load(data : *u8, len : size_t) : Result<Font, FontError> {
    var parsed = Font.make()
    unsafe {
        var i : size_t = 0
        while(i < len) {
            parsed.data.push(data[i])
            i += 1
        }
    }

    var header = parse_font_header(&raw mut parsed)
    if(header is Result.Err) {
        var Err(e) = header else unsafe { unreachable }
        return Result.Err(e)
    }
    parsed.loaded = true
    return Result.Ok(parsed)
}

// ── metrics ───────────────────────────────────────────────────────────────

func font_safe_metrics(f : &Font, font_size : float) : FontMetrics {
    var scale : float = 0.0f
    if(f.units_per_em > 0) {
        scale = font_size / (f.units_per_em as float)
    }
    return FontMetrics {
        units_per_em : f.units_per_em,
        ascent : (f.ascent as float * scale) as i32,
        descent : (-f.descent as float * scale) as i32,
        line_height : (font_line_height(unsafe(f as *mut Font)) as float * scale) as i32,
        num_glyphs : f.num_glyphs
    }
}

} // end namespace font
