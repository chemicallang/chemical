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
        unsafe {
        return ((data[offset] as u16) << 8) | (data[offset + 1] as u16)

        }
    }

func read_u32_be(data : *u8, offset : size_t) : u32 {
        unsafe {
        return ((data[offset] as u32) << 24) | ((data[offset + 1] as u32) << 16) |
               ((data[offset + 2] as u32) << 8) | (data[offset + 3] as u32)

        }
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
    // Parsed state is owned by the Font. Callers use the read-only accessors
    // below; exposing the backing vector/table offsets would let safe code
    // forge a Font and drive unchecked reads in the glyph parser.
    internal var data : vector<u8>
    internal var tables : vector<FontTable>
    internal var units_per_em : u16
    internal var ascent : i16
    internal var descent : i16
    internal var num_glyphs : u16
    internal var loaded : bool

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

    // ── safe construction ────────────────────────────────────────────────

    // Parse a TrueType/OpenType font from memory.
    func load(data : &[u8]) : std::Result<Font, FontError> {
        return decode_font(data.data(), data.size())
    }

    // Read and parse a font from a filesystem path.
    func from_path(path : &path::Path) : std::Result<Font, FontError> {
        return load_from_path(path)
    }

    // ── safe metrics ─────────────────────────────────────────────────────

    public func is_loaded(&self) : bool { return self.loaded }
    public func em_size(&self) : u16 { return self.units_per_em }
    public func ascender(&self) : i16 { return self.ascent }
    public func descender(&self) : i16 { return self.descent }
    public func glyph_count(&self) : u16 { return self.num_glyphs }
    public func data_size(&self) : size_t { return self.data.size() }
    public func table_count(&self) : size_t { return self.tables.size() }

    /// Verify that the parsed table directory is internally consistent.
    public func validate(&self) : std::Result<std::Unit, FontError> {
        if(!self.loaded || self.data.size() < 12) {
            return std::Result.Err< std::Unit, FontError >(FontError.InvalidFormat(string("font is not loaded")))
        }
        var i : size_t = 0
        while(i < self.tables.size()) {
            var t = self.tables.at(i)
            var off = t.offset as size_t
            var len = t.length as size_t
            if(off > self.data.size()) {
                return std::Result.Err< std::Unit, FontError >(FontError.InvalidFormat(string("font table offset is out of range")))
            }
            if(len > self.data.size() - off) {
                return std::Result.Err< std::Unit, FontError >(FontError.InvalidFormat(string("font table length is out of range")))
            }
            i += 1
        }
        return std::Result.Ok< std::Unit, FontError >(std::Unit{})
    }

    func metrics(&self, font_size : float) : FontMetrics {
        return metrics_scaled(self, font_size)
    }

    func line_height(&self) : i32 {
        return impl_font_line_height(unsafe(self as *mut Font))
    }

    // Measure a UTF-8 text run at `font_size` pixels.
    func measure_text(&self, text : &str, font_size : float) : TextMetrics {
        return impl_font_measure_text(unsafe(self as *mut Font), text.data(), text.size(), font_size)
    }

    // ── safe glyph lookup ────────────────────────────────────────────────

    func glyph(&self, codepoint : u32) : std::Result<u16, FontError> {
        var check = self.validate()
        if(check is std::Result.Err) { var Err(e) = check else unsafe { unreachable }; return std::Result.Err<u16, FontError>(e) }
        return impl_font_get_glyph(unsafe(self as *mut Font), codepoint)
    }

    func glyph_metrics(&self, glyph_id : u16) : std::Result<GlyphMetrics, FontError> {
        var check = self.validate()
        if(check is std::Result.Err) { var Err(e) = check else unsafe { unreachable }; return std::Result.Err<GlyphMetrics, FontError>(e) }
        return impl_font_get_glyph_metrics(unsafe(self as *mut Font), glyph_id)
    }

    func glyph_outline(&self, glyph_id : u16) : std::Result<GlyphOutline, FontError> {
        var check = self.validate()
        if(check is std::Result.Err) { var Err(e) = check else unsafe { unreachable }; return std::Result.Err<GlyphOutline, FontError>(e) }
        return impl_font_get_glyph_outline(unsafe(self as *mut Font), glyph_id)
    }
}

public struct FontTable {
    var tag : u32
    var offset : u32
    var length : u32
}

impl Copy for FontTable {}

func impl_font_load(path : *char) : std::Result<Font, FontError> {
        unsafe {
        var read_result = fs::raw::read_entire_file(path)
        if(read_result is Result.Err) {
            return std.Result.Err(FontError.FileNotFound())
        }

        var font = Font.make()
        var Ok(data) = read_result else unsafe { unreachable }
        unsafe { memcpy(&raw mut font.data, &raw data, sizeof(vector<u8>)) }
        new(&raw data) vector<u8>()

        var parse_result = parse_font_header(&raw mut font)
        if(parse_result is Result.Err) {
            return std.Result.Err(FontError.InvalidFormat(string("failed to parse font header")))
        }

        font.loaded = true
        return std.Result.Ok(font)

        }
    }

func parse_font_header(font : *mut Font) : std::Result<std::Unit, FontError> {
        unsafe {
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
            var Ok(head) = head_result else unsafe { unreachable }
            if((head.offset as size_t) + 54 <= font.data.size()) {
                font.units_per_em = read_u16_be(font.data.data(), (head.offset as size_t) + 18)
            }
        }

        // Read hhea table for ascent/descent
        var hhea_result = find_table(font, 0x68686561u32) // 'hhea'
        if(hhea_result is Result.Ok) {
            var Ok(hhea) = hhea_result else unsafe { unreachable }
            if((hhea.offset as size_t) + 36 <= font.data.size()) {
                font.ascent = read_i16_be(font.data.data(), (hhea.offset as size_t) + 4)
                font.descent = read_i16_be(font.data.data(), (hhea.offset as size_t) + 6)
            }
        }

        // Read maxp table for num_glyphs
        var maxp_result = find_table(font, 0x6D617870u32) // 'maxp'
        if(maxp_result is Result.Ok) {
            var Ok(maxp) = maxp_result else unsafe { unreachable }
            if((maxp.offset as size_t) + 32 <= font.data.size()) {
                font.num_glyphs = read_u16_be(font.data.data(), (maxp.offset as size_t) + 4)
            }
        }

        return std.Result.Ok(std::Unit{})

        }
    }

func find_table(font : *mut Font, tag : u32) : std::Result<FontTable, FontError> {
        unsafe {
        var i : size_t = 0
        while(i < font.tables.size()) {
            var t = font.tables.get_ptr(i)
            if(t.tag == tag) {
                return std.Result.Ok(font.tables.at(i))
            }
            i += 1
        }
        return std.Result.Err(FontError.InvalidFormat(string("required table not found")))

        }
    }

func impl_font_units_per_em(font : *mut Font) : u16 {
        unsafe {
        return font.units_per_em

        }
    }

func impl_font_ascent(font : *mut Font) : i16 {
        unsafe {
        return font.ascent

        }
    }

func impl_font_descent(font : *mut Font) : i16 {
        unsafe {
        return font.descent

        }
    }

func impl_font_num_glyphs(font : *mut Font) : u16 {
        unsafe {
        return font.num_glyphs

        }
    }

func impl_font_line_height(font : *mut Font) : i32 {
        unsafe {
        return (font.ascent as i32) - (font.descent as i32)

        }
    }

// ---------------------------------------------------------------------------
// cmap: character to glyph index mapping
// ---------------------------------------------------------------------------

func cmap_lookup(font : *mut Font, codepoint : u32) : u16 {
        unsafe {
        var cmap_result = find_table(font, 0x636D6170u32) // 'cmap'
        if(cmap_result is Result.Err) { return 0 }
        var Ok(cmap) = cmap_result else unsafe { unreachable }
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
            var first_code = read_u16_be(font.data.data(), st_base + 6)
            var entry_count = read_u16_be(font.data.data(), st_base + 8)
            if(codepoint < first_code || codepoint >= first_code + entry_count) { return 0 }
            return read_u16_be(font.data.data(), st_base + 10 + ((codepoint - first_code as u32) as size_t) * 2)
        }

        return 0

        }
    }

func cmap_format4_lookup(font : *mut Font, base : size_t, codepoint : u32) : u16 {
        unsafe {
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
    }

// ---------------------------------------------------------------------------
// Glyph metrics from 'hmtx' table
// ---------------------------------------------------------------------------

func impl_font_get_glyph_metrics(font : *mut Font, glyph_id : u16) : std::Result<GlyphMetrics, FontError> {
        unsafe {
        var hmtx_result = find_table(font, 0x686D7478u32) // 'hmtx'
        if(hmtx_result is Result.Err) {
            return std.Result.Err(FontError.InvalidFormat(string("hmtx table not found")))
        }
        var Ok(hmtx) = hmtx_result else unsafe { unreachable }
        var base = hmtx.offset as size_t

        var num_lsb : u16 = 0
        var hhea_result = find_table(font, 0x68686561u32)
        if(hhea_result is Result.Ok) {
            var Ok(hhea) = hhea_result else unsafe { unreachable }
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
        } else if(num_lsb > 0) {
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
    }

// ---------------------------------------------------------------------------
// Glyph outline from 'glyf' table
// ---------------------------------------------------------------------------

func impl_font_get_glyph_outline(font : *mut Font, glyph_id : u16) : std::Result<GlyphOutline, FontError> {
        unsafe {
        var glyf_result = find_table(font, 0x676C7966u32) // 'glyf'
        if(glyf_result is Result.Err) {
            return std.Result.Err(FontError.UnsupportedFeature(string("glyf table not found")))
        }
        var Ok(glyf) = glyf_result else unsafe { unreachable }
        var base = glyf.offset as size_t

        // Get location offset from 'loca' table
        var loca_result = find_table(font, 0x6C6F6361u32) // 'loca'
        if(loca_result is Result.Err) {
            return std.Result.Err(FontError.InvalidFormat(string("loca table not found")))
        }
        var Ok(loca) = loca_result else unsafe { unreachable }
        var loca_base = loca.offset as size_t

        // Determine if short or long format from head table
        var is_short = true
        var head_result = find_table(font, 0x68656164u32)
        if(head_result is Result.Ok) {
            var Ok(head) = head_result else unsafe { unreachable }
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
            // Simple glyph. Every offset below is derived from the file; reject
            // a malformed contour/instruction count before reading it.
            var remaining = font.data.size() - (glyf_offset + 10)
            if((num_contours as size_t) > remaining / 2) {
                return std.Result.Err<GlyphOutline, FontError>(FontError.InvalidFormat(string("glyph contour table is out of range")))
            }
            var contour_end_offset = glyf_offset + 10 + (num_contours as size_t) * 2
            if(contour_end_offset + 2 > font.data.size()) {
                return std.Result.Err<GlyphOutline, FontError>(FontError.InvalidFormat(string("glyph instructions offset is out of range")))
            }
            var instructions_offset = contour_end_offset + 2
            var num_instructions = read_u16_be(font.data.data(), contour_end_offset)

            var points_offset = instructions_offset + (num_instructions as size_t)
            if(points_offset > font.data.size()) {
                return std.Result.Err<GlyphOutline, FontError>(FontError.InvalidFormat(string("glyph instructions are out of range")))
            }
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
                if((flags & 0x02) != 0) {
                    // Short vector, same or different
                    if(pos < font.data.size()) {
                        var dx = font.data.data()[pos] as i32
                        if((flags & 0x10) == 0) { dx = -dx }
                        var prev_x : i32 = 0
                        if(outline.points.size() > 0) { prev_x = outline.points.at(outline.points.size() - 2) }
                        outline.points.push(prev_x + dx)
                        pos += 1
                    }
                } else {
                    // Long vector
                    if((flags & 0x10) != 0) {
                        // Same as previous
                        var prev_x : i32 = 0
                        if(outline.points.size() > 0) { prev_x = outline.points.at(outline.points.size() - 2) }
                        outline.points.push(prev_x)
                    } else {
                        if(pos + 2 <= font.data.size()) {
                            var dx = read_i16_be(font.data.data(), pos) as i32
                            var prev_x : i32 = 0
                            if(outline.points.size() > 0) { prev_x = outline.points.at(outline.points.size() - 2) }
                            outline.points.push(prev_x + dx)
                            pos += 2
                        }
                    }
                }

                // Y coordinate
                if((flags & 0x04) != 0) {
                    if(pos < font.data.size()) {
                        var dy = font.data.data()[pos] as i32
                        if((flags & 0x20) == 0) { dy = -dy }
                        var prev_y : i32 = 0
                        if(outline.points.size() > 0) { prev_y = outline.points.at(outline.points.size() - 1) }
                        outline.points.push(prev_y + dy)
                        pos += 1
                    }
                } else {
                    if((flags & 0x20) != 0) {
                        var prev_y : i32 = 0
                        if(outline.points.size() > 0) { prev_y = outline.points.at(outline.points.size() - 1) }
                        outline.points.push(prev_y)
                    } else {
                        if(pos + 2 <= font.data.size()) {
                            var dy = read_i16_be(font.data.data(), pos) as i32
                            var prev_y : i32 = 0
                            if(outline.points.size() > 0) { prev_y = outline.points.at(outline.points.size() - 1) }
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
    }

func impl_font_get_glyph(font : *mut Font, codepoint : u32) : std::Result<u16, FontError> {
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

// Number of bytes in the UTF-8 sequence starting at `pos` (1..4). Invalid or
// truncated leads advance by one byte so measurement always makes progress.
func font_utf8_advance(data : *char, len : size_t, pos : size_t) : size_t {
        unsafe {
        var b0 = data[pos] as u8
        if(b0 < 0x80u8) { return 1 }
        if(b0 >= 0xC2u8 && b0 <= 0xDFu8) {
            if(pos + 1 < len) { return 2 }
            return 1
        }
        if(b0 >= 0xE0u8 && b0 <= 0xEFu8) {
            if(pos + 2 < len) { return 3 }
            return 1
        }
        if(b0 >= 0xF0u8 && b0 <= 0xF4u8) {
            if(pos + 3 < len) { return 4 }
            return 1
        }
        return 1

        }
    }

// Decode one UTF-8 code point at `pos`. Malformed, overlong, surrogate, and
// out-of-range sequences decode to U+FFFD while `font_utf8_advance` still
// consumes at least one byte.
func font_utf8_codepoint(data : *char, len : size_t, pos : size_t) : u32 {
        unsafe {
        var b0 = (data[pos] as u8) as u32
        if(b0 < 0x80u32) { return b0 }
        if(b0 < 0xC2u32 || b0 > 0xF4u32) { return 0xFFFDu32 }

        var need : size_t = 0
        var cp : u32 = 0
        if(b0 <= 0xDFu32) {
            need = 1
            cp = b0 & 0x1Fu32
        } else if(b0 <= 0xEFu32) {
            need = 2
            cp = b0 & 0x0Fu32
        } else {
            need = 3
            cp = b0 & 0x07u32
        }
        if(pos + need >= len) { return 0xFFFDu32 }

        var i : size_t = 1
        while(i <= need) {
            var b = (data[pos + i] as u8) as u32
            if(b < 0x80u32 || b > 0xBFu32) { return 0xFFFDu32 }
            cp = (cp << 6) | (b & 0x3Fu32)
            i += 1
        }

        // Reject overlong forms, UTF-16 surrogates, and values beyond U+10FFFF.
        if(need == 2 && cp < 0x800u32) { return 0xFFFDu32 }
        if(need == 3 && cp < 0x10000u32) { return 0xFFFDu32 }
        if(cp >= 0xD800u32 && cp <= 0xDFFFu32) { return 0xFFFDu32 }
        if(cp > 0x10FFFFu32) { return 0xFFFDu32 }
        return cp

        }
    }

func impl_font_measure_text(font : *mut Font, text : *char, text_len : size_t, font_size : float) : TextMetrics {
        unsafe {
        var scale = font_size / (font.units_per_em as float)
        var total_width : i32 = 0
        var max_height : i32 = 0

        var i : size_t = 0
        while(i < text_len) {
            // Decode a full UTF-8 code point; advancing by bytes here would
            // measure each continuation byte as a separate glyph.
            var cp = font_utf8_codepoint(text, text_len, i)
            var step = font_utf8_advance(text, text_len, i)
            var glyph_result = impl_font_get_glyph(font, cp)
            if(glyph_result is Result.Ok) {
                var Ok(glyph_id) = glyph_result else unsafe { unreachable }
                var metrics_result = impl_font_get_glyph_metrics(font, glyph_id)
                if(metrics_result is Result.Ok) {
                    var Ok(metrics) = metrics_result else unsafe { unreachable }
                    total_width += (metrics.advance_width as float * scale) as i32
                    var glyph_height = (metrics.ymax - metrics.ymin)
                    if(glyph_height > max_height) { max_height = glyph_height }
                }
            }
            i += step
        }

        return TextMetrics {
            width : total_width,
            height : (max_height as float * scale) as i32,
            ascent : (font.ascent as float * scale) as i32,
            descent : (-font.descent as float * scale) as i32
        }

        }
    }

// Raw byte/pointer entry points. Safe code uses `Font.from_path`/`Font.load`
// and the `Font` accessors; these take C strings and `*mut Font` handles.
public namespace raw {
public func font_load(path : *char) : std::Result<Font, FontError> {
    return impl_font_load(path)
}

public func font_units_per_em(font : *mut Font) : u16 {
    return impl_font_units_per_em(font)
}

public func font_ascent(font : *mut Font) : i16 {
    return impl_font_ascent(font)
}

public func font_descent(font : *mut Font) : i16 {
    return impl_font_descent(font)
}

public func font_num_glyphs(font : *mut Font) : u16 {
    return impl_font_num_glyphs(font)
}

public func font_line_height(font : *mut Font) : i32 {
    return impl_font_line_height(font)
}

public func font_get_glyph_metrics(font : *mut Font, glyph_id : u16) : std::Result<GlyphMetrics, FontError> {
    return impl_font_get_glyph_metrics(font, glyph_id)
}

public func font_get_glyph_outline(font : *mut Font, glyph_id : u16) : std::Result<GlyphOutline, FontError> {
    return impl_font_get_glyph_outline(font, glyph_id)
}

public func font_get_glyph(font : *mut Font, codepoint : u32) : std::Result<u16, FontError> {
    return impl_font_get_glyph(font, codepoint)
}

public func font_measure_text(font : *mut Font, text : *char, text_len : size_t, font_size : float) : TextMetrics {
    return impl_font_measure_text(font, text, text_len, font_size)
}
} // end namespace raw

} // end namespace font
