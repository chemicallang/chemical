// P0.4 design-token checks. Exact values, scales, and WCAG contrast.
// These assertions are consumers of the generated Theme table; they do not
// define a second token source.

using std::Option;
using std::Result;

public namespace design {

func close(a : float, b : float) : bool {
    var d = a - b
    if(d < 0.0f) { d = 0.0f - d }
    return d < 0.0001f
}

func channel(v : u8) : f64 {
    // IEC 61966-2-1 sRGB linearization table. A table avoids libm so the
    // TCC link stays exactly the ui test link (no -lm dependency).
    var table : [256]f64 = [
        0.000000000000, 0.000303526984, 0.000607053967, 0.000910580951, 0.001214107934, 0.001517634918, 0.001821161901, 0.002124688885, 0.002428215868, 0.002731742852, 0.003035269835, 0.003346535764, 0.003676507324, 0.004024717018, 0.004391442037, 0.004776953481, 0.005181516702, 0.005605391624, 0.006048833023, 0.006512090793, 0.006995410187, 0.007499032043, 0.008023192985, 0.008568125618, 0.009134058702, 0.009721217320, 0.010329823030, 0.010960094006, 0.011612245180, 0.012286488357, 0.012983032342, 0.013702083047, 0.014443843596, 0.015208514423, 0.015996293366, 0.016807375753, 0.017641954488, 0.018500220128, 0.019382360957, 0.020288563057, 0.021219010376, 0.022173884793, 0.023153366178, 0.024157632449, 0.025186859627, 0.026241221895, 0.027320891639, 0.028426039504, 0.029556834438, 0.030713443733, 0.031896033073, 0.033104766571, 0.034339806809, 0.035601314875, 0.036889450401, 0.038204371595, 0.039546235277, 0.040915196907, 0.042311410621, 0.043735029257, 0.045186204386, 0.046665086337, 0.048171824227, 0.049706565984, 0.051269458374, 0.052860647023, 0.054480276442, 0.056128490050, 0.057805430191, 0.059511238163, 0.061246054232, 0.063010017653, 0.064803266693, 0.066625938644, 0.068478169844, 0.070360095697, 0.072271850682, 0.074213568380, 0.076185381481, 0.078187421805, 0.080219820314, 0.082282707130, 0.084376211544, 0.086500462037, 0.088655586286, 0.090841711183, 0.093058962847, 0.095307466631, 0.097587347142, 0.099898728247, 0.102241733088, 0.104616484091, 0.107023102978, 0.109461710778, 0.111932427837, 0.114435373827, 0.116970667759, 0.119538427988, 0.122138772230, 0.124771817561, 0.127437680436, 0.130136476690, 0.132868321554, 0.135633329655, 0.138431615032, 0.141263291140, 0.144128470858, 0.147027266498, 0.149959789811, 0.152926151996, 0.155926463708, 0.158960835061, 0.162029375639, 0.165132194502, 0.168269400190, 0.171441100733, 0.174647403656, 0.177888415984, 0.181164244250, 0.184474994500, 0.187820772301, 0.191201682741, 0.194617830442, 0.198069319560, 0.201556253794, 0.205078736390, 0.208636870145, 0.212230757414, 0.215860500114, 0.219526199729, 0.223227957317, 0.226965873510, 0.230740048524, 0.234550582161, 0.238397573812, 0.242281122466, 0.246201326708, 0.250158284730, 0.254152094331, 0.258182852922, 0.262250657530, 0.266355604803, 0.270497791013, 0.274677312060, 0.278894263477, 0.283148740430, 0.287440837727, 0.291770649818, 0.296138270798, 0.300543794416, 0.304987314070, 0.309468922818, 0.313988713376, 0.318546778125, 0.323143209113, 0.327778098057, 0.332451536346, 0.337163615048, 0.341914424909, 0.346704056355, 0.351532599500, 0.356400144146, 0.361306779784, 0.366252595599, 0.371237680474, 0.376262122991, 0.381326011433, 0.386429433787, 0.391572477750, 0.396755230726, 0.401977779832, 0.407240211902, 0.412542613484, 0.417885070848, 0.423267669986, 0.428690496614, 0.434153636175, 0.439657173841, 0.445201194516, 0.450785782838, 0.456411023180, 0.462076999654, 0.467783796112, 0.473531496148, 0.479320183101, 0.485149940056, 0.491020849848, 0.496932995061, 0.502886458033, 0.508881320855, 0.514917665377, 0.520995573204, 0.527115125706, 0.533276404011, 0.539479489012, 0.545724461370, 0.552011401512, 0.558340389634, 0.564711505705, 0.571124829465, 0.577580440430, 0.584078417891, 0.590618840919, 0.597201788364, 0.603827338855, 0.610495570808, 0.617206562420, 0.623960391675, 0.630757136346, 0.637596873994, 0.644479681971, 0.651405637420, 0.658374817279, 0.665387298282, 0.672443156958, 0.679542469633, 0.686685312435, 0.693871761292, 0.701101891933, 0.708375779892, 0.715693500506, 0.723055128922, 0.730460740090, 0.737910408773, 0.745404209540, 0.752942216776, 0.760524504675, 0.768151147248, 0.775822218317, 0.783537791526, 0.791297940333, 0.799102738014, 0.806952257669, 0.814846572216, 0.822785754396, 0.830769876775, 0.838799011741, 0.846873231510, 0.854992608124, 0.863157213454, 0.871367119199, 0.879622396888, 0.887923117882, 0.896269353374, 0.904661174391, 0.913098651793, 0.921581856277, 0.930110858375, 0.938685728458, 0.947306536733, 0.955973353249, 0.964686247894, 0.973445290398, 0.982250550333, 0.991102097114, 1.000000000000
    ]
    return table[v as size_t]
}

func luminance(c : Color) : f64 {
    return 0.2126 * channel(c.r) + 0.7152 * channel(c.g) + 0.0722 * channel(c.b)
}

func contrast(a : Color, b : Color) : f64 {
    var la = luminance(a)
    var lb = luminance(b)
    var hi = la
    var lo = lb
    if(lb > la) { hi = lb; lo = la }
    return (hi + 0.05) / (lo + 0.05)
}

func expect_color(name : *char, got : Color, want : Color) : bool {
    if(got.r != want.r || got.g != want.g || got.b != want.b || got.a != want.a) {
        unsafe { printf("design FAIL %s got=%u,%u,%u,%u want=%u,%u,%u,%u\n", name, got.r, got.g, got.b, got.a, want.r, want.g, want.b, want.a) }
        return false
    }
    return true
}

func expect_contrast(name : *char, a : Color, b : Color, threshold : f64) : bool {
    var ratio = contrast(a, b)
    if(ratio + 0.0001 < threshold) {
        unsafe { printf("design FAIL %s contrast=%.3f threshold=%.2f\n", name, ratio, threshold) }
        return false
    }
    return true
}

func light_colors() : [32]Color {
    return [
        Color { r : 0u8, g : 0u8, b : 0u8, a : 0u8 },
        Color { r : 248u8, g : 250u8, b : 252u8, a : 255u8 },
        Color { r : 255u8, g : 255u8, b : 255u8, a : 255u8 },
        Color { r : 241u8, g : 245u8, b : 249u8, a : 255u8 },
        Color { r : 255u8, g : 255u8, b : 255u8, a : 255u8 },
        Color { r : 15u8, g : 23u8, b : 42u8, a : 255u8 },
        Color { r : 71u8, g : 85u8, b : 105u8, a : 255u8 },
        Color { r : 100u8, g : 116u8, b : 139u8, a : 255u8 },
        Color { r : 37u8, g : 99u8, b : 235u8, a : 255u8 },
        Color { r : 255u8, g : 255u8, b : 255u8, a : 255u8 },
        Color { r : 29u8, g : 78u8, b : 216u8, a : 255u8 },
        Color { r : 30u8, g : 64u8, b : 175u8, a : 255u8 },
        Color { r : 226u8, g : 232u8, b : 240u8, a : 255u8 },
        Color { r : 15u8, g : 23u8, b : 42u8, a : 255u8 },
        Color { r : 220u8, g : 38u8, b : 38u8, a : 255u8 },
        Color { r : 255u8, g : 255u8, b : 255u8, a : 255u8 },
        Color { r : 21u8, g : 128u8, b : 61u8, a : 255u8 },
        Color { r : 255u8, g : 255u8, b : 255u8, a : 255u8 },
        Color { r : 180u8, g : 83u8, b : 9u8, a : 255u8 },
        Color { r : 255u8, g : 255u8, b : 255u8, a : 255u8 },
        Color { r : 226u8, g : 232u8, b : 240u8, a : 255u8 },
        Color { r : 203u8, g : 213u8, b : 225u8, a : 255u8 },
        Color { r : 37u8, g : 99u8, b : 235u8, a : 255u8 },
        Color { r : 37u8, g : 99u8, b : 235u8, a : 255u8 },
        Color { r : 219u8, g : 234u8, b : 254u8, a : 255u8 },
        Color { r : 15u8, g : 23u8, b : 42u8, a : 115u8 },
        Color { r : 37u8, g : 99u8, b : 235u8, a : 255u8 },
        Color { r : 13u8, g : 148u8, b : 136u8, a : 255u8 },
        Color { r : 217u8, g : 119u8, b : 6u8, a : 255u8 },
        Color { r : 124u8, g : 58u8, b : 237u8, a : 255u8 },
        Color { r : 219u8, g : 39u8, b : 119u8, a : 255u8 },
        Color { r : 100u8, g : 116u8, b : 139u8, a : 255u8 }
    ]
}

func dark_colors() : [32]Color {
    return [
        Color { r : 0u8, g : 0u8, b : 0u8, a : 0u8 },
        Color { r : 11u8, g : 18u8, b : 32u8, a : 255u8 },
        Color { r : 17u8, g : 24u8, b : 39u8, a : 255u8 },
        Color { r : 2u8, g : 6u8, b : 23u8, a : 255u8 },
        Color { r : 17u8, g : 24u8, b : 39u8, a : 255u8 },
        Color { r : 248u8, g : 250u8, b : 252u8, a : 255u8 },
        Color { r : 203u8, g : 213u8, b : 225u8, a : 255u8 },
        Color { r : 148u8, g : 163u8, b : 184u8, a : 255u8 },
        Color { r : 96u8, g : 165u8, b : 250u8, a : 255u8 },
        Color { r : 11u8, g : 18u8, b : 32u8, a : 255u8 },
        Color { r : 147u8, g : 197u8, b : 253u8, a : 255u8 },
        Color { r : 59u8, g : 130u8, b : 246u8, a : 255u8 },
        Color { r : 31u8, g : 41u8, b : 55u8, a : 255u8 },
        Color { r : 248u8, g : 250u8, b : 252u8, a : 255u8 },
        Color { r : 248u8, g : 113u8, b : 113u8, a : 255u8 },
        Color { r : 11u8, g : 18u8, b : 32u8, a : 255u8 },
        Color { r : 74u8, g : 222u8, b : 128u8, a : 255u8 },
        Color { r : 11u8, g : 18u8, b : 32u8, a : 255u8 },
        Color { r : 251u8, g : 191u8, b : 36u8, a : 255u8 },
        Color { r : 11u8, g : 18u8, b : 32u8, a : 255u8 },
        Color { r : 31u8, g : 41u8, b : 55u8, a : 255u8 },
        Color { r : 51u8, g : 65u8, b : 85u8, a : 255u8 },
        Color { r : 147u8, g : 197u8, b : 253u8, a : 255u8 },
        Color { r : 147u8, g : 197u8, b : 253u8, a : 255u8 },
        Color { r : 30u8, g : 58u8, b : 138u8, a : 255u8 },
        Color { r : 0u8, g : 0u8, b : 0u8, a : 153u8 },
        Color { r : 96u8, g : 165u8, b : 250u8, a : 255u8 },
        Color { r : 45u8, g : 212u8, b : 191u8, a : 255u8 },
        Color { r : 251u8, g : 191u8, b : 36u8, a : 255u8 },
        Color { r : 167u8, g : 139u8, b : 250u8, a : 255u8 },
        Color { r : 244u8, g : 114u8, b : 182u8, a : 255u8 },
        Color { r : 148u8, g : 163u8, b : 184u8, a : 255u8 }
    ]
}

func high_contrast_colors() : [32]Color {
    return [
        Color { r : 0u8, g : 0u8, b : 0u8, a : 0u8 },
        Color { r : 0u8, g : 0u8, b : 0u8, a : 255u8 },
        Color { r : 0u8, g : 0u8, b : 0u8, a : 255u8 },
        Color { r : 0u8, g : 0u8, b : 0u8, a : 255u8 },
        Color { r : 0u8, g : 0u8, b : 0u8, a : 255u8 },
        Color { r : 255u8, g : 255u8, b : 255u8, a : 255u8 },
        Color { r : 255u8, g : 255u8, b : 255u8, a : 255u8 },
        Color { r : 255u8, g : 255u8, b : 255u8, a : 255u8 },
        Color { r : 255u8, g : 212u8, b : 0u8, a : 255u8 },
        Color { r : 0u8, g : 0u8, b : 0u8, a : 255u8 },
        Color { r : 255u8, g : 212u8, b : 0u8, a : 255u8 },
        Color { r : 255u8, g : 212u8, b : 0u8, a : 255u8 },
        Color { r : 0u8, g : 0u8, b : 0u8, a : 255u8 },
        Color { r : 255u8, g : 255u8, b : 255u8, a : 255u8 },
        Color { r : 255u8, g : 107u8, b : 107u8, a : 255u8 },
        Color { r : 0u8, g : 0u8, b : 0u8, a : 255u8 },
        Color { r : 105u8, g : 240u8, b : 174u8, a : 255u8 },
        Color { r : 0u8, g : 0u8, b : 0u8, a : 255u8 },
        Color { r : 255u8, g : 212u8, b : 0u8, a : 255u8 },
        Color { r : 0u8, g : 0u8, b : 0u8, a : 255u8 },
        Color { r : 255u8, g : 255u8, b : 255u8, a : 255u8 },
        Color { r : 255u8, g : 255u8, b : 255u8, a : 255u8 },
        Color { r : 255u8, g : 212u8, b : 0u8, a : 255u8 },
        Color { r : 255u8, g : 212u8, b : 0u8, a : 255u8 },
        Color { r : 255u8, g : 212u8, b : 0u8, a : 255u8 },
        Color { r : 0u8, g : 0u8, b : 0u8, a : 217u8 },
        Color { r : 255u8, g : 212u8, b : 0u8, a : 255u8 },
        Color { r : 105u8, g : 240u8, b : 174u8, a : 255u8 },
        Color { r : 255u8, g : 107u8, b : 107u8, a : 255u8 },
        Color { r : 255u8, g : 255u8, b : 255u8, a : 255u8 },
        Color { r : 255u8, g : 212u8, b : 0u8, a : 255u8 },
        Color { r : 105u8, g : 240u8, b : 174u8, a : 255u8 }
    ]
}

func check_colors(t : &Theme, want : *mut Color, mode : *char) : bool {
    var ok = true
    var i : size_t = 0
    while(i < 32u) {
        var got = t.color(i as ColorToken)
        var w = unsafe(want[i])
        if(!expect_color(mode, got, w)) { ok = false }
        i = i + 1u
    }
    return ok
}

func check_spaces(t : &mut Theme, dense : Density, want : *mut float, name : *char) : bool {
    var ok = true
    t.set_density(dense)
    if(t.density() != dense) { unsafe { printf("design FAIL %s density\n", name) }; ok = false }
    var i : size_t = 0
    while(i < 9u) {
        var got = t.space(i as Space)
        var want_i = unsafe(want[i])
        if(!close(got, want_i)) {
            unsafe { printf("design FAIL %s space[%lu]=%.1f want=%.1f\n", name, i, got, want_i) }
            ok = false
        }
        i = i + 1u
    }
    return ok
}

func check_radii(t : &Theme) : bool {
    var want = [0.0f, 2.0f, 4.0f, 6.0f, 8.0f, 12.0f, 999.0f]
    var ok = true
    var i : size_t = 0
    while(i < 7u) {
        if(!close(t.radius(i as Radius), want[i])) { unsafe { printf("design FAIL radius[%lu]\n", i) }; ok = false }
        i = i + 1u
    }
    return ok
}

func check_type(t : &Theme) : bool {
    var ok = true
    var sizes = [40.0f, 32.0f, 24.0f, 20.0f, 16.0f, 14.0f, 14.0f, 13.0f, 12.0f, 13.0f]
    var lines_px = [48.0f, 40.0f, 32.0f, 28.0f, 24.0f, 20.0f, 20.0f, 18.0f, 16.0f, 20.0f]
    var weights = [600, 700, 600, 600, 600, 400, 600, 500, 400, 400]
    var letters = [-0.01f, -0.01f, -0.01f, 0f, 0f, 0f, 0f, 0f, 0.01f, 0f]
    var i : size_t = 0
    while(i < 10u) {
        var s = t.text(i as TextRole)
        if(!close(s.size, sizes[i]) || !close(s.size * s.line_height, lines_px[i]) || !close(s.letter_spacing, letters[i])) {
            unsafe { printf("design FAIL text role %lu size/line/letter\n", i) }
            ok = false
        }
        var want_weight = FontWeight.Regular
        if(weights[i] == 500) { want_weight = FontWeight.Medium }
        if(weights[i] == 600) { want_weight = FontWeight.Semibold }
        if(weights[i] == 700) { want_weight = FontWeight.Bold }
        if(s.weight != want_weight) { unsafe { printf("design FAIL text role %lu weight\n", i) }; ok = false }
        var want_family = FontFamily.Ui
        if(i == 9u) { want_family = FontFamily.Mono }
        if(s.family != want_family) { unsafe { printf("design FAIL text role %lu family\n", i) }; ok = false }
        i = i + 1u
    }
    return ok
}

func check_motion(t : &Theme) : bool {
    var ok = true
    if(!close(t.motion(MotionToken.Fast).ms, 120.0f) || !close(t.motion(MotionToken.Base).ms, 160.0f) || !close(t.motion(MotionToken.Slow).ms, 240.0f) || !close(t.motion(MotionToken.Spring).ms, 220.0f)) { unsafe { printf("design FAIL motion durations\n") }; ok = false }
    if(!close(t.disabled_opacity(), 0.4f) || !close(t.text_scale(), 1.0f)) { unsafe { printf("design FAIL disabled/text scale\n") }; ok = false }
    return ok
}

func check_elevation(t : &Theme, mode : *char) : bool {
    var low = t.elevation(Elevation.Low)
    var med = t.elevation(Elevation.Medium)
    var high = t.elevation(Elevation.High)
    var ok = true
    if(mode_equals(mode, "high-contrast")) {
        if(low.blur != 0.0f || med.blur != 0.0f || high.blur != 0.0f) { unsafe { printf("design FAIL high-contrast shadows must be none\n") }; ok = false }
        return ok
    }
    var dark = mode_equals(mode, "dark")
    var alpha = 20u8
    if(dark) { alpha = 89u8 }
    if(!close(low.y, 1.0f) || !close(low.blur, 2.0f) || low.color.a != alpha) { unsafe { printf("design FAIL low shadow\n") }; ok = false }
    if(!close(med.y, 4.0f) || !close(med.blur, 8.0f)) { unsafe { printf("design FAIL medium shadow\n") }; ok = false }
    if(!close(high.y, 12.0f) || !close(high.blur, 24.0f)) { unsafe { printf("design FAIL high shadow\n") }; ok = false }
    return ok
}

func mode_equals(a : *char, b : *char) : bool { unsafe { return strcmp(a, b) == 0 } }

public func run() : bool {
    var ok = true
    var light = Theme.light()
    var dark = Theme.dark()
    var hc = Theme.high_contrast()
    var lc = light_colors(); var dc = dark_colors(); var hcc = high_contrast_colors()
    if(!check_colors(&light, &raw mut lc[0], "light")) { ok = false }
    if(!check_colors(&dark, &raw mut dc[0], "dark")) { ok = false }
    if(!check_colors(&hc, &raw mut hcc[0], "high-contrast")) { ok = false }
    var comfy = [0.0f, 2.0f, 4.0f, 8.0f, 12.0f, 16.0f, 24.0f, 32.0f, 48.0f]
    var compact = [0.0f, 2.0f, 4.0f, 6.0f, 10.0f, 14.0f, 20.0f, 28.0f, 40.0f]
    if(!check_spaces(&mut light, Density.Comfortable, &raw mut comfy[0], "comfortable")) { ok = false }
    if(!check_spaces(&mut light, Density.Compact, &raw mut compact[0], "compact")) { ok = false }
    if(!check_spaces(&mut dark, Density.Compact, &raw mut compact[0], "compact-dark")) { ok = false }
    if(!check_spaces(&mut hc, Density.Compact, &raw mut compact[0], "compact-high-contrast")) { ok = false }
    if(!check_radii(&light)) { ok = false }
    if(!check_type(&light)) { ok = false }
    if(!check_motion(&light)) { ok = false }
    if(!check_elevation(&light, "light")) { ok = false }
    if(!check_elevation(&dark, "dark")) { ok = false }
    if(!check_elevation(&hc, "high-contrast")) { ok = false }
    if(!expect_contrast("light OnSurface/Surface", light.color(ColorToken.OnSurface), light.color(ColorToken.Surface), 4.5)) { ok = false }
    if(!expect_contrast("light OnSurfaceMuted/Surface", light.color(ColorToken.OnSurfaceMuted), light.color(ColorToken.Surface), 4.5)) { ok = false }
    if(!expect_contrast("light OnPrimary/Primary", light.color(ColorToken.OnPrimary), light.color(ColorToken.Primary), 4.5)) { ok = false }
    if(!expect_contrast("light OnDanger/Danger", light.color(ColorToken.OnDanger), light.color(ColorToken.Danger), 4.5)) { ok = false }
    if(!expect_contrast("light OnSurfaceFaint/Surface", light.color(ColorToken.OnSurfaceFaint), light.color(ColorToken.Surface), 4.5)) { ok = false }
    if(!expect_contrast("light FocusRing/Surface", light.color(ColorToken.FocusRing), light.color(ColorToken.Surface), 3.0)) { ok = false }
    if(!expect_contrast("dark OnSurface/Surface", dark.color(ColorToken.OnSurface), dark.color(ColorToken.Surface), 4.5)) { ok = false }
    if(!expect_contrast("dark OnSurfaceMuted/Surface", dark.color(ColorToken.OnSurfaceMuted), dark.color(ColorToken.Surface), 4.5)) { ok = false }
    if(!expect_contrast("dark OnPrimary/Primary", dark.color(ColorToken.OnPrimary), dark.color(ColorToken.Primary), 4.5)) { ok = false }
    if(!expect_contrast("dark OnDanger/Danger", dark.color(ColorToken.OnDanger), dark.color(ColorToken.Danger), 4.5)) { ok = false }
    if(!expect_contrast("dark OnSurfaceFaint/Surface", dark.color(ColorToken.OnSurfaceFaint), dark.color(ColorToken.Surface), 4.5)) { ok = false }
    if(!expect_contrast("dark FocusRing/Surface", dark.color(ColorToken.FocusRing), dark.color(ColorToken.Surface), 3.0)) { ok = false }
    if(!expect_contrast("hc OnSurface/Surface", hc.color(ColorToken.OnSurface), hc.color(ColorToken.Surface), 4.5)) { ok = false }
    if(!expect_contrast("hc OnPrimary/Primary", hc.color(ColorToken.OnPrimary), hc.color(ColorToken.Primary), 4.5)) { ok = false }
    if(!expect_contrast("hc OnDanger/Danger", hc.color(ColorToken.OnDanger), hc.color(ColorToken.Danger), 4.5)) { ok = false }
    if(!expect_contrast("hc FocusRing/Surface", hc.color(ColorToken.FocusRing), hc.color(ColorToken.Surface), 3.0)) { ok = false }
    return ok
}

} // namespace design
