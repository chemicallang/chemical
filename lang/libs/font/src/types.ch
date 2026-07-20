public namespace font {

using std::string;
using std::vector;

public variant FontError {
    FileNotFound()
    InvalidFormat(msg : string)
    UnsupportedFeature(msg : string)
    IoError(msg : string)
    GlyphNotFound(codepoint : u32)

    func message(&self) : string {
        switch(self) {
            FileNotFound() => return string("FontError: file not found")
            InvalidFormat(msg) => {
                var s = string("FontError: ")
                s.append_string(&msg)
                return s
            }
            UnsupportedFeature(msg) => {
                var s = string("FontError: unsupported: ")
                s.append_string(&msg)
                return s
            }
            IoError(msg) => {
                var s = string("FontError: IO error: ")
                s.append_string(&msg)
                return s
            }
            GlyphNotFound(cp) => return string("FontError: glyph not found")
        }
    }
}

@direct_init
public struct GlyphMetrics {
    var advance_width : i32
    var left_side_bearing : i32
    var xmin : i32
    var ymin : i32
    var xmax : i32
    var ymax : i32
    var width : i32
    var height : i32
}

@direct_init
public struct GlyphOutline {
    var points : vector<i32>
    var flags : vector<u8>
    var instructions : vector<u8>

    @make
    func make() : GlyphOutline {
        return GlyphOutline {
            points : vector<i32>(),
            flags : vector<u8>(),
            instructions : vector<u8>()
        }
    }
}

} // end namespace font
