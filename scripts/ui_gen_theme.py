#!/usr/bin/env python3
"""Generate lang/libs/ui/src/theme.ch from lang/docs/ui/tokens.json.

tokens.json is the single source of truth. `--check` regenerates in memory and
fails if the checked-in theme.ch differs (no hand-maintained second table).
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOKENS = os.path.join(ROOT, "lang/docs/ui/tokens.json")
THEME = os.path.join(ROOT, "lang/libs/ui/src/theme.ch")

COLOR_TOKENS = [
    "Transparent", "Surface", "SurfaceRaised", "SurfaceSunken", "SurfaceOverlay",
    "OnSurface", "OnSurfaceMuted", "OnSurfaceFaint", "Primary", "OnPrimary",
    "PrimaryHover", "PrimaryPressed", "Secondary", "OnSecondary", "Danger",
    "OnDanger", "Success", "OnSuccess", "Warning", "OnWarning", "Border",
    "BorderStrong", "BorderFocus", "FocusRing", "Selection", "Scrim",
    "Chart1", "Chart2", "Chart3", "Chart4", "Chart5", "Chart6",
]
SPACES = ["None", "XXS", "XS", "SM", "MD", "LG", "XL", "XXL", "XXXL"]
RADII = ["None", "XS", "SM", "MD", "LG", "XL", "Full"]
TEXT_ROLES = ["Display", "H1", "H2", "H3", "H4", "Body", "BodyStrong", "Label", "Caption", "Code"]
ELEVATIONS = ["None", "Low", "Medium", "High"]
MOTIONS = ["Fast", "Base", "Slow", "Spring"]
WEIGHTS = {400: "Regular", 500: "Medium", 600: "Semibold", 700: "Bold"}
FAMILIES = {"Ui": "Ui", "Mono": "Mono", "System": "System"}


def fail(msg):
    print("ui-gen-theme: " + msg, file=sys.stderr)
    sys.exit(1)


def parse_color(value):
    hex_match = re.fullmatch(r"#([0-9a-fA-F]{6})([0-9a-fA-F]{2})?", value)
    if hex_match:
        h = hex_match.group(1)
        a = int(hex_match.group(2), 16) if hex_match.group(2) else 255
        return int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), a
    rgba_match = re.fullmatch(
        r"rgba\(\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]*\.?[0-9]+)\s*\)",
        value,
    )
    if rgba_match:
        return (
            int(rgba_match.group(1)),
            int(rgba_match.group(2)),
            int(rgba_match.group(3)),
            round(float(rgba_match.group(4)) * 255.0),
        )
    fail(f"unsupported color value {value!r}")


def color_literal(value):
    r, g, b, a = parse_color(value)
    return f"Color {{ r : {r}u8, g : {g}u8, b : {b}u8, a : {a}u8 }}"


def arr(values, literal):
    return "[" + ", ".join(literal(v) for v in values) + "]"


def theme_literal(tokens, mode):
    colors = tokens["colors"]["modes"][mode]
    if list(colors.keys()) != COLOR_TOKENS:
        fail(f"{mode} color keys do not match ColorToken order")
    spaces = tokens["spacing"]["comfortable"]
    compact = tokens["spacing"]["compact"]
    if list(spaces.keys()) != SPACES or list(compact.keys()) != SPACES:
        fail("spacing keys do not match Space order")
    radii = tokens["radii"]
    if list(radii.keys()) != RADII:
        fail("radii keys do not match Radius order")
    roles = tokens["typography"]["roles"]
    if list(roles.keys()) != TEXT_ROLES:
        fail("typography roles do not match TextRole order")
    for role, spec in roles.items():
        if spec["weight"] not in WEIGHTS:
            fail(f"unsupported font weight {spec['weight']} for {role}")
        if spec["family"] not in FAMILIES:
            fail(f"unsupported font family {spec['family']} for {role}")
    elev = tokens["elevation"][mode]
    if list(elev.keys()) != ELEVATIONS:
        fail(f"{mode} elevation keys do not match Elevation order")
    motion = tokens["motion"]
    if any(name not in motion for name in MOTIONS):
        fail("motion tokens incomplete")

    def first_shadow(e):
        layers = elev[e]["layers"]
        if not layers:
            return "Shadow { x : 0.0f, y : 0.0f, blur : 0.0f, spread : 0.0f, color : Color { r : 0u8, g : 0u8, b : 0u8, a : 0u8 } }"
        layer = layers[0]
        return (
            "Shadow { x : %sf, y : %sf, blur : %sf, spread : %sf, color : %s }"
            % (layer["x"], layer["y"], layer["blur"], layer["spread"], color_literal(layer["color"]))
        )

    return (
        "Theme {\n"
        f"            colors : {arr([colors[c] for c in COLOR_TOKENS], color_literal)},\n"
        f"            spaces : {arr([spaces[s] for s in SPACES], lambda v: f'{v}.0f')},\n"
        f"            compact_spaces : {arr([compact[s] for s in SPACES], lambda v: f'{v}.0f')},\n"
        f"            radii : {arr([radii[r] for r in RADII], lambda v: f'{v}.0f')},\n"
        f"            text_sizes : {arr([roles[r]['size'] for r in TEXT_ROLES], lambda v: f'{v}.0f')},\n"
        f"            text_lines : {arr([roles[r]['line'] for r in TEXT_ROLES], lambda v: f'{v}.0f')},\n"
        f"            text_weights : {arr([roles[r]['weight'] for r in TEXT_ROLES], lambda v: 'FontWeight.' + WEIGHTS[v])},\n"
        f"            text_letters : {arr([roles[r]['letter_spacing'] for r in TEXT_ROLES], lambda v: f'{v}f')},\n"
        f"            text_families : {arr([roles[r]['family'] for r in TEXT_ROLES], lambda v: 'FontFamily.' + FAMILIES[v])},\n"
        f"            elevations : {arr([first_shadow(e) for e in ELEVATIONS], lambda v: v)},\n"
        f"            motions : {arr([motion[m]['ms'] for m in MOTIONS], lambda v: f'{v}.0f')},\n"
        "            density_value : Density.Comfortable,\n"
        "            scale_value : 1.0f,\n"
        f"            disabled_value : {tokens['disabled_opacity']}f,\n"
        "            compact_value : false\n"
        "        }"
    )


def generate():
    raw = open(TOKENS, encoding="utf-8").read()
    tokens = json.loads(raw)
    for key in ("colors", "spacing", "radii", "typography", "elevation", "motion",
                "breakpoints", "density", "focus", "contrast", "layout", "disabled_opacity"):
        if key not in tokens:
            fail(f"tokens.json missing {key}")
    if tokens["disabled_opacity"] != 0.4:
        fail("disabled_opacity must be 0.4")
    if tokens["focus"]["light"] != {"width": 2, "offset": 2}:
        fail("light focus ring must be 2px / 2px")
    if tokens["focus"]["dark"] != {"width": 2, "offset": 2}:
        fail("dark focus ring must be 2px / 2px")
    if tokens["focus"]["high-contrast"] != {"width": 3, "offset": 2}:
        fail("high-contrast focus ring must be 3px / 2px")
    if tokens["breakpoints"] != {
        "Sm": {"min": 0, "max": 639},
        "Md": {"min": 640, "max": 1023},
        "Lg": {"min": 1024, "max": 1439},
        "Xl": {"min": 1440, "max": None},
    }:
        fail("breakpoints must be Sm <640, Md 640-1023, Lg 1024-1439, Xl >=1440")
    if tokens["layout"]["min_touch"] != 24 or tokens["layout"]["preferred_touch"] != 44:
        fail("touch targets must be 24/44")

    lines = []
    lines.append("// GENERATED by scripts/ui-gen-theme.sh from lang/docs/ui/tokens.json.")
    lines.append("// Do not edit by hand; edit tokens.json and run the generator.")
    lines.append("// P0.4 token table; P1.5 consumes these exact values.")
    lines.append("")
    lines.append("using std::string_view;")
    lines.append("")
    lines.append("public enum ThemeMode { Light, Dark, HighContrast, System }")
    lines.append("public enum Density { Comfortable, Compact }")
    lines.append("public enum FontWeight { Regular, Medium, Semibold, Bold }")
    lines.append("public enum FontFamily { Ui, Mono, System }")
    lines.append("public enum MotionToken { Fast, Base, Slow, Spring }")
    lines.append("")
    lines.append("public enum ColorToken {")
    for i, name in enumerate(COLOR_TOKENS):
        comma = "," if i + 1 < len(COLOR_TOKENS) else ""
        lines.append(f"    {name}{comma}")
    lines.append("}")
    lines.append("")
    lines.append("public enum Space { " + ", ".join(SPACES) + " }")
    lines.append("public enum Radius { " + ", ".join(RADII) + " }")
    lines.append("public enum Elevation { " + ", ".join(ELEVATIONS) + " }")
    lines.append("public enum TextRole { " + ", ".join(TEXT_ROLES) + " }")
    lines.append("")
    b = tokens["breakpoints"]
    d = tokens["density"]
    f = tokens["focus"]
    lines.append("// Internal constants for values Theme does not expose (breakpoints,")
    lines.append("// density metrics, focus geometry). P1.5 consumes them; they are not API.")
    lines.append(f"const UI_BREAKPOINT_SM_MAX : u32 = {b['Sm']['max']}u32")
    lines.append(f"const UI_BREAKPOINT_MD_MIN : u32 = {b['Md']['min']}u32")
    lines.append(f"const UI_BREAKPOINT_MD_MAX : u32 = {b['Md']['max']}u32")
    lines.append(f"const UI_BREAKPOINT_LG_MIN : u32 = {b['Lg']['min']}u32")
    lines.append(f"const UI_BREAKPOINT_LG_MAX : u32 = {b['Lg']['max']}u32")
    lines.append(f"const UI_BREAKPOINT_XL_MIN : u32 = {b['Xl']['min']}u32")
    for name, key in (("CONTROL_SM", "control_small"), ("CONTROL_MD", "control_medium"), ("CONTROL_LG", "control_large")):
        lines.append(f"const UI_COMFY_{name} : u32 = {d['Comfortable'][key]}u32")
        lines.append(f"const UI_COMPACT_{name} : u32 = {d['Compact'][key]}u32")
    lines.append(f"const UI_COMFY_TABLE_ROW : u32 = {d['Comfortable']['table_row']}u32")
    lines.append(f"const UI_COMPACT_TABLE_ROW : u32 = {d['Compact']['table_row']}u32")
    lines.append(f"const UI_COMFY_TABLE_HEADER : u32 = {d['Comfortable']['table_header']}u32")
    lines.append(f"const UI_COMPACT_TABLE_HEADER : u32 = {d['Compact']['table_header']}u32")
    lines.append(f"const UI_SIDEBAR_LG : u32 = {d['Comfortable']['sidebar_lg']}u32")
    lines.append(f"const UI_SIDEBAR_XL : u32 = {d['Comfortable']['sidebar_xl']}u32")
    lines.append(f"const UI_PAGE_PADDING_SM : u32 = {d['Comfortable']['page_padding']['Sm']}u32")
    lines.append(f"const UI_PAGE_PADDING_MD : u32 = {d['Comfortable']['page_padding']['Md']}u32")
    lines.append(f"const UI_PAGE_PADDING_LG : u32 = {d['Comfortable']['page_padding']['Lg']}u32")
    lines.append(f"const UI_FOCUS_WIDTH_LIGHT : u32 = {f['light']['width']}u32")
    lines.append(f"const UI_FOCUS_WIDTH_DARK : u32 = {f['dark']['width']}u32")
    lines.append(f"const UI_FOCUS_WIDTH_HC : u32 = {f['high-contrast']['width']}u32")
    lines.append(f"const UI_FOCUS_OFFSET_LIGHT : u32 = {f['light']['offset']}u32")
    lines.append(f"const UI_FOCUS_OFFSET_DARK : u32 = {f['dark']['offset']}u32")
    lines.append(f"const UI_FOCUS_OFFSET_HC : u32 = {f['high-contrast']['offset']}u32")
    lines.append(f"const UI_BORDER_WIDTH_HC : u32 = {f['border_width']['high-contrast']}u32")
    lines.append(f"const UI_DISABLED_OPACITY : float = {tokens['disabled_opacity']}f")
    lines.append("")
    lines.append("@direct_init")
    lines.append("public struct Color { var r : u8; var g : u8; var b : u8; var a : u8 }")
    lines.append("")
    lines.append("@direct_init")
    lines.append("public struct Shadow {")
    lines.append("    var x : float; var y : float; var blur : float; var spread : float; var color : Color")
    lines.append("}")
    lines.append("")
    lines.append("@direct_init")
    lines.append("public struct Duration { var ms : float }")
    lines.append("")
    lines.append("@direct_init")
    lines.append("public struct TextFeatures {")
    lines.append("    var ligatures : bool; var tabular_numbers : bool; var slashed_zero : bool")
    lines.append("    var stylistic_set : u8; var raw_tags : &[u32]")
    lines.append("}")
    lines.append("")
    lines.append("@direct_init")
    lines.append("public struct TextStyle {")
    lines.append("    var family : FontFamily")
    lines.append("    var family_name : string_view")
    lines.append("    var size : float")
    lines.append("    var weight : FontWeight")
    lines.append("    var italic : bool")
    lines.append("    var line_height : float       // multiplier")
    lines.append("    var letter_spacing : float")
    lines.append("    var features : TextFeatures")
    lines.append("    var tab_size : u8")
    lines.append("}")
    lines.append("")
    lines.append("public struct Theme {")
    lines.append("    var colors : [32]Color")
    lines.append("    var spaces : [9]float")
    lines.append("    var compact_spaces : [9]float")
    lines.append("    var radii : [7]float")
    lines.append("    var text_sizes : [10]float")
    lines.append("    var text_lines : [10]float")
    lines.append("    var text_weights : [10]FontWeight")
    lines.append("    var text_letters : [10]float")
    lines.append("    var text_families : [10]FontFamily")
    lines.append("    var elevations : [4]Shadow")
    lines.append("    var motions : [4]float")
    lines.append("    var density_value : Density")
    lines.append("    var scale_value : float")
    lines.append("    var disabled_value : float")
    lines.append("    var compact_value : bool")
    lines.append("")
    for mode, name in (("light", "light"), ("dark", "dark"), ("high-contrast", "high_contrast")):
        lines.append(f"    public func {name}() : Theme {{")
        lines.append("        return " + theme_literal(tokens, mode))
        lines.append("    }")
        lines.append("")
    lines.append("    public func color(&self, t : ColorToken) : Color { return self.colors[t as size_t] }")
    lines.append("")
    lines.append("    public func space(&self, s : Space) : float {")
    lines.append("        if(self.compact_value) { return self.compact_spaces[s as size_t] }")
    lines.append("        return self.spaces[s as size_t]")
    lines.append("    }")
    lines.append("")
    lines.append("    public func radius(&self, r : Radius) : float { return self.radii[r as size_t] }")
    lines.append("")
    lines.append("    public func text(&self, r : TextRole) : TextStyle {")
    lines.append("        var ix = r as size_t")
    lines.append("        var size = self.text_sizes[ix] * self.scale_value")
    lines.append("        var line = self.text_lines[ix] * self.scale_value")
    lines.append("        return TextStyle {")
    lines.append("            family : self.text_families[ix],")
    lines.append("            family_name : \"\",")
    lines.append("            size : size,")
    lines.append("            weight : self.text_weights[ix],")
    lines.append("            italic : false,")
    lines.append("            line_height : line / self.text_sizes[ix],")
    lines.append("            letter_spacing : self.text_letters[ix],")
    lines.append("            features : TextFeatures { ligatures : false, tabular_numbers : false, slashed_zero : false, stylistic_set : 0u8, raw_tags : zeroed<[0]u32>() },")
    lines.append("            tab_size : 4u8")
    lines.append("        }")
    lines.append("    }")
    lines.append("")
    lines.append("    public func elevation(&self, e : Elevation) : Shadow { return self.elevations[e as size_t] }")
    lines.append("    public func density(&self) : Density { return self.density_value }")
    lines.append("    public func text_scale(&self) : float { return self.scale_value }")
    lines.append("    public func motion(&self, m : MotionToken) : Duration { return Duration { ms : self.motions[m as size_t] } }")
    lines.append("    public func disabled_opacity(&self) : float { return self.disabled_value }")
    lines.append("")
    lines.append("    public func set_color(&mut self, t : ColorToken, c : Color) { self.colors[t as size_t] = c }")
    lines.append("    public func set_space(&mut self, s : Space, px : float) {")
    lines.append("        self.spaces[s as size_t] = px")
    lines.append("        self.compact_spaces[s as size_t] = px")
    lines.append("    }")
    lines.append("    public func set_radius(&mut self, r : Radius, px : float) { self.radii[r as size_t] = px }")
    lines.append("    public func set_text_style(&mut self, role : TextRole, t : TextStyle) {")
    lines.append("        var ix = role as size_t")
    lines.append("        self.text_sizes[ix] = t.size")
    lines.append("        self.text_lines[ix] = t.size * t.line_height")
    lines.append("        self.text_weights[ix] = t.weight")
    lines.append("        self.text_letters[ix] = t.letter_spacing")
    lines.append("        self.text_families[ix] = t.family")
    lines.append("    }")
    lines.append("    public func set_density(&mut self, d : Density) {")
    lines.append("        self.density_value = d")
    lines.append("        self.compact_value = (d as int) == (Density.Compact as int)")
    lines.append("    }")
    lines.append("    public func set_text_scale(&mut self, scale : float) { self.scale_value = scale }")
    lines.append("}")
    lines.append("")
    return "\n".join(lines)


def main():
    check = "--check" in sys.argv[1:]
    unknown = [a for a in sys.argv[1:] if a != "--check"]
    if unknown:
        print("ui-gen-theme: unknown argument " + " ".join(unknown), file=sys.stderr)
        return 2
    text = generate()
    if check:
        if not os.path.isfile(THEME):
            print(f"ui-gen-theme: missing {THEME}", file=sys.stderr)
            return 1
        current = open(THEME, encoding="utf-8").read()
        if current != text:
            print("ui-gen-theme: theme.ch is stale; run scripts/ui-gen-theme.sh", file=sys.stderr)
            return 1
        print("ui-gen-theme: theme.ch matches tokens.json")
        return 0
    os.makedirs(os.path.dirname(THEME), exist_ok=True)
    with open(THEME, "w", encoding="utf-8") as f:
        f.write(text)
    print(f"ui-gen-theme: wrote {THEME}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
