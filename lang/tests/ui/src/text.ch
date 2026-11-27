// P1.5b font cmap-4 oracle, pinned-metrics fixture, wrapping and rich spans.
using std::Option;
using std::Result;
using std::string_view;
using std::vector;
using ui_internal::text_context_from_font;
using ui_internal::empty_text_context;

public namespace text_tests {

func close(a : float, b : float) : bool {
    var d = a - b
    if(d < 0.0f) { d = 0.0f - d }
    return d < 0.01f
}
func fail(name : *char) : bool {
    unsafe { printf("text_metrics FAIL %s\n", name) }
    return false
}
func asset_path() : path::PathBuf {
    return path::PathBuf.from("lang/tests/ui/assets/fonts/LiberationSans-Regular.ttf")
}
func metrics_path() : path::PathBuf {
    return path::PathBuf.from("lang/tests/ui/assets/metrics.json")
}
func load_asset() : Result<font::Font, font::FontError> {
    var p = asset_path()
    return font::Font.from_path(&p.as_path())
}
func font_view_of(v : &vector<u8>) : [u8] {
    return unsafe(std::span<u8>(v.data(), v.size()))
}
func write_u16_be(out : *mut u8, off : size_t, value : u16) {
    unsafe {
        out[off] = ((value >> 8u16) & 0xFFu16) as u8
        out[off + 1u] = (value & 0xFFu16) as u8
    }
}
func write_u32_be(out : *mut u8, off : size_t, value : u32) {
    unsafe {
        out[off] = ((value >> 24u32) & 0xFFu32) as u8
        out[off + 1u] = ((value >> 16u32) & 0xFFu32) as u8
        out[off + 2u] = ((value >> 8u32) & 0xFFu32) as u8
        out[off + 3u] = (value & 0xFFu32) as u8
    }
}
// Synthetic format-4 font: A -> glyph 2, eacute -> glyph 3, sentinel -> 0.
// This is deliberately different from Liberation Sans (A -> 36), so a regression
// in the segment-array offsets cannot accidentally pass.
func build_cmap4_font() : vector<u8> {
    const HEAD_OFF : size_t = 92
    const HHEA_OFF : size_t = 146
    const MAXP_OFF : size_t = 182
    const CMAP_OFF : size_t = 214
    const HMTX_OFF : size_t = 282
    const TOTAL : size_t = 298
    var out = vector<u8>()
    out.resize(TOTAL)
    var p = out.data() as *mut u8
    write_u32_be(p, 0, 0x00010000u32)
    write_u16_be(p, 4, 5u16); write_u16_be(p, 6, 0u16); write_u16_be(p, 8, 0u16); write_u16_be(p, 10, 0u16)
    write_u32_be(p, 12, 0x68656164u32); write_u32_be(p, 16, 0u32); write_u32_be(p, 20, HEAD_OFF as u32); write_u32_be(p, 24, 54u32)
    write_u32_be(p, 28, 0x68686561u32); write_u32_be(p, 32, 0u32); write_u32_be(p, 36, HHEA_OFF as u32); write_u32_be(p, 40, 36u32)
    write_u32_be(p, 44, 0x6D617870u32); write_u32_be(p, 48, 0u32); write_u32_be(p, 52, MAXP_OFF as u32); write_u32_be(p, 56, 32u32)
    write_u32_be(p, 60, 0x636D6170u32); write_u32_be(p, 64, 0u32); write_u32_be(p, 68, CMAP_OFF as u32); write_u32_be(p, 72, 56u32)
    write_u32_be(p, 76, 0x686D7478u32); write_u32_be(p, 80, 0u32); write_u32_be(p, 84, HMTX_OFF as u32); write_u32_be(p, 88, 16u32)
    write_u16_be(p, HEAD_OFF + 18u, 1000u16)
    write_u16_be(p, HHEA_OFF + 4u, 800u16); write_u16_be(p, HHEA_OFF + 6u, 0xFF38u16); write_u16_be(p, HHEA_OFF + 8u, 100u16); write_u16_be(p, HHEA_OFF + 34u, 4u16)
    write_u16_be(p, MAXP_OFF + 4u, 10u16)
    write_u16_be(p, CMAP_OFF, 0u16); write_u16_be(p, CMAP_OFF + 2u, 1u16)
    write_u16_be(p, CMAP_OFF + 4u, 3u16); write_u16_be(p, CMAP_OFF + 6u, 1u16); write_u32_be(p, CMAP_OFF + 8u, 12u32)
    var sub = CMAP_OFF + 12u
    write_u16_be(p, sub, 4u16); write_u16_be(p, sub + 2u, 44u16); write_u16_be(p, sub + 4u, 0u16)
    write_u16_be(p, sub + 6u, 6u16); write_u16_be(p, sub + 8u, 4u16); write_u16_be(p, sub + 10u, 1u16); write_u16_be(p, sub + 12u, 2u16)
    // endCode[3], reservedPad, startCode[3], idDelta[3], idRangeOffset[3]
    write_u16_be(p, sub + 14u, 0x41u16); write_u16_be(p, sub + 16u, 0xE9u16); write_u16_be(p, sub + 18u, 0xFFFFu16)
    write_u16_be(p, sub + 20u, 0u16)
    write_u16_be(p, sub + 22u, 0x41u16); write_u16_be(p, sub + 24u, 0xE9u16); write_u16_be(p, sub + 26u, 0xFFFFu16)
    write_u16_be(p, sub + 28u, 0u16); write_u16_be(p, sub + 30u, 0u16); write_u16_be(p, sub + 32u, 1u16)
    write_u16_be(p, sub + 34u, 6u16); write_u16_be(p, sub + 36u, 6u16); write_u16_be(p, sub + 38u, 0u16)
    write_u16_be(p, sub + 40u, 2u16); write_u16_be(p, sub + 42u, 3u16)
    // hmtx: glyph 0..3, then A->600 and eacute->700 units.
    write_u16_be(p, HMTX_OFF, 0u16); write_u16_be(p, HMTX_OFF + 2u, 0u16)
    write_u16_be(p, HMTX_OFF + 4u, 500u16); write_u16_be(p, HMTX_OFF + 6u, 0u16)
    write_u16_be(p, HMTX_OFF + 8u, 600u16); write_u16_be(p, HMTX_OFF + 10u, 0u16)
    write_u16_be(p, HMTX_OFF + 12u, 700u16); write_u16_be(p, HMTX_OFF + 14u, 0u16)
    return out
}
func base_style() : TextStyle { return Theme.light().text(TextRole.Body) }
func constraints(maxw : float, max_lines : u16, wrap : WrapMode) : TextConstraints {
    return TextConstraints { max_width : maxw, max_lines : max_lines, wrap : wrap,
        overflow : TextOverflow.Clip, tab_size : 4u8, direction : Direction.Ltr }
}
func check_font_cmap() : bool {
    var loaded = load_asset()
    if(loaded is Result.Err) { return fail("asset-load") }
    var Ok(f) = loaded else return false
    if(f.em_size() != 2048u16 || f.ascender() != 1854i16 || f.descender() != -434i16 || f.line_gap() != 307i16) { return fail("font-metrics") }
    if(f.cap_height() != 1409i16 || f.x_height() != 1082i16) { return fail("font-cap-x") }
    var a = f.glyph(0x41u32)
    if(a is Result.Err) { return fail("cmap-A-missing") }
    var Ok(ag) = a else return false
    var e = f.glyph(0xE9u32)
    if(e is Result.Err) { return fail("cmap-eacute-missing") }
    var Ok(eg) = e else return false
    if(ag != 36u16 || eg != 171u16) { return fail("cmap-oracle") }
    var am = f.glyph_metrics(ag)
    if(am is Result.Err) { return fail("hmtx-A") }
    var Ok(amm) = am else return false
    if(amm.advance_width != 1366) { return fail("advance-A") }
    if(f.glyph(0x4E2Du32) is Result.Ok) { return fail("missing-cjk") }
    // Synthetic format-4 font where A is glyph 2, not 36.
    var synthetic = build_cmap4_font()
    var sl = font::Font.load(font_view_of(&synthetic))
    if(sl is Result.Err) { return fail("synthetic-load") }
    var Ok(sf) = sl else return false
    var sa = sf.glyph(0x41u32)
    var se = sf.glyph(0xE9u32)
    if(sa is Result.Err || se is Result.Err) { return fail("synthetic-cmap") }
    var Ok(sag) = sa else return false
    var Ok(seg) = se else return false
    if(sag != 2u16 || seg != 3u16) { return fail("synthetic-oracle") }
    // units_per_em == 0 must be rejected, not measured as a zero-scale font.
    var bad = build_cmap4_font()
    write_u16_be(bad.data() as *mut u8, 110u, 0u16)
    var bad_load = font::Font.load(font_view_of(&bad))
    if(bad_load is Result.Ok) { return fail("units-per-em-zero") }
    return true
}
func check_metrics_json_present() : bool {
    var p = metrics_path()
    var got = fs::read_to_string(&p.as_path())
    if(got is Result.Err) { return fail("metrics-json-read") }
    var Ok(text) = got else return false
    var view = text.to_view()
    // Keep the committed oracle honest even though the compiled test uses the
    // exact constants below (JSON variants are destructible in this surface).
    var checks : [6]string_view = ["\"ascent\": 1854", "\"descent\": 434", "\"line_gap\": 307", "\"cap_height\": 1409", "\"x_height\": 1082", "\"units_per_em\": 2048"]
    var i : size_t = 0u
    while(i < 6u) {
        if(!view.contains(&checks[i])) { return fail("metrics-json-value") }
        i = i + 1u
    }
    return true
}
func check_text_metrics() : bool {
    var loaded = load_asset()
    if(loaded is Result.Err) { return fail("text-asset-load") }
    var Ok(f) = loaded else return false
    var ctx = text_context_from_font(f)
    var st = base_style()
    var c = constraints(0.0f, 0u16, WrapMode.NoWrap)
    var a = ctx.measure("A", st, c)
    if(!close(a.width, 9.337890625f) || !close(a.height, 20.0f) || a.lines != 1u32 || !close(a.line_gap, 2.0986328125f) || !close(a.cap_height, 9.6318359375f) || !close(a.x_height, 7.396484375f)) { return fail("measure-A") }
    if(!close(a.first_baseline, 13.80419921875f) || !close(a.last_baseline, 13.80419921875f)) { return fail("baseline-A") }
    var two = ctx.measure("A\nB", st, c)
    if(!close(two.width, 9.337890625f) || !close(two.height, 40.0f) || two.lines != 2u32 || !close(two.last_baseline, 33.80419921875f)) { return fail("measure-two-lines") }
    var empty = ctx.measure("", st, c)
    if(empty.width != 0.0f || empty.height != 0.0f || empty.lines != 0u32) { return fail("measure-empty") }
    var missing = ctx.measure("中", st, c)
    if(!close(missing.width, 7.0f) || missing.lines != 1u32) { unsafe { printf("missing width=%.6f lines=%u\n", missing.width, missing.lines) }; return fail("measure-missing") }
    var both = ctx.measure("A中", st, c)
    if(!close(both.width, 16.337890625f)) { return fail("measure-fallback") }
    var zero = empty_text_context()
    var z = zero.measure("A", st, c)
    if(!close(z.width, 7.0f) || z.lines != 1u32) { return fail("zero-font") }
    var inf = 1.0f / 0.0f
    var nf = ctx.measure("A", st, constraints(inf, 0u16, WrapMode.NoWrap))
    if(nf.width != nf.width || nf.height != nf.height || !close(nf.width, a.width)) { return fail("nonfinite-width") }
    var bad_style = st; bad_style.size = inf
    var bs = ctx.measure("A", bad_style, c)
    if(bs.width != bs.width || bs.height != bs.height) { return fail("nonfinite-style") }
    // Weight does not change advances until shaping/GPOS (P2.3).
    var bold = st; bold.weight = FontWeight.Bold
    var bw = ctx.measure("A", bold, c)
    if(!close(bw.width, a.width)) { return fail("weight-metrics") }
    return true
}
func check_wrap_and_rich() : bool {
    var loaded = load_asset()
    if(loaded is Result.Err) { return fail("wrap-load") }
    var Ok(f) = loaded else return false
    var ctx = text_context_from_font(f)
    var st = base_style()
    var wrapped = ctx.measure("hello world", st, constraints(40.0f, 0u16, WrapMode.Wrap))
    if(wrapped.lines != 2u32 || !close(wrapped.width, 33.455078125f)) { return fail("wrap-two-lines") }
    var clipped = ctx.measure("hello world", st, constraints(40.0f, 1u16, WrapMode.Wrap))
    if(clipped.lines != 1u32 || !clipped.truncated || !close(clipped.width, 29.5791015625f)) { return fail("wrap-truncated") }
    var spans = vector<TextSpan>()
    spans.push(TextSpan { text : "Hello", role : TextRole.Body, color : ColorToken.OnSurface, weight : FontWeight.Regular, italic : false, family : FontFamily.Ui, features : TextFeatures { ligatures : false, tabular_numbers : false, slashed_zero : false, stylistic_set : 0u8, raw_tags : zeroed<[0]u32>() }, link : Option.None<string_view>(), decoration : 0u8 })
    spans.push(TextSpan { text : "world", role : TextRole.BodyStrong, color : ColorToken.OnSurface, weight : FontWeight.Bold, italic : false, family : FontFamily.Ui, features : TextFeatures { ligatures : false, tabular_numbers : false, slashed_zero : false, stylistic_set : 0u8, raw_tags : zeroed<[0]u32>() }, link : Option.None<string_view>(), decoration : 0u8 })
    var rich = ctx.measure_rich(spans.as_span(), st, constraints(0.0f, 0u16, WrapMode.NoWrap))
    if(!close(rich.width, 65.3583984375f) || rich.lines != 1u32) { return fail("rich-spans") }
    return true
}
public func run_text_metrics() : bool {
    if(!check_font_cmap()) { return false }
    if(!check_metrics_json_present()) { return false }
    if(!check_text_metrics()) { return false }
    if(!check_wrap_and_rich()) { return false }
    return true
}
}
