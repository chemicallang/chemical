using std::Result;

@test
public func font_create_empty_works(env : &mut TestEnv) {
    var f = font::Font.make()
    if(f.loaded) { env.error("unloaded font should not be loaded") }
    if(f.units_per_em != 0) { env.error("units per em should be 0") }
    if(f.num_glyphs != 0) { env.error("num glyphs should be 0") }
}

@test
public func font_glyph_metrics_struct_works(env : &mut TestEnv) {
    var metrics = font::GlyphMetrics{advance_width: 0, left_side_bearing: 0, xmin: 0, ymin: 0, xmax: 0, ymax: 0, width: 0, height: 0}
    metrics.advance_width = 600
    metrics.left_side_bearing = 50
    if(metrics.advance_width != 600) { env.error("advance width") }
    if(metrics.left_side_bearing != 50) { env.error("left side bearing") }
}

@test
public func font_outline_create_works(env : &mut TestEnv) {
    var outline = font::GlyphOutline.make()
    if(outline.points.size() != 0) { env.error("empty outline has no points") }
    if(outline.flags.size() != 0) { env.error("empty outline has no flags") }
}

@test
public func font_error_messages_work(env : &mut TestEnv) {
    var err = font::FontError.FileNotFound()
    var msg = err.message()
    if(msg.size() == 0) { env.error("error message should not be empty") }
}

@test
public func text_metrics_struct_works(env : &mut TestEnv) {
    var m = font::TextMetrics{width: 0, height: 0, ascent: 0, descent: 0}
    m.width = 500
    m.height = 20
    m.ascent = 15
    m.descent = 5
    if(m.width != 500) { env.error("width") }
    if(m.height != 20) { env.error("height") }
}
