using std::Result;
using std::vector;
using std::string;

// Focused tests for the additive safe font surface: `Font::load`,
// `from_path`, `metrics`, `measure_text` over `&str`, and glyph lookup.

func font_temp_pathbuf(name : *char) : path::PathBuf {
    var s = make_temp_test_path(name)
    return path::PathBuf.from(std::str.make_view(s.to_view()))
}

func font_view_of(v : &vector<u8>) : [u8] {
    return unsafe(std::span<u8>(v.data(), v.size()))
}

func font_fixed_view(p : *u8, n : size_t) : [u8] {
    return unsafe(std::span<u8>(p, n))
}

func font_write_u16_le(out : *mut u8, off : size_t, value : u16) {
    unsafe {
        out[off] = ((value >> 8) & 0xFFu16) as u8
        out[off + 1] = (value & 0xFFu16) as u8
    }
}

func font_write_u32_le(out : *mut u8, off : size_t, value : u32) {
    unsafe {
        out[off] = ((value >> 24) & 0xFFu32) as u8
        out[off + 1] = ((value >> 16) & 0xFFu32) as u8
        out[off + 2] = ((value >> 8) & 0xFFu32) as u8
        out[off + 3] = (value & 0xFFu32) as u8
    }
}

// Minimal sfnt with `head` (units_per_em=1000), `hhea` (800/-200),
// `maxp` (10 glyphs), a format-6 `cmap` mapping `first_code` -> glyph 1, and
// an `hmtx` table giving glyph 1 an advance width of 500.
func font_build_tiny_font(first_code : u16 = 65u16) : vector<u8> {
    const HEAD_OFF : size_t = 92
    const HHEA_OFF : size_t = 146
    const MAXP_OFF : size_t = 182
    const CMAP_OFF : size_t = 214
    const HMTX_OFF : size_t = 238
    const TOTAL : size_t = 248

    var out = vector<u8>()
    out.resize(TOTAL)
    var p = out.data() as *mut u8

    // sfnt header
    font_write_u32_le(p, 0, 0x00010000u32)
    font_write_u16_le(p, 4, 5u16)
    font_write_u16_le(p, 6, 0u16)
    font_write_u16_le(p, 8, 0u16)
    font_write_u16_le(p, 10, 0u16)

    // table directory
    font_write_u32_le(p, 12, 0x68656164u32) // head
    font_write_u32_le(p, 16, 0u32)
    font_write_u32_le(p, 20, HEAD_OFF as u32)
    font_write_u32_le(p, 24, 54u32)

    font_write_u32_le(p, 28, 0x68686561u32) // hhea
    font_write_u32_le(p, 32, 0u32)
    font_write_u32_le(p, 36, HHEA_OFF as u32)
    font_write_u32_le(p, 40, 36u32)

    font_write_u32_le(p, 44, 0x6D617870u32) // maxp
    font_write_u32_le(p, 48, 0u32)
    font_write_u32_le(p, 52, MAXP_OFF as u32)
    font_write_u32_le(p, 56, 32u32)

    font_write_u32_le(p, 60, 0x636D6170u32) // cmap
    font_write_u32_le(p, 64, 0u32)
    font_write_u32_le(p, 68, CMAP_OFF as u32)
    font_write_u32_le(p, 72, 24u32)

    font_write_u32_le(p, 76, 0x686D7478u32) // hmtx
    font_write_u32_le(p, 80, 0u32)
    font_write_u32_le(p, 84, HMTX_OFF as u32)
    font_write_u32_le(p, 88, 8u32)

    // head: units_per_em at +18
    font_write_u16_le(p, HEAD_OFF + 18, 1000u16)

    // hhea: ascent at +4, descent at +6, numberOfHMetrics at +34
    font_write_u16_le(p, HHEA_OFF + 4, 800u16)
    font_write_u16_le(p, HHEA_OFF + 6, 0xFF38u16) // -200
    font_write_u16_le(p, HHEA_OFF + 34, 2u16)

    // maxp: num_glyphs at +4
    font_write_u16_le(p, MAXP_OFF + 4, 10u16)

    // hmtx: glyph 0 advance=0/lsb=0, glyph 1 advance=500/lsb=0
    font_write_u16_le(p, HMTX_OFF, 0u16)
    font_write_u16_le(p, HMTX_OFF + 2, 0u16)
    font_write_u16_le(p, HMTX_OFF + 4, 500u16)
    font_write_u16_le(p, HMTX_OFF + 6, 0u16)

    // cmap: header + one Windows Unicode BMP subtable at offset 12
    font_write_u16_le(p, CMAP_OFF, 0u16)
    font_write_u16_le(p, CMAP_OFF + 2, 1u16)
    font_write_u16_le(p, CMAP_OFF + 4, 3u16)
    font_write_u16_le(p, CMAP_OFF + 6, 1u16)
    font_write_u32_le(p, CMAP_OFF + 8, 12u32)

    var sub = CMAP_OFF + 12
    font_write_u16_le(p, sub, 6u16)            // format 6
    font_write_u16_le(p, sub + 2, 12u16)       // length
    font_write_u16_le(p, sub + 4, 0u16)        // language
    font_write_u16_le(p, sub + 6, first_code)  // firstCode
    font_write_u16_le(p, sub + 8, 1u16)        // entryCount
    font_write_u16_le(p, sub + 10, 1u16)       // glyph id

    return out
}

// ═══════════════════════════════════════════════════════════════
// Positive
// ═══════════════════════════════════════════════════════════════

@test
public func font_api_load_and_metrics(env : &mut TestEnv) {
    var bytes = font_build_tiny_font()
    var loaded = font::Font.load(font_view_of(&bytes))
    if(loaded is Result.Err) { env.error("Font.load should succeed"); return }
    var Ok(font) = loaded else unsafe { unreachable }

    if(!font.is_loaded()) { env.error("font should be loaded") }
    if(font.em_size() != 1000) { env.error("units_per_em should be 1000") }
    if(font.ascender() != 800) { env.error("ascent should be 800") }
    if(font.descender() != -200) { env.error("descent should be -200") }
    if(font.glyph_count() != 10) { env.error("num_glyphs should be 10") }
    if(font.line_height() != 1000) { env.error("line height should be 1000") }

    var metrics = font.metrics(100.0f)
    if(metrics.ascent != 80) { env.error("scaled ascent should be 80") }
    if(metrics.descent != 20) { env.error("scaled descent should be 20") }
    if(metrics.line_height != 100) { env.error("scaled line height should be 100") }
    if(metrics.units_per_em != 1000) { env.error("metrics units_per_em") }
    if(metrics.num_glyphs != 10) { env.error("metrics num_glyphs") }
}

@test
public func font_api_from_path_roundtrip(env : &mut TestEnv) {
    var bytes = font_build_tiny_font()
    var p = font_temp_pathbuf("p24_safe_font.ttf")

    var ps = p.as_string()
    var wr = fs::raw::write_text_file(ps.c_str(), bytes.data(), bytes.size())
    if(wr is Result.Err) { env.error("failed to write font fixture"); return }

    var loaded = font::Font.from_path(&p.as_path())
    if(loaded is Result.Err) { env.error("Font.from_path should succeed"); return }
    var Ok(font) = loaded else unsafe { unreachable }
    if(font.em_size() != 1000) { env.error("loaded units_per_em") }
    if(font.glyph_count() != 10) { env.error("loaded num_glyphs") }
}

@test
public func font_api_measure_text_and_glyph(env : &mut TestEnv) {
    var bytes = font_build_tiny_font()
    var loaded = font::Font.load(font_view_of(&bytes))
    if(loaded is Result.Err) { env.error("Font.load should succeed"); return }
    var Ok(font) = loaded else unsafe { unreachable }

    var glyph = font.glyph(65)
    if(glyph is Result.Err) { env.error("'A' should map to a glyph"); return }
    var Ok(glyph_id) = glyph else unsafe { unreachable }
    if(glyph_id != 1) { env.error("'A' should map to glyph 1") }

    var missing = font.glyph(66)
    if(missing is Result.Ok) { env.error("unmapped codepoint should fail") }

    var measured = font.measure_text("A", 100.0f)
    if(measured.ascent != 80) { env.error("measured ascent should be 80") }
    if(measured.descent != 20) { env.error("measured descent should be 20") }
}

@test
public func font_api_measure_text_decodes_utf8(env : &mut TestEnv) {
    // 'é' is U+00E9 (two UTF-8 bytes). The cmap maps that codepoint to glyph 1,
    // whose advance is 500 units at 1000 upem, so at 100px the width is 50.
    var bytes = font_build_tiny_font(233u16)
    var loaded = font::Font.load(font_view_of(&bytes))
    if(loaded is Result.Err) { env.error("UTF-8 fixture should load"); return }
    var Ok(font) = loaded else unsafe { unreachable }

    var measured = font.measure_text("é", 100.0f)
    if(measured.width != 50) { env.error("two-byte UTF-8 codepoint should advance once") }

    // An unmapped multi-byte codepoint contributes no width but must not be
    // measured as two separate continuation bytes either.
    var missing = font.measure_text("ø", 100.0f)
    if(missing.width != 0) { env.error("unmapped codepoint should not add width") }
}

// ═══════════════════════════════════════════════════════════════
// Misuse
// ═══════════════════════════════════════════════════════════════

@test
public func font_api_bad_magic_rejected(env : &mut TestEnv) {
    var garbage : [16]u8
    var i : size_t = 0
    while(i < 16) { garbage[i] = 0xAB; i += 1 }
    var view = font_fixed_view(&raw garbage[0], 16)
    var result = font::Font.load(view)
    if(result is Result.Ok) { env.error("bad font magic must be rejected") }
}

@test
public func font_api_truncated_rejected(env : &mut TestEnv) {
    var tiny : [4]u8 = [0x00, 0x01, 0x00, 0x00]
    var view = font_fixed_view(&raw tiny[0], 4)
    var result = font::Font.load(view)
    if(result is Result.Ok) { env.error("truncated font must be rejected") }
}

@test
public func font_api_from_path_nonexistent(env : &mut TestEnv) {
    var p = font_temp_pathbuf("p24_no_such_font.ttf")
    var result = font::Font.from_path(&p.as_path())
    if(result is Result.Ok) { env.error("missing font file must fail") }
}

@test
public func font_api_rejects_unloaded_font(env : &mut TestEnv) {
    var font = font::Font.make()
    if(font.is_loaded()) { env.error("fresh Font should not be loaded") }
    if(font.validate() is Result.Ok) { env.error("unloaded Font must not validate") }
    var glyph = font.glyph(65)
    if(glyph is Result.Ok) { env.error("unloaded Font must not resolve glyphs") }
}
