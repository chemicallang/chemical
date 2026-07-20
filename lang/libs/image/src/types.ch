public namespace image {

using std::string;
using std::vector;

public variant ImageError {
    FileNotFound()
    InvalidFormat(msg : string)
    UnsupportedFormat(msg : string)
    IoError(msg : string)
    InvalidDimensions(w : int, h : int)
    PixelOutOfBounds(x : int, y : int)

    func message(&self) : string {
        switch(self) {
            FileNotFound() => return string("ImageError: file not found")
            InvalidFormat(msg) => {
                var s = string("ImageError: ")
                s.append_string(&msg)
                return s
            }
            UnsupportedFormat(msg) => {
                var s = string("ImageError: unsupported format: ")
                s.append_string(&msg)
                return s
            }
            IoError(msg) => {
                var s = string("ImageError: IO error: ")
                s.append_string(&msg)
                return s
            }
            InvalidDimensions(w, h) => {
                var s = string("ImageError: invalid dimensions ")
                s.append_integer(w)
                s.append('x')
                s.append_integer(h)
                return s
            }
            PixelOutOfBounds(x, y) => {
                var s = string("ImageError: pixel out of bounds (")
                s.append_integer(x)
                s.append(',')
                s.append_integer(y)
                s.append(')')
                return s
            }
        }
    }
}

public struct RGBA8 {
    var r : u8
    var g : u8
    var b : u8
    var a : u8

    @make
    func make(r_ : u8, g_ : u8, b_ : u8, a_ : u8 = 255) : RGBA8 {
        return RGBA8 { r : r_, g : g_, b : b_, a : a_ }
    }
}

public struct RGB8 {
    var r : u8
    var g : u8
    var b : u8
}

public struct Gray8 {
    var v : u8
}

@direct_init
public struct Image {
    var pixels : vector<u8>
    var width : int
    var height : int
    var channels : int

    @make
    func make() : Image {
        return Image {
            pixels : vector<u8>(),
            width : 0,
            height : 0,
            channels : 0
        }
    }
}

} // end namespace image
