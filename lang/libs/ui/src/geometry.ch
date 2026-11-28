// P1.1 geometry, transforms, and sRGB-u8 color values. No renderer/layout.
// Coordinates are logical points; pixel convention is half-open [x,x+w) × [y,y+h)
// as in CONTRACTS.md §6. Floats are the API's 32-bit type; luminance uses f64.

using std::Option;

union F32Bits {
    var f : float
    var u : u32
}

func f32_bits(x : float) : u32 { unsafe {
    var v : F32Bits
    v.f = x
    return v.u
} }

func f32_from_bits(b : u32) : float { unsafe {
    var v : F32Bits
    v.u = b
    return v.f
} }

func finite32(x : float) : bool {
    return (f32_bits(x) & 0x7F800000u32) != 0x7F800000u32
}

func finite_or_zero(x : float) : float {
    if(!finite32(x)) { return 0.0f }
    return x
}

func nonneg(x : float) : float {
    if(!finite32(x) || x < 0.0f) { return 0.0f }
    return x
}

func min32(a : float, b : float) : float {
    if(a < b) { return a }
    return b
}

func max32(a : float, b : float) : float {
    if(a > b) { return a }
    return b
}

func sqrt32(x : float) : float {
    if(!finite32(x) || x <= 0.0f) { return 0.0f }
    var g = x
    if(g < 1.0f) { g = 1.0f }
    var i = 0
    while(i < 24) {
        var next = 0.5f * (g + x / g)
        if(finite32(next) && absf32(next - g) < 0.000001f) { return next }
        g = next
        i = i + 1
    }
    if(!finite32(g)) { return 0.0f }
    return g
}

func absf32(x : float) : float {
    if(x < 0.0f) { return 0.0f - x }
    return x
}

// Range-reduced Taylor approximations avoid a libm link in the TCC path.
const TWO_PI : float = 6.283185307179586f
const PI_F : float = 3.141592653589793f

func sin32(x : float) : float {
    if(!finite32(x)) { return 0.0f }
    var k = (x / TWO_PI) as int
    var y = x - (k as float) * TWO_PI
    if(y > PI_F) { y = y - TWO_PI }
    if(y < 0.0f - PI_F) { y = y + TWO_PI }
    var y2 = y * y
    return y * (1.0f - y2 / 6.0f + y2 * y2 / 120.0f - y2 * y2 * y2 / 5040.0f + y2 * y2 * y2 * y2 / 362880.0f)
}

func cos32(x : float) : float {
    if(!finite32(x)) { return 1.0f }
    var k = (x / TWO_PI) as int
    var y = x - (k as float) * TWO_PI
    if(y > PI_F) { y = y - TWO_PI }
    if(y < 0.0f - PI_F) { y = y + TWO_PI }
    var y2 = y * y
    return 1.0f - y2 / 2.0f + y2 * y2 / 24.0f - y2 * y2 * y2 / 720.0f + y2 * y2 * y2 * y2 / 40320.0f
}

@direct_init
public struct Point {
    var x : float
    var y : float

    public func zero() : Point { return Point { x : 0.0f, y : 0.0f } }
    public func new(x_ : float, y_ : float) : Point {
        return Point { x : finite_or_zero(x_), y : finite_or_zero(y_) }
    }
    public func add(&self, other : Point) : Point {
        return Point.new(self.x + other.x, self.y + other.y)
    }
    public func sub(&self, other : Point) : Point {
        return Point.new(self.x - other.x, self.y - other.y)
    }
    public func scale(&self, s : float) : Point {
        return Point.new(self.x * s, self.y * s)
    }
    public func length(&self) : float {
        return sqrt32(self.x * self.x + self.y * self.y)
    }
    public func distance(&self, other : Point) : float {
        var dx = self.x - other.x
        var dy = self.y - other.y
        return sqrt32(dx * dx + dy * dy)
    }
    public func lerp(&self, other : Point, t : float) : Point {
        var tt = t
        if(!finite32(tt)) { tt = 0.0f }
        if(tt < 0.0f) { tt = 0.0f }
        if(tt > 1.0f) { tt = 1.0f }
        return Point.new(self.x + (other.x - self.x) * tt, self.y + (other.y - self.y) * tt)
    }
    public func is_finite(&self) : bool { return finite32(self.x) && finite32(self.y) }
}

@direct_init
public struct Size {
    var width : float
    var height : float

    public func zero() : Size { return Size { width : 0.0f, height : 0.0f } }
    public func new(width_ : float, height_ : float) : Size {
        return Size { width : nonneg(width_), height : nonneg(height_) }
    }
    public func add(&self, other : Size) : Size {
        return Size.new(self.width + other.width, self.height + other.height)
    }
    public func sub(&self, other : Size) : Size {
        return Size.new(max32(0.0f, self.width - other.width), max32(0.0f, self.height - other.height))
    }
    public func max(&self, other : Size) : Size {
        return Size.new(max32(self.width, other.width), max32(self.height, other.height))
    }
    public func min(&self, other : Size) : Size {
        return Size.new(min32(self.width, other.width), min32(self.height, other.height))
    }
    public func area(&self) : float {
        var a = self.width * self.height
        if(!finite32(a)) { return 0.0f }
        return a
    }
    public func is_finite(&self) : bool { return finite32(self.width) && finite32(self.height) }
}

@direct_init
public struct Rect {
    var x : float
    var y : float
    var w : float
    var h : float

    public func zero() : Rect { return Rect { x : 0.0f, y : 0.0f, w : 0.0f, h : 0.0f } }
    public func new(x_ : float, y_ : float, w_ : float, h_ : float) : Rect {
        return Rect { x : finite_or_zero(x_), y : finite_or_zero(y_), w : nonneg(w_), h : nonneg(h_) }
    }
    public func from_ltrb(left : float, top : float, right : float, bottom : float) : Rect {
        if(!finite32(left) || !finite32(top) || !finite32(right) || !finite32(bottom)) { return Rect.zero() }
        var x0 = min32(left, right)
        var y0 = min32(top, bottom)
        return Rect.new(x0, y0, absf32(right - left), absf32(bottom - top))
    }
    public func x2(&self) : float { return finite_or_zero(self.x + self.w) }
    public func y2(&self) : float { return finite_or_zero(self.y + self.h) }
    public func contains(&self, p : Point) : bool {
        if(!self.is_finite() || !p.is_finite() || self.w <= 0.0f || self.h <= 0.0f) { return false }
        return p.x >= self.x && p.x < self.x2() && p.y >= self.y && p.y < self.y2()
    }
    public func intersects(&self, other : Rect) : bool {
        if(!self.is_finite() || !other.is_finite()) { return false }
        if(self.w <= 0.0f || self.h <= 0.0f || other.w <= 0.0f || other.h <= 0.0f) { return false }
        return self.x < other.x2() && self.x2() > other.x && self.y < other.y2() && self.y2() > other.y
    }
    public func intersection(&self, other : Rect) : Option<Rect> {
        if(!self.intersects(other)) { return Option.None<Rect>() }
        return Option.Some<Rect>(Rect.from_ltrb(max32(self.x, other.x), max32(self.y, other.y),
                                               min32(self.x2(), other.x2()), min32(self.y2(), other.y2())))
    }
    public func union(&self, other : Rect) : Rect {
        if(!self.is_finite()) { return other }
        if(!other.is_finite()) { return Rect.new(self.x, self.y, self.w, self.h) }
        if(self.w <= 0.0f || self.h <= 0.0f) { return other }
        if(other.w <= 0.0f || other.h <= 0.0f) { return Rect.new(self.x, self.y, self.w, self.h) }
        return Rect.from_ltrb(min32(self.x, other.x), min32(self.y, other.y),
                              max32(self.x2(), other.x2()), max32(self.y2(), other.y2()))
    }
    public func inset(&self, i : Insets) : Rect {
        if(!i.is_finite()) { return Rect.zero() }
        return Rect.new(self.x + i.left, self.y + i.top,
                        max32(0.0f, self.w - i.left - i.right),
                        max32(0.0f, self.h - i.top - i.bottom))
    }
    public func offset(&self, dx : float, dy : float) : Rect {
        return Rect.new(self.x + finite_or_zero(dx), self.y + finite_or_zero(dy), self.w, self.h)
    }
    public func area(&self) : float {
        if(!self.is_finite() || self.w <= 0.0f || self.h <= 0.0f) { return 0.0f }
        var a = self.w * self.h
        if(!finite32(a)) { return 0.0f }
        return a
    }
    public func is_finite(&self) : bool {
        return finite32(self.x) && finite32(self.y) && finite32(self.w) && finite32(self.h)
    }
}

@direct_init
public struct Insets {
    var left : float
    var top : float
    var right : float
    var bottom : float

    public func zero() : Insets { return Insets { left : 0.0f, top : 0.0f, right : 0.0f, bottom : 0.0f } }
    public func all(v : float) : Insets {
        var n = finite_or_zero(v)
        return Insets { left : n, top : n, right : n, bottom : n }
    }
    public func symmetric(horizontal : float, vertical : float) : Insets {
        var h = finite_or_zero(horizontal)
        var v = finite_or_zero(vertical)
        return Insets { left : h, top : v, right : h, bottom : v }
    }
    public func horizontal(v : float) : Insets {
        var h = finite_or_zero(v)
        return Insets { left : h, top : 0.0f, right : h, bottom : 0.0f }
    }
    public func vertical(v : float) : Insets {
        var n = finite_or_zero(v)
        return Insets { left : 0.0f, top : n, right : 0.0f, bottom : n }
    }
    public func is_finite(&self) : bool {
        return finite32(self.left) && finite32(self.top) && finite32(self.right) && finite32(self.bottom)
    }
}

@direct_init
public struct Transform2D {
    // Canvas2D order [a, b, c, d, tx, ty]:
    // x' = a*x + c*y + tx, y' = b*x + d*y + ty.
    var m : [6]float

    public func identity() : Transform2D {
        return Transform2D { m : [1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f] }
    }
    public func translate(tx : float, ty : float) : Transform2D {
        return Transform2D { m : [1.0f, 0.0f, 0.0f, 1.0f, finite_or_zero(tx), finite_or_zero(ty)] }
    }
    public func scale(sx : float, sy : float) : Transform2D {
        return Transform2D { m : [finite_or_zero(sx), 0.0f, 0.0f, finite_or_zero(sy), 0.0f, 0.0f] }
    }
    public func rotate(radians : float) : Transform2D {
        if(!finite32(radians)) { return Transform2D.identity() }
        var c = cos32(radians)
        var s = sin32(radians)
        return Transform2D { m : [c, s, 0.0f - s, c, 0.0f, 0.0f] }
    }
    // Returns self ∘ other (apply other first, then self).
    public func multiply(&self, other : Transform2D) : Transform2D {
        var r = Transform2D { m : [
            self.m[0] * other.m[0] + self.m[2] * other.m[1],
            self.m[1] * other.m[0] + self.m[3] * other.m[1],
            self.m[0] * other.m[2] + self.m[2] * other.m[3],
            self.m[1] * other.m[2] + self.m[3] * other.m[3],
            self.m[0] * other.m[4] + self.m[2] * other.m[5] + self.m[4],
            self.m[1] * other.m[4] + self.m[3] * other.m[5] + self.m[5]
        ] }
        if(!r.is_finite()) { return Transform2D.identity() }
        return r
    }
    public func transform_point(&self, p : Point) : Point {
        return Point.new(self.m[0] * p.x + self.m[2] * p.y + self.m[4],
                         self.m[1] * p.x + self.m[3] * p.y + self.m[5])
    }
    public func is_finite(&self) : bool {
        return finite32(self.m[0]) && finite32(self.m[1]) && finite32(self.m[2]) &&
               finite32(self.m[3]) && finite32(self.m[4]) && finite32(self.m[5])
    }
}

func div255(x : u32) : u32 {
    var n = x + 128u32
    return (n + (n >> 8u32)) >> 8u32
}

func clamp_u8(x : u32) : u8 {
    if(x > 255u32) { return 255u8 }
    return x as u8
}

func mix_u8(a : u8, b : u8, t : float) : u8 {
    // Integer interpolation avoids float->u8 edge cases in the LLVM backend.
    var ti = (t * 255.0f) as u32
    if(ti > 255u32) { ti = 255u32 }
    var inv = 255u32 - ti
    var v = (a as u32) * inv + (b as u32) * ti + 127u32
    return (v / 255u32) as u8
}

@direct_init
public struct Color {
    var r : u8
    var g : u8
    var b : u8
    var a : u8

    public func new(r_ : u8, g_ : u8, b_ : u8) : Color {
        return Color { r : r_, g : g_, b : b_, a : 255u8 }
    }
    public func rgba(r_ : u8, g_ : u8, b_ : u8, a_ : u8) : Color {
        return Color { r : r_, g : g_, b : b_, a : a_ }
    }
    // Byte order is 0xRRGGBBAA (red in the high byte).
    public func from_u32(v : u32) : Color {
        return Color { r : ((v >> 24u32) & 0xFFu32) as u8,
                       g : ((v >> 16u32) & 0xFFu32) as u8,
                       b : ((v >> 8u32) & 0xFFu32) as u8,
                       a : (v & 0xFFu32) as u8 }
    }
    public func to_u32(&self) : u32 {
        return ((self.r as u32) << 24u32) | ((self.g as u32) << 16u32) |
               ((self.b as u32) << 8u32) | (self.a as u32)
    }
    public func with_alpha(&self, alpha : u8) : Color {
        return Color { r : self.r, g : self.g, b : self.b, a : alpha }
    }
    public func premultiply(&self) : Color {
        var aa = self.a as u32
        return Color { r : clamp_u8(div255((self.r as u32) * aa)),
                       g : clamp_u8(div255((self.g as u32) * aa)),
                       b : clamp_u8(div255((self.b as u32) * aa)),
                       a : self.a }
    }
    public func unpremultiply(&self) : Color {
        if(self.a == 0u8) { return Color { r : 0u8, g : 0u8, b : 0u8, a : 0u8 } }
        var aa = self.a as u32
        return Color { r : clamp_u8(((self.r as u32) * 255u32 + aa / 2u32) / aa),
                       g : clamp_u8(((self.g as u32) * 255u32 + aa / 2u32) / aa),
                       b : clamp_u8(((self.b as u32) * 255u32 + aa / 2u32) / aa),
                       a : self.a }
    }
    public func lerp(&self, other : Color, t : float) : Color {
        var tt = t
        if(!finite32(tt)) { tt = 0.0f }
        if(tt < 0.0f) { tt = 0.0f }
        if(tt > 1.0f) { tt = 1.0f }
        return Color { r : mix_u8(self.r, other.r, tt), g : mix_u8(self.g, other.g, tt),
                       b : mix_u8(self.b, other.b, tt), a : mix_u8(self.a, other.a, tt) }
    }
    // Integer sRGB-encoded source-over compositing (CONTRACTS.md §6).
    public func over(&self, dst : Color) : Color {
        if(self.a == 0u8) { return Color.rgba(dst.r, dst.g, dst.b, dst.a) }
        if(self.a == 255u8) { return Color.rgba(self.r, self.g, self.b, self.a) }
        var s = self.premultiply()
        var d = dst.premultiply()
        var inv = 255u32 - (s.a as u32)
        var out_a = (s.a as u32) + div255((d.a as u32) * inv)
        var out = Color { r : clamp_u8((s.r as u32) + div255((d.r as u32) * inv)),
                          g : clamp_u8((s.g as u32) + div255((d.g as u32) * inv)),
                          b : clamp_u8((s.b as u32) + div255((d.b as u32) * inv)),
                          a : clamp_u8(out_a) }
        return out.unpremultiply()
    }
    public func relative_luminance(&self) : f64 {
        var table : [256]f64 = [
        0.000000000000,
        0.000303526984,
        0.000607053967,
        0.000910580951,
        0.001214107934,
        0.001517634918,
        0.001821161901,
        0.002124688885,
        0.002428215868,
        0.002731742852,
        0.003035269835,
        0.003346535764,
        0.003676507324,
        0.004024717018,
        0.004391442037,
        0.004776953481,
        0.005181516702,
        0.005605391624,
        0.006048833023,
        0.006512090793,
        0.006995410187,
        0.007499032043,
        0.008023192985,
        0.008568125618,
        0.009134058702,
        0.009721217320,
        0.010329823030,
        0.010960094006,
        0.011612245180,
        0.012286488357,
        0.012983032342,
        0.013702083047,
        0.014443843596,
        0.015208514423,
        0.015996293366,
        0.016807375753,
        0.017641954488,
        0.018500220128,
        0.019382360957,
        0.020288563057,
        0.021219010376,
        0.022173884793,
        0.023153366178,
        0.024157632449,
        0.025186859627,
        0.026241221895,
        0.027320891639,
        0.028426039504,
        0.029556834438,
        0.030713443733,
        0.031896033073,
        0.033104766571,
        0.034339806809,
        0.035601314875,
        0.036889450401,
        0.038204371595,
        0.039546235277,
        0.040915196907,
        0.042311410621,
        0.043735029257,
        0.045186204386,
        0.046665086337,
        0.048171824227,
        0.049706565984,
        0.051269458374,
        0.052860647023,
        0.054480276442,
        0.056128490050,
        0.057805430191,
        0.059511238163,
        0.061246054232,
        0.063010017653,
        0.064803266693,
        0.066625938644,
        0.068478169844,
        0.070360095697,
        0.072271850682,
        0.074213568380,
        0.076185381481,
        0.078187421805,
        0.080219820314,
        0.082282707130,
        0.084376211544,
        0.086500462037,
        0.088655586286,
        0.090841711183,
        0.093058962847,
        0.095307466631,
        0.097587347142,
        0.099898728247,
        0.102241733088,
        0.104616484091,
        0.107023102978,
        0.109461710778,
        0.111932427837,
        0.114435373827,
        0.116970667759,
        0.119538427988,
        0.122138772230,
        0.124771817561,
        0.127437680436,
        0.130136476690,
        0.132868321554,
        0.135633329655,
        0.138431615032,
        0.141263291140,
        0.144128470858,
        0.147027266498,
        0.149959789811,
        0.152926151996,
        0.155926463708,
        0.158960835061,
        0.162029375639,
        0.165132194502,
        0.168269400190,
        0.171441100733,
        0.174647403656,
        0.177888415984,
        0.181164244250,
        0.184474994500,
        0.187820772301,
        0.191201682741,
        0.194617830442,
        0.198069319560,
        0.201556253794,
        0.205078736390,
        0.208636870145,
        0.212230757414,
        0.215860500114,
        0.219526199729,
        0.223227957317,
        0.226965873510,
        0.230740048524,
        0.234550582161,
        0.238397573812,
        0.242281122466,
        0.246201326708,
        0.250158284730,
        0.254152094331,
        0.258182852922,
        0.262250657530,
        0.266355604803,
        0.270497791013,
        0.274677312060,
        0.278894263477,
        0.283148740430,
        0.287440837727,
        0.291770649818,
        0.296138270798,
        0.300543794416,
        0.304987314070,
        0.309468922818,
        0.313988713376,
        0.318546778125,
        0.323143209113,
        0.327778098057,
        0.332451536346,
        0.337163615048,
        0.341914424909,
        0.346704056355,
        0.351532599500,
        0.356400144146,
        0.361306779784,
        0.366252595599,
        0.371237680474,
        0.376262122991,
        0.381326011433,
        0.386429433787,
        0.391572477750,
        0.396755230726,
        0.401977779832,
        0.407240211902,
        0.412542613484,
        0.417885070848,
        0.423267669986,
        0.428690496614,
        0.434153636175,
        0.439657173841,
        0.445201194516,
        0.450785782838,
        0.456411023180,
        0.462076999654,
        0.467783796112,
        0.473531496148,
        0.479320183101,
        0.485149940056,
        0.491020849848,
        0.496932995061,
        0.502886458033,
        0.508881320855,
        0.514917665377,
        0.520995573204,
        0.527115125706,
        0.533276404011,
        0.539479489012,
        0.545724461370,
        0.552011401512,
        0.558340389634,
        0.564711505705,
        0.571124829465,
        0.577580440430,
        0.584078417891,
        0.590618840919,
        0.597201788364,
        0.603827338855,
        0.610495570808,
        0.617206562420,
        0.623960391675,
        0.630757136346,
        0.637596873994,
        0.644479681971,
        0.651405637420,
        0.658374817279,
        0.665387298282,
        0.672443156958,
        0.679542469633,
        0.686685312435,
        0.693871761292,
        0.701101891933,
        0.708375779892,
        0.715693500506,
        0.723055128922,
        0.730460740090,
        0.737910408773,
        0.745404209540,
        0.752942216776,
        0.760524504675,
        0.768151147248,
        0.775822218317,
        0.783537791526,
        0.791297940333,
        0.799102738014,
        0.806952257669,
        0.814846572216,
        0.822785754396,
        0.830769876775,
        0.838799011741,
        0.846873231510,
        0.854992608124,
        0.863157213454,
        0.871367119199,
        0.879622396888,
        0.887923117882,
        0.896269353374,
        0.904661174391,
        0.913098651793,
        0.921581856277,
        0.930110858375,
        0.938685728458,
        0.947306536733,
        0.955973353249,
        0.964686247894,
        0.973445290398,
        0.982250550333,
        0.991102097114,
        1.000000000000
        ]
        return 0.2126 * table[self.r as size_t] + 0.7152 * table[self.g as size_t] + 0.0722 * table[self.b as size_t]
    }
    public func contrast_ratio(&self, other : Color) : f64 {
        var a = self.relative_luminance()
        var b = other.relative_luminance()
        if(a < b) { var t = a; a = b; b = t }
        return (a + 0.05) / (b + 0.05)
    }
    public func is_opaque(&self) : bool { return self.a == 255u8 }
    public func to_premul_u32(&self) : u32 { return self.premultiply().to_u32() }
}

@direct_init
public struct Shadow {
    var x : float; var y : float; var blur : float; var spread : float; var color : Color
}
