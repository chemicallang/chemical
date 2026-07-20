public namespace font {

using std::Result;
using std::string;
using std::vector;
using fs::File;
using fs::OpenOptions;

// ---------------------------------------------------------------------------
// TTF/OTF font parser
// ---------------------------------------------------------------------------
// Parses TrueType font tables and extracts glyph data.
// Does NOT perform hinting, ligatures, or complex shaping.

func read_u16_be(data : *u8, offset : size_t) : u16 {
    return ((data[offset] as u16) << 8) | (data[offset + 1] as u16)
}

func read_u32_be(data : *u8, offset : size_t) : u32 {
    return ((data[offset] as u32) << 24) | ((data[offset + 1] as u32) << 16) |
           ((data[offset + 2] as u32) << 8) | (data[offset + 3] as u32)
}

func read_i16_be(data : *u8, offset : size_t) : i16 {
    var val = read_u16_be(data, offset)
    return val as i16
}

func read_i32_be(data : *u8, offset : size_t) : i32 {
    var val = read_u32_be(data, offset)
    return val as i32
}

@direct_init
public struct Font {
    var data : vector<u8>
    var tables : vector<FontTable>
    var units_per_em : u16
    var ascent : i16
    var descent : i16
    var num_glyphs : u16
    var loaded : bool

    @make
    func make() : Font {
        return Font {
            data : vector<u8>(),
            tables : vector<FontTable>(),
            units_per_em : 0,
            ascent : 0,
            descent : 0,
            num_glyphs : 0,
            loaded : false
        }
    }
}

public struct FontTable {
    var tag : u32
    var offset : u32
    var length : u32
}

public func font_load(path : *char) : std::Result<Font, FontError> {
    var read_result = fs::read_entire_file(path)
    if(read_result is Result.Err) {
        return std.Result.Err(FontError.FileNotFound())
    }

    var font = Font.make()
    var Ok(data) = read_result else unreachable
    memcpy(&raw mut font.data, &raw data, sizeof(vector<u8>))
    new(&raw data) vector<u8>()

    var parse_result = parse_font_header(&raw mut font)
    if(parse_result is Result.Err) {
        return std.Result.Err(FontError.InvalidFormat(string("failed to parse font header")))
    }

    font.loaded = true
    return std.Result.Ok(font)
}

func parse_font_header(font : *mut Font) : std::Result<std::Unit, FontError> {
    if(font.data.size() < 12) {
        return std.Result.Err(FontError.InvalidFormat(string("font file too small")))
    }

    var sfnt_version = read_u32_be(font.data.data(), 0)

    // TrueType = 0x00010000, OpenType with CFF = 'OTTO', OpenType with TrueType = 0x00010000
    if(sfnt_version != 0x00010000u32 && sfnt_version != 0x4F54544Fu32) {
        return std.Result.Err(FontError.InvalidFormat(string("not a TrueType or OpenType font")))
    }

    var num_tables = read_u16_be(font.data.data(), 4)

    // Parse table directory
    var i : u16 = 0
    while(i < num_tables) {
        var entry_offset = 12 + (i as size_t) * 16
        if(entry_offset + 16 > font.data.size()) { break }

        var tag = read_u32_be(font.data.data(), entry_offset)
        var checksum = read_u32_be(font.data.data(), entry_offset + 4)
        var table_offset = read_u32_be(font.data.data(), entry_offset + 8)
        var table_length = read_u32_be(font.data.data(), entry_offset + 12)

        var table : FontTable
        table.tag = tag
        table.offset = table_offset
        table.length = table_length
        font.tables.push(table)

        i += 1
    }

    // Read head table for units_per_em
    var head_result = find_table(font, 0x68656164u32) // 'head'
    if(head_result is Result.Ok) {
        var Ok(head) = head_result else unreachable
        if((head.offset as size_t) + 54 <= font.data.size()) {
            font.units_per_em = read_u16_be(font.data.data(), (head.offset as size_t) + 18)
        }
    }

    // Read hhea table for ascent/descent
    var hhea_result = find_table(font, 0x68686561u32) // 'hhea'
    if(hhea_result is Result.Ok) {
        var Ok(hhea) = hhea_result else unreachable
        if((hhea.offset as size_t) + 36 <= font.data.size()) {
            font.ascent = read_i16_be(font.data.data(), (hhea.offset as size_t) + 4)
            font.descent = read_i16_be(font.data.data(), (hhea.offset as size_t) + 6)
        }
    }

    // Read maxp table for num_glyphs
    var maxp_result = find_table(font, 0x6D617870u32) // 'maxp'
    if(maxp_result is Result.Ok) {
        var Ok(maxp) = maxp_result else unreachable
        if((maxp.offset as size_t) + 32 <= font.data.size()) {
            font.num_glyphs = read_u16_be(font.data.data(), (maxp.offset as size_t) + 4)
        }
    }

    return std.Result.Ok(std::Unit{})
}

func find_table(font : *mut Font, tag : u32) : std::Result<FontTable, FontError> {
    var i : size_t = 0
    while(i < font.tables.size()) {
        var t = font.tables.get_ptr(i)
        if(t.tag == tag) {
            return std.Result.Ok(font.tables.get(i))
        }
        i += 1
    }
    return std.Result.Err(FontError.InvalidFormat(string("required table not found")))
}

public func font_units_per_em(font : *mut Font) : u16 {
    return font.units_per_em
}

public func font_ascent(font : *mut Font) : i16 {
    return font.ascent
}

public func font_descent(font : *mut Font) : i16 {
    return font.descent
}

public func font_num_glyphs(font : *mut Font) : u16 {
    return font.num_glyphs
}

public func font_line_height(font : *mut Font) : i32 {
    return (font.ascent as i32) - (font.descent as i32)
}

// ---------------------------------------------------------------------------
// cmap: character to glyph index mapping
// ---------------------------------------------------------------------------

func cmap_lookup(font : *mut Font, codepoint : u32) : u16 {
    var cmap_result = find_table(font, 0x636D6170u32) // 'cmap'
    if(cmap_result is Result.Err) { return 0 }
    var Ok(cmap) = cmap_result else unreachable
    var base = cmap.offset as size_t

    if(base + 4 > font.data.size()) { return 0 }
    var version = read_u16_be(font.data.data(), base)
    var num_subtables = read_u16_be(font.data.data(), base + 2)

    // Find the best subtable (platform 3 encoding 1 = Windows Unicode BMP, or platform 0)
    var best_platform : u16 = 0
    var best_encoding : u16 = 0
    var best_offset : u32 = 0

    var i : u16 = 0
    while(i < num_subtables) {
        var sub_offset = base + 4 + (i as size_t) * 8
        if(sub_offset + 8 > font.data.size()) { break }

        var platform = read_u16_be(font.data.data(), sub_offset)
        var encoding = read_u16_be(font.data.data(), sub_offset + 2)
        var subtable_offset = read_u32_be(font.data.data(), sub_offset + 4)

        if(platform == 3 && encoding == 1) {
            best_platform = platform
            best_encoding = encoding
            best_offset = subtable_offset
            break
        }
        if(platform == 0 && best_platform == 0) {
            best_platform = platform
            best_encoding = encoding
            best_offset = subtable_offset
        }
        i += 1
    }

    if(best_offset == 0) { return 0 }

    var st_base = base + (best_offset as size_t)
    if(st_base + 2 > font.data.size()) { return 0 }
    var format = read_u16_be(font.data.data(), st_base)

    if(format == 0) {
        // Byte encoding table
        if(st_base + 256 + 2 > font.data.size()) { return 0 }
        if(codepoint < 256) {
            return read_u16_be(font.data.data(), st_base + 6 + codepoint as size_t)
        }
        return 0
    } else if(format == 4) {
        // Segment mapping to delta values
        return cmap_format4_lookup(font, st_base, codepoint)
    } else if(format == 6) {
        // Trimmed table mapping
        if(st_base + 6 > font.data.size()) { return 0 }
        var length = read_u16_be(font.data.data(), st_base + 2)
        var first_code = read_u16_be(font.data.data(), st_base + 4)
        var entry_count = read_u16_be(font.data.data(), st_base + 6)
        if(codepoint < first_code || codepoint >= first_code + entry_count) { return 0 }
        return read_u16_be(font.data.data(), st_base + 8 + ((codepoint - first_code as u32) as size_t) * 2)
    }

    return 0
}

func cmap_format4_lookup(font : *mut Font, base : size_t, codepoint : u32) : u16 {
    if(base + 8 > font.data.size()) { return 0 }
    var length = read_u16_be(font.data.data(), base + 2)
    var seg_count = read_u16_be(font.data.data(), base + 6) / 2

    var search_offset = base + 14
    var i : u16 = 0
    while(i < seg_count) {
        var end_offset = search_offset + (i as size_t) * 4
        var start_offset = search_offset + 2 + (i as size_t) * 4
        var id_delta_offset = search_offset + seg_count as size_t * 4 + (i as size_t) * 2
        var id_range_offset = search_offset + seg_count as size_t * 6 + (i as size_t) * 2

        if(id_range_offset + 2 > font.data.size()) { break }

        var end_code = read_u16_be(font.data.data(), end_offset)
        var start_code = read_u16_be(font.data.data(), start_offset)
        var id_delta = read_i16_be(font.data.data(), id_delta_offset)
        var id_range_off = read_u16_be(font.data.data(), id_range_offset)

        if(codepoint >= start_code as u32 && codepoint <= end_code as u32) {
            if(id_range_off == 0) {
                return ((codepoint as i32) + (id_delta as i32)) as u16
            } else {
                var glyph_offset = id_range_offset as size_t + (id_range_off as size_t) + ((codepoint - start_code as u32) as size_t) * 2
                if(glyph_offset + 2 > font.data.size()) { return 0 }
                var glyph_id = read_u16_be(font.data.data(), glyph_offset)
                if(glyph_id != 0) {
                    return ((glyph_id as i32 + id_delta) & 0xFFFF) as u16
                }
                return 0
            }
        }
        i += 1
    }
    return 0
}

// ---------------------------------------------------------------------------
// Glyph metrics from 'hmtx' table
// ---------------------------------------------------------------------------

public func font_get_glyph_metrics(font : *mut Font, glyph_id : u16) : std::Result<GlyphMetrics, FontError> {
    var hmtx_result = find_table(font, 0x686D7478u32) // 'hmtx'
    if(hmtx_result is Result.Err) {
        return std.Result.Err(FontError.InvalidFormat(string("hmtx table not found")))
    }
    var Ok(hmtx) = hmtx_result else unreachable
    var base = hmtx.offset as size_t

    var num_lsb : u16 = 0
    var hhea_result = find_table(font, 0x68686561u32)
    if(hhea_result is Result.Ok) {
        var Ok(hhea) = hhea_result else unreachable
        if((hhea.offset as size_t) + 34 <= font.data.size()) {
            num_lsb = read_u16_be(font.data.data(), (hhea.offset as size_t) + 34)
        }
    }

    var metrics = GlyphMetrics{advance_width: 0, left_side_bearing: 0, xmin: 0, ymin: 0, xmax: 0, ymax: 0, width: 0, height: 0}

    if((glyph_id as u32) < num_lsb as u32) {
        // Long metrics entry
        var entry_offset = base + (glyph_id as size_t) * 4
        if(entry_offset + 4 <= font.data.size()) {
            metrics.advance_width = read_u16_be(font.data.data(), entry_offset) as i32
            metrics.left_side_bearing = read_i16_be(font.data.data(), entry_offset + 2)
        }
    } else {
        // Use last long metric entry
        var last_offset = base + ((num_lsb as size_t) - 1) * 4
        if(last_offset + 4 <= font.data.size()) {
            metrics.advance_width = read_u16_be(font.data.data(), last_offset) as i32
        }
        // Short metrics entry (lsb only)
        var short_offset = base + (num_lsb as size_t) * 4 + ((glyph_id as size_t) - (num_lsb as size_t)) * 2
        if(short_offset + 2 <= font.data.size()) {
            metrics.left_side_bearing = read_i16_be(font.data.data(), short_offset)
        }
    }

    return std.Result.Ok(metrics)
}

// ---------------------------------------------------------------------------
// Glyph outline from 'glyf' table
// ---------------------------------------------------------------------------

public func font_get_glyph_outline(font : *mut Font, glyph_id : u16) : std::Result<GlyphOutline, FontError> {
    var glyf_result = find_table(font, 0x676C7966u32) // 'glyf'
    if(glyf_result is Result.Err) {
        return std.Result.Err(FontError.UnsupportedFeature(string("glyf table not found")))
    }
    var Ok(glyf) = glyf_result else unreachable
    var base = glyf.offset as size_t

    // Get location offset from 'loca' table
    var loca_result = find_table(font, 0x6C6F6361u32) // 'loca'
    if(loca_result is Result.Err) {
        return std.Result.Err(FontError.InvalidFormat(string("loca table not found")))
    }
    var Ok(loca) = loca_result else unreachable
    var loca_base = loca.offset as size_t

    // Determine if short or long format from head table
    var is_short = true
    var head_result = find_table(font, 0x68656164u32)
    if(head_result is Result.Ok) {
        var Ok(head) = head_result else unreachable
        if((head.offset as size_t) + 50 <= font.data.size()) {
            var index_to_loc_format = read_i32_be(font.data.data(), (head.offset as size_t) + 50)
            is_short = (index_to_loc_format == 0)
        }
    }

    var glyph_offset : u32 = 0
    if(is_short) {
        var loc = loca_base + (glyph_id as size_t) * 2
        if(loc + 4 <= font.data.size()) {
            glyph_offset = (read_u16_be(font.data.data(), loc) as u32) * 2
        }
    } else {
        var loc = loca_base + (glyph_id as size_t) * 4
        if(loc + 4 <= font.data.size()) {
            glyph_offset = read_u32_be(font.data.data(), loc)
        }
    }

    var glyf_offset = base + (glyph_offset as size_t)
    if(glyf_offset + 10 > font.data.size()) {
        return std.Result.Err(FontError.GlyphNotFound(glyph_id as u32))
    }

    var num_contours = read_i16_be(font.data.data(), glyf_offset)
    var xmin = read_i16_be(font.data.data(), glyf_offset + 2)
    var ymin = read_i16_be(font.data.data(), glyf_offset + 4)
    var xmax = read_i16_be(font.data.data(), glyf_offset + 6)
    var ymax = read_i16_be(font.data.data(), glyf_offset + 8)

    var outline = GlyphOutline{}

    if(num_contours == 0) {
        // Empty glyph
        return std.Result.Ok(outline)
    } else if(num_contours > 0) {
        // Simple glyph
        var contour_end_offset = glyf_offset + 10 + (num_contours as size_t) * 2
        var instructions_offset = contour_end_offset + 2
        var num_instructions = read_u16_be(font.data.data(), contour_end_offset)

        var points_offset = instructions_offset + (num_instructions as size_t)
        var num_points : u16 = 0
        var i : i16 = 0
        while(i < num_contours) {
            var end = read_u16_be(font.data.data(), glyf_offset + 10 + (i as size_t) * 2)
            if(end >= num_points) { num_points = end + 1 }
            i += 1
        }

        // Parse flags and coordinates
        var pos = points_offset
        var j : u16 = 0
        while(j < num_points && pos < font.data.size()) {
            var flags = font.data.data()[pos]
            pos += 1
            outline.flags.push(flags)

            // X coordinate
            if(flags & 0x02 != 0) {
                // Short vector, same or different
                if(pos < font.data.size()) {
                    var dx = font.data.data()[pos] as i32
                    if(flags & 0x10 == 0) { dx = -dx }
                    var prev_x : i32 = 0
                    if(outline.points.size() > 0) { prev_x = outline.points.get(outline.points.size() - 2) }
                    outline.points.push(prev_x + dx)
                    pos += 1
                }
            } else {
                // Long vector
                if(flags & 0x10 != 0) {
                    // Same as previous
                    var prev_x : i32 = 0
                    if(outline.points.size() > 0) { prev_x = outline.points.get(outline.points.size() - 2) }
                    outline.points.push(prev_x)
                } else {
                    if(pos + 2 <= font.data.size()) {
                        var dx = read_i16_be(font.data.data(), pos) as i32
                        var prev_x : i32 = 0
                        if(outline.points.size() > 0) { prev_x = outline.points.get(outline.points.size() - 2) }
                        outline.points.push(prev_x + dx)
                        pos += 2
                    }
                }
            }

            // Y coordinate
            if(flags & 0x04 != 0) {
                if(pos < font.data.size()) {
                    var dy = font.data.data()[pos] as i32
                    if(flags & 0x20 == 0) { dy = -dy }
                    var prev_y : i32 = 0
                    if(outline.points.size() > 0) { prev_y = outline.points.get(outline.points.size() - 1) }
                    outline.points.push(prev_y + dy)
                    pos += 1
                }
            } else {
                if(flags & 0x20 != 0) {
                    var prev_y : i32 = 0
                    if(outline.points.size() > 0) { prev_y = outline.points.get(outline.points.size() - 1) }
                    outline.points.push(prev_y)
                } else {
                    if(pos + 2 <= font.data.size()) {
                        var dy = read_i16_be(font.data.data(), pos) as i32
                        var prev_y : i32 = 0
                        if(outline.points.size() > 0) { prev_y = outline.points.get(outline.points.size() - 1) }
                        outline.points.push(prev_y + dy)
                        pos += 2
                    }
                }
            }

            j += 1
        }

        return std.Result.Ok(outline)
    }

    // Composite glyphs not fully supported
    return std.Result.Err(FontError.UnsupportedFeature(string("composite glyphs not yet supported")))
}

public func font_get_glyph(font : *mut Font, codepoint : u32) : std::Result<u16, FontError> {
    var glyph_id = cmap_lookup(font, codepoint)
    if(glyph_id == 0 && codepoint != 0) {
        return std.Result.Err(FontError.GlyphNotFound(codepoint))
    }
    return std.Result.Ok(glyph_id)
}

// ---------------------------------------------------------------------------
// Text measurement (simple, no shaping)
// ---------------------------------------------------------------------------

public struct TextMetrics {
    var width : i32
    var height : i32
    var ascent : i32
    var descent : i32
}

public func font_measure_text(font : *mut Font, text : *char, text_len : size_t, font_size : float) : TextMetrics {
    var scale = font_size / (font.units_per_em as float)
    var total_width : i32 = 0
    var max_height : i32 = 0

    var i : size_t = 0
    while(i < text_len) {
        var cp = text[i] as u32
        var glyph_result = font_get_glyph(font, cp)
        if(glyph_result is Result.Ok) {
            var Ok(glyph_id) = glyph_result else unreachable
            var metrics_result = font_get_glyph_metrics(font, glyph_id)
            if(metrics_result is Result.Ok) {
                var Ok(metrics) = metrics_result else unreachable
                total_width += (metrics.advance_width as float * scale) as i32
                var glyph_height = (metrics.ymax - metrics.ymin)
                if(glyph_height > max_height) { max_height = glyph_height }
            }
        }
        i += 1
    }

    return TextMetrics {
        width : total_width,
        height : (max_height as float * scale) as i32,
        ascent : (font.ascent as float * scale) as i32,
        descent : (-font.descent as float * scale) as i32
    }
}

} // end namespace font
