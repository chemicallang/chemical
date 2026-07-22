using std::Result;
using std::vector;
using std::string;

// ═══════════════════════════════════════════════════════════════
// Font struct and error tests
// ═══════════════════════════════════════════════════════════════

@test
public func font_create_empty_works(env : &mut TestEnv) {
    var f = font::Font.make()
    if(f.loaded) { env.error("unloaded font should not be loaded") }
    if(f.units_per_em != 0) { env.error("units per em should be 0") }
    if(f.num_glyphs != 0) { env.error("num glyphs should be 0") }
}

@test
public func font_error_messages_all_variants(env : &mut TestEnv) {
    var err1 = font::FontError.FileNotFound()
    var msg1 = err1.message()
    if(msg1.size() == 0) { env.error("FileNotFound message empty") }

    var err2 = font::FontError.InvalidFormat(string("bad"))
    var msg2 = err2.message()
    if(msg2.size() == 0) { env.error("InvalidFormat message empty") }

    var err3 = font::FontError.UnsupportedFeature(string("no"))
    var msg3 = err3.message()
    if(msg3.size() == 0) { env.error("UnsupportedFeature message empty") }

    var err4 = font::FontError.IoError(string("disk"))
    var msg4 = err4.message()
    if(msg4.size() == 0) { env.error("IoError message empty") }

    var err5 = font::FontError.GlyphNotFound(65)
    var msg5 = err5.message()
    if(msg5.size() == 0) { env.error("GlyphNotFound message empty") }
}

// ═══════════════════════════════════════════════════════════════
// GlyphMetrics struct
// ═══════════════════════════════════════════════════════════════

@test
public func font_glyph_metrics_struct_works(env : &mut TestEnv) {
    var metrics = font::GlyphMetrics{advance_width: 0, left_side_bearing: 0, xmin: 0, ymin: 0, xmax: 0, ymax: 0, width: 0, height: 0}
    metrics.advance_width = 600
    metrics.left_side_bearing = 50
    metrics.xmin = 10
    metrics.ymin = -20
    metrics.xmax = 500
    metrics.ymax = 700
    metrics.width = 490
    metrics.height = 720
    if(metrics.advance_width != 600) { env.error("advance width") }
    if(metrics.left_side_bearing != 50) { env.error("left side bearing") }
    if(metrics.xmin != 10) { env.error("xmin") }
    if(metrics.ymin != -20) { env.error("ymin") }
    if(metrics.xmax != 500) { env.error("xmax") }
    if(metrics.ymax != 700) { env.error("ymax") }
    if(metrics.width != 490) { env.error("width") }
    if(metrics.height != 720) { env.error("height") }
}

// ═══════════════════════════════════════════════════════════════
// GlyphOutline struct
// ═══════════════════════════════════════════════════════════════

@test
public func font_outline_create_works(env : &mut TestEnv) {
    var outline = font::GlyphOutline.make()
    if(outline.points.size() != 0) { env.error("empty outline has no points") }
    if(outline.flags.size() != 0) { env.error("empty outline has no flags") }
}

@test
public func font_outline_add_data(env : &mut TestEnv) {
    var outline = font::GlyphOutline.make()
    outline.points.push(100)
    outline.points.push(200)
    outline.flags.push(1)
    if(outline.points.size() != 2) { env.error("outline should have 2 points") }
    if(outline.points.get(0) != 100) { env.error("first point X") }
    if(outline.points.get(1) != 200) { env.error("first point Y") }
    if(outline.flags.get(0) != 1) { env.error("first flag") }
}

@test
public func font_outline_instructions(env : &mut TestEnv) {
    var outline = font::GlyphOutline.make()
    outline.instructions.push(0x01)
    outline.instructions.push(0x02)
    if(outline.instructions.size() != 2) { env.error("instructions size") }
}

// ═══════════════════════════════════════════════════════════════
// TextMetrics struct
// ═══════════════════════════════════════════════════════════════

@test
public func text_metrics_struct_works(env : &mut TestEnv) {
    var m = font::TextMetrics{width: 0, height: 0, ascent: 0, descent: 0}
    m.width = 500
    m.height = 20
    m.ascent = 15
    m.descent = 5
    if(m.width != 500) { env.error("width") }
    if(m.height != 20) { env.error("height") }
    if(m.ascent != 15) { env.error("ascent") }
    if(m.descent != 5) { env.error("descent") }
}

// ═══════════════════════════════════════════════════════════════
// Font load error paths (no real font file available)
// ═══════════════════════════════════════════════════════════════

@test
public func font_load_nonexistent_file(env : &mut TestEnv) {
    var result = font::font_load("/tmp/nonexistent_font_file.ttf")
    if(result is Result.Ok) { env.error("should fail on nonexistent file") }
}

@test
public func font_load_garbage_data(env : &mut TestEnv) {
    // Write garbage data to temp file and try to load it
    var garbage : [20]u8; var i : size_t = 0
    while(i < 20) { garbage[i] = i as u8; i += 1 }
    var write_result = fs::write_text_file("/tmp/test_bad_font.ttf", &raw garbage[0], 20)
    if(write_result is Result.Err) { env.error("should write temp file"); return }

    var result = font::font_load("/tmp/test_bad_font.ttf")
    if(result is Result.Ok) { env.error("should fail on garbage data") }
}

@test
public func font_load_too_small(env : &mut TestEnv) {
    var tiny : [4]u8 = [0, 0, 0, 0]
    var write_result = fs::write_text_file("/tmp/test_tiny_font.ttf", &raw tiny[0], 4)
    if(write_result is Result.Err) { env.error("should write temp file"); return }

    var result = font::font_load("/tmp/test_tiny_font.ttf")
    if(result is Result.Ok) { env.error("should fail on <12 byte file") }
}

@test
public func font_load_bad_sfnt_version(env : &mut TestEnv) {
    // Create a minimal file with invalid sfVersion
    var data : [12]u8
    data[0] = 0x00; data[1] = 0x00; data[2] = 0x00; data[3] = 0x00  // sfVersion = 0, not 0x00010000
    data[4] = 0; data[5] = 0  // numTables = 0
    var write_result = fs::write_text_file("/tmp/test_bad_sfnt.ttf", &raw data[0], 12)
    if(write_result is Result.Err) { env.error("should write temp file"); return }

    var result = font::font_load("/tmp/test_bad_sfnt.ttf")
    if(result is Result.Ok) { env.error("should fail on bad sfVersion") }
}

// ═══════════════════════════════════════════════════════════════
// FontTable struct access
// ═══════════════════════════════════════════════════════════════

@test
public func font_table_struct_works(env : &mut TestEnv) {
    var table : font::FontTable
    table.tag = 0x68656164u32  // 'head'
    table.offset = 100
    table.length = 54
    if(table.tag != 0x68656164u32) { env.error("table tag") }
    if(table.offset != 100) { env.error("table offset") }
    if(table.length != 54) { env.error("table length") }
}
