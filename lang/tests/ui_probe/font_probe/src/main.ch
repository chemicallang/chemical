// P0.1 font probe: what the in-tree `font` parser actually supports on the
// reference Linux host. Paths are the host's system fonts; the evidence file
// records the exact files and sizes.
using std::Result;

func probe_atomic_write() {
    var bytes : [4]u8
    bytes[0] = 0x11u8
    bytes[1] = 0x22u8
    bytes[2] = 0x33u8
    bytes[3] = 0x44u8
    var target = "/tmp/ui_probe_atomic.bin"
    var wrote = unsafe(fs::raw::atomic_write(target as *char, &raw mut bytes[0], 4u))
    if(wrote is Result.Err) {
        println("P0.1 font_atomic_write FAIL write")
        return
    }
    var p = path::PathBuf.from("/tmp/ui_probe_atomic.bin")
    var read = fs::read(&p.as_path())
    if(read is Result.Err) {
        println("P0.1 font_atomic_write FAIL read")
        return
    }
    var Ok(data) = read else unsafe { unreachable }
    var ok : bool = false
    unsafe {
        ok = data.size() == 4u && data.data()[0] == 0x11u8 && data.data()[3] == 0x44u8
    }
    if(ok) {
        println("P0.1 font_atomic_write PASS no-partial")
    } else {
        println("P0.1 font_atomic_write FAIL content")
    }
}


func probe_glyph(f : &font::Font, cp : u32, label : *char) {
    var g = f.glyph(cp)
    if(g is Result.Err) {
        unsafe { printf("  glyph %s U+%04X FAIL\n", label, cp) }
        return
    }
    var Ok(gid) = g else unsafe { unreachable }
    unsafe { printf("  glyph %s U+%04X id=%u\n", label, cp, gid as u32) }
}

func probe_outline(f : &font::Font, cp : u32, label : *char) {
    var g = f.glyph(cp)
    if(g is Result.Err) {
        unsafe { printf("  outline %s U+%04X FAIL(no glyph)\n", label, cp) }
        return
    }
    var Ok(gid) = g else unsafe { unreachable }
    var o = f.glyph_outline(gid)
    if(o is Result.Err) {
        unsafe { printf("  outline %s U+%04X FAIL(unsupported)\n", label, cp) }
        return
    }
    var Ok(outline) = o else unsafe { unreachable }
    unsafe {
        printf("  outline %s U+%04X points=%lu flags=%lu\n", label, cp,
               outline.points.size() as u64, outline.flags.size() as u64)
    }
}

func probe_font(path_str : &str, label : *char) {
    var p = path::PathBuf.from(path_str)
    var loaded = font::Font.from_path(&p.as_path())
    if(loaded is Result.Err) {
        unsafe { printf("font %s LOAD_FAIL\n", label) }
        return
    }
    var Ok(f) = loaded else unsafe { unreachable }
    unsafe {
        printf("font %s size=%lu tables=%lu glyphs=%u em=%u asc=%d desc=%d\n",
               label, f.data_size() as u64, f.table_count() as u64,
               f.glyph_count() as u32, f.em_size() as u32,
               f.ascender() as i32, f.descender() as i32)
    }
    probe_glyph(&f, 0x41u32, "A")
    probe_glyph(&f, 0xE9u32, "eacute")
    probe_glyph(&f, 0x4E2Du32, "CJK")
    probe_glyph(&f, 0x1F600u32, "emoji")
    probe_outline(&f, 0x41u32, "A")
    probe_outline(&f, 0xE9u32, "eacute")
    probe_outline(&f, 0x4E2Du32, "CJK")
}

public func main() : int {
    probe_atomic_write()
    probe_font("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "DejaVuSans.ttf")
    probe_font("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf", "NotoSans.ttf")
    probe_font("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc", "NotoSansCJK.ttc")
    probe_font("/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf", "NotoColorEmoji.ttf")
    probe_font("/usr/share/fonts/opentype/urw-base35/NimbusSans-Regular.otf", "NimbusSans.otf")
    println("P0.1 font_probe DONE")
    return 0
}
