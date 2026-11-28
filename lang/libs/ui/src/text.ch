// P1.5b DOM-free, shaping-free text metrics. Fonts are opt-in; this module
// never links or embeds a font. No rasterization/atlas/kerning yet (P2.2/P2.3).
using std::Option;
using std::Result;
using std::string_view;

@direct_init
public struct TextLayoutMetrics {
    var width : float
    var height : float
    var first_baseline : float
    var last_baseline : float
    var line_gap : float
    var lines : u32
    var truncated : bool
    var cap_height : float
    var x_height : float
}

@direct_init
public struct TextConstraints {
    var max_width : float
    var max_lines : u16
    var wrap : WrapMode
    var overflow : TextOverflow
    var tab_size : u8
    var direction : Direction
}

@direct_init
public struct TextContext {
    internal var font_value : font::Font
    internal var has_font : bool
    internal var fallback_advance_em : float

    // TextContext is intentionally internal/test-constructible in P1.5b.
    public func measure(&self, text : string_view, style : TextStyle, c : TextConstraints) : TextLayoutMetrics {
        return text_measure_impl(self, text, style, c)
    }

    public func measure_rich(&self, spans : &[TextSpan], style : TextStyle, c : TextConstraints) : TextLayoutMetrics {
        var out = TextLayoutMetrics { width : 0.0f, height : 0.0f, first_baseline : 0.0f,
            last_baseline : 0.0f, line_gap : 0.0f, lines : 0u32, truncated : false,
            cap_height : 0.0f, x_height : 0.0f }
        var i : size_t = 0
        while(i < spans.size()) {
            var span = spans[i]
            var m = text_measure_impl(self, span.text, style, c)
            out.width = out.width + m.width
            if(m.height > out.height) { out.height = m.height }
            if(m.first_baseline > out.first_baseline) { out.first_baseline = m.first_baseline }
            if(m.last_baseline > out.last_baseline) { out.last_baseline = m.last_baseline }
            if(m.line_gap > out.line_gap) { out.line_gap = m.line_gap }
            if(m.lines > out.lines) { out.lines = m.lines }
            if(m.truncated) { out.truncated = true }
            if(m.cap_height > out.cap_height) { out.cap_height = m.cap_height }
            if(m.x_height > out.x_height) { out.x_height = m.x_height }
            i = i + 1u
        }
        return out
    }
}

public namespace ui_internal {
public func empty_text_context() : TextContext {
    return TextContext { font_value : font::Font.make(), has_font : false, fallback_advance_em : 0.5f }
}

public func text_context_from_font(f : font::Font) : TextContext {
    return TextContext { font_value : f, has_font : true, fallback_advance_em : 0.5f }
}
}

@direct_init
struct Utf8Step {
    var cp : u32
    var step : size_t
}

func text_utf8_next(data : *char, len : size_t, pos : size_t) : Utf8Step {
    unsafe {
        if(pos >= len) { return Utf8Step { cp : 0u32, step : 0u } }
        var b0 = data[pos] as u8
        if(b0 < 0x80u8) { return Utf8Step { cp : b0 as u32, step : 1u } }
        if(b0 < 0xC2u8 || b0 > 0xF4u8) { return Utf8Step { cp : 0xFFFDu32, step : 1u } }
        var need : size_t = 0u
        var cp : u32 = 0u32
        if(b0 <= 0xDFu8) { need = 1u; cp = (b0 as u32) & 0x1Fu32 }
        else if(b0 <= 0xEFu8) { need = 2u; cp = (b0 as u32) & 0x0Fu32 }
        else { need = 3u; cp = (b0 as u32) & 0x07u32 }
        if(pos + need >= len) { return Utf8Step { cp : 0xFFFDu32, step : 1u } }
        var i : size_t = 1u
        while(i <= need) {
            var b = (data[pos + i] as u8) as u32
            if(b < 0x80u32 || b > 0xBFu32) { return Utf8Step { cp : 0xFFFDu32, step : 1u } }
            cp = (cp << 6u32) | (b & 0x3Fu32)
            i = i + 1u
        }
        if(need == 2u && cp < 0x800u32) { return Utf8Step { cp : 0xFFFDu32, step : 1u } }
        if(need == 3u && cp < 0x10000u32) { return Utf8Step { cp : 0xFFFDu32, step : 1u } }
        if(cp >= 0xD800u32 && cp <= 0xDFFFu32) { return Utf8Step { cp : 0xFFFDu32, step : 1u } }
        if(cp > 0x10FFFFu32) { return Utf8Step { cp : 0xFFFDu32, step : 1u } }
        return Utf8Step { cp : cp, step : need + 1u }
    }
}

func text_clamp_finite(v : float, fallback : float) : float {
    if(v != v) { return fallback }
    if(v > 1000000.0f || v < -1000000.0f) { return fallback }
    return v
}

func text_advance(ctx : &TextContext, cp : u32, font_size : float, scale : float, fallback : float) : float {
    if(ctx.has_font) {
        var glyph = ctx.font_value.glyph(cp)
        if(glyph is Result.Ok) {
            var Ok(gid) = glyph else return fallback
            var gm = ctx.font_value.glyph_metrics(gid)
            if(gm is Result.Ok) {
                var Ok(m) = gm else return fallback
                return (m.advance_width as float) * scale
            }
        }
    }
    return fallback
}

func text_is_space(cp : u32) : bool { return cp == 0x20u32 || cp == 0x09u32 || cp == 0x0Du32 }

@direct_init
struct TextLineState {
    var line_width : float
    var max_line_width : float
    var lines : u32
    var has_content : bool
    var truncated : bool
}

func text_finish_line(s : &mut TextLineState) {
    if(s.line_width > s.max_line_width) { s.max_line_width = s.line_width }
    s.lines = s.lines + 1u32
    s.line_width = 0.0f
    s.has_content = false
}

func text_measure_impl(ctx : &TextContext, text : string_view, style : TextStyle, c : TextConstraints) : TextLayoutMetrics {
    var size = text_clamp_finite(style.size, 14.0f)
    if(size <= 0.0f) { size = 14.0f }
    var ratio = text_clamp_finite(style.line_height, 1.2f)
    if(ratio <= 0.0f) { ratio = 1.2f }
    var max_width = text_clamp_finite(c.max_width, 0.0f)
    if(max_width <= 0.0f) { max_width = 0.0f }
    var upem = 0u16
    var ascent_u : i16 = 0
    var descent_u : i16 = 0
    var gap_u : i16 = 0
    var cap_u : i16 = 0
    var x_u : i16 = 0
    if(ctx.has_font) {
        upem = ctx.font_value.em_size()
        ascent_u = ctx.font_value.ascender()
        descent_u = ctx.font_value.descender()
        gap_u = ctx.font_value.line_gap()
        cap_u = ctx.font_value.cap_height()
        x_u = ctx.font_value.x_height()
    }
    var scale = 0.0f
    if(upem > 0u16) { scale = size / (upem as float) }
    var ascent = 0.0f
    var descent = 0.0f
    var line_gap = 0.0f
    if(ascent_u > 0i16) { ascent = (ascent_u as float) * scale }
    if(descent_u < 0i16) { descent = (-(descent_u as i32) as float) * scale }
    if(gap_u > 0i16) { line_gap = (gap_u as float) * scale }
    var cap_height = 0.0f
    var x_height = 0.0f
    if(cap_u > 0i16) { cap_height = (cap_u as float) * scale }
    if(x_u > 0i16) { x_height = (x_u as float) * scale }
    var font_line = ascent + descent + line_gap
    var line_box = size * ratio
    if(line_box < font_line) { line_box = font_line }
    var first_baseline = ascent
    if(line_box > font_line) { first_baseline = ascent + (line_box - font_line) * 0.5f }
    var fallback = size * ctx.fallback_advance_em
    var data = text.data()
    var len = text.size()
    var i : size_t = 0u
    var word_width = 0.0f
    var space_width = 0.0f
    var has_word = false
    var saw_any = false
    var state = TextLineState { line_width : 0.0f, max_line_width : 0.0f, lines : 0u32,
        has_content : false, truncated : false }
    while(i < len) {
        var u = text_utf8_next(data, len, i)
        var cp = u.cp
        var adv = 0.0f
        if(cp == 0x09u32) {
            var space = text_advance(ctx, 0x20u32, size, scale, fallback)
            var tabs = c.tab_size
            if(tabs == 0u8) { tabs = style.tab_size }
            if(tabs == 0u8) { tabs = 4u8 }
            adv = space * (tabs as float)
        } else {
            adv = text_advance(ctx, cp, size, scale, fallback)
        }
        saw_any = true
        if(cp == 0x0Au32) {
            if(has_word) {
                var add = space_width + word_width
                if(c.wrap != WrapMode.NoWrap && state.has_content && max_width > 0.0f && state.line_width + add > max_width) {
                    text_finish_line(&mut state)
                    if(c.max_lines > 0u16 && state.lines >= c.max_lines as u32) { state.truncated = true; break }
                    state.line_width = word_width
                } else {
                    state.line_width = state.line_width + add
                }
                state.has_content = true
                has_word = false; word_width = 0.0f; space_width = 0.0f
            }
            if(state.has_content) {
                text_finish_line(&mut state)
                if(c.max_lines > 0u16 && state.lines >= c.max_lines as u32) {
                    // There is a newline after this line; mark truncated if more bytes remain.
                    if(i + u.step < len) { state.truncated = true }
                    if(state.truncated) { break }
                }
            }
            i = i + u.step
            continue
        }
        if(text_is_space(cp)) {
            if(has_word) {
                var add = space_width + word_width
                if(c.wrap != WrapMode.NoWrap && state.has_content && max_width > 0.0f && state.line_width + add > max_width) {
                    text_finish_line(&mut state)
                    if(c.max_lines > 0u16 && state.lines >= c.max_lines as u32) { state.truncated = true; break }
                    state.line_width = word_width
                } else {
                    state.line_width = state.line_width + add
                }
                state.has_content = true
                has_word = false; word_width = 0.0f; space_width = 0.0f
            }
            space_width = space_width + adv
        } else {
            word_width = word_width + adv
            has_word = true
        }
        i = i + u.step
    }
    if(has_word && !state.truncated) {
        var add = space_width + word_width
        if(c.wrap != WrapMode.NoWrap && state.has_content && max_width > 0.0f && state.line_width + add > max_width) {
            text_finish_line(&mut state)
            if(c.max_lines == 0u16 || state.lines < c.max_lines as u32) { state.line_width = word_width }
            else { state.truncated = true }
        } else {
            state.line_width = state.line_width + add
        }
        state.has_content = true
    }
    if(state.has_content && !state.truncated) { text_finish_line(&mut state) }
    if(state.truncated && state.lines == 0u32) {
        // A single truncated line still has a visible line.
        state.lines = 1u32
    }
    var height = 0.0f
    if(state.lines > 0u32) { height = line_box * (state.lines as float) }
    var last = 0.0f
    if(state.lines > 0u32) { last = first_baseline + line_box * ((state.lines - 1u32) as float) }
    var width = state.max_line_width
    if(state.lines == 0u32) { width = 0.0f }
    return TextLayoutMetrics {
        width : width,
        height : height,
        first_baseline : first_baseline,
        last_baseline : last,
        line_gap : line_gap,
        lines : state.lines,
        truncated : state.truncated,
        cap_height : cap_height,
        x_height : x_height
    }
}

func empty_text_metrics() : TextLayoutMetrics {
    return TextLayoutMetrics { width : 0.0f, height : 0.0f, first_baseline : 0.0f,
        last_baseline : 0.0f, line_gap : 0.0f, lines : 0u32, truncated : false,
        cap_height : 0.0f, x_height : 0.0f }
}
