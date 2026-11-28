// P1.5 theme values, setter/custom overrides, style resolution and state precedence.
using std::Option;
using std::Result;
using std::string_view;
using ui_internal::style_resolve;
using ui_internal::style_resolve_state;
using ui_internal::style_id;
using ui_internal::style_store_size;
using ui_internal::style_store_overflow;
using ui_internal::state_flags_none;
using ui_internal::state_styles_empty;
using ui_internal::StyleStateFlags;
using ui_internal::StyleStateSet;
using ui_internal::ResolvedStyle;
using ui_internal::breakpoint_for_width;
using ui_internal::control_height;
using ui_internal::table_row_height;
using ui_internal::table_header_height;
using ui_internal::page_padding;
using ui_internal::sidebar_width;
using ui_internal::focus_width;
using ui_internal::focus_offset;
using ui_internal::border_width;
using ui_internal::min_touch_target;
using ui_internal::preferred_touch_target;

public namespace style_tests {

func close(a : float, b : float) : bool {
    var d = a - b
    if(d < 0.0f) { d = 0.0f - d }
    return d < 0.0001f
}
func fail(name : *char) : bool {
    unsafe { printf("style FAIL %s\n", name) }
    return false
}
func color_eq(a : Color, b : Color) : bool { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a }
func dim_px(d : Dim) : float { switch(d) { Px(v) => return v; default => return 0.0f } }
func expect_color(name : *char, got : Color, want : Color) : bool {
    if(got.r != want.r || got.g != want.g || got.b != want.b || got.a != want.a) { return fail(name) }
    return true
}
func theme_features() : bool {
    var light = Theme.light()
    var body = light.text(TextRole.Body)
    if(!close(body.size, 14.0f) || !close(body.line_height, 20.0f / 14.0f) || body.weight != FontWeight.Regular) { return fail("body-metrics") }
    if(body.italic || body.features.ligatures || body.features.tabular_numbers || body.features.slashed_zero || body.features.stylistic_set != 0u8) { return fail("body-features") }
    if(body.tab_size != 4u8 || body.family != FontFamily.Ui || body.family_name.size() == 0u) { return fail("body-family-tab") }
    var code = light.text(TextRole.Code)
    if(code.family != FontFamily.Mono || code.size != 13.0f || code.line_height != 20.0f / 13.0f || code.tab_size != 4u8 || code.family_name.size() == 0u) { return fail("code-style") }
    var h1 = light.text(TextRole.H1)
    if(h1.weight != FontWeight.Bold || !close(h1.letter_spacing, -0.01f)) { return fail("h1-style") }
    var cap = light.text(TextRole.Caption)
    if(!close(cap.letter_spacing, 0.01f) || cap.weight != FontWeight.Regular) { return fail("caption-style") }
    return true
}
func theme_setters() : bool {
    var t = Theme.light()
    var custom_color = Color { r : 1u8, g : 2u8, b : 3u8, a : 255u8 }
    t.set_color(ColorToken.Surface, custom_color)
    if(!expect_color("set-color", t.color(ColorToken.Surface), custom_color)) { return false }
    t.set_space(Space.SM, 9.0f)
    if(!close(t.space(Space.SM), 9.0f)) { return fail("set-space") }
    t.set_density(Density.Compact)
    if(t.density() != Density.Compact || !close(t.space(Space.SM), 9.0f)) { return fail("set-density-space") }
    t.set_radius(Radius.MD, 7.0f)
    if(!close(t.radius(Radius.MD), 7.0f)) { return fail("set-radius") }
    var features = TextFeatures { ligatures : true, tabular_numbers : true, slashed_zero : true, stylistic_set : 3u8, raw_tags : zeroed<[0]u32>() }
    var custom = TextStyle { family : FontFamily.Mono, family_name : "custom-font", size : 18.0f, weight : FontWeight.Bold, italic : true, line_height : 1.5f, letter_spacing : 0.02f, features : features, tab_size : 2u8 }
    t.set_text_style(TextRole.Body, custom)
    var got = t.text(TextRole.Body)
    if(got.family != FontFamily.Mono || !got.family_name.equals_with_len("custom-font", 11u) || !close(got.size, 18.0f) || !close(got.line_height, 1.5f) || got.weight != FontWeight.Bold || !got.italic || !close(got.letter_spacing, 0.02f) || !got.features.ligatures || got.features.stylistic_set != 3u8 || got.tab_size != 2u8) { return fail("set-text-style") }
    t.set_text_scale(2.0f)
    got = t.text(TextRole.Body)
    if(!close(t.text_scale(), 2.0f) || !close(got.size, 36.0f) || !close(got.size * got.line_height, 54.0f) || !close(got.line_height, 1.5f) || got.tab_size != 2u8) { return fail("text-scale") }
    if(!close(t.disabled_opacity(), 0.4f)) { return fail("disabled-opacity") }
    if(!close(t.motion(MotionToken.Fast).ms, 120.0f) || !close(t.motion(MotionToken.Base).ms, 160.0f) || !close(t.motion(MotionToken.Slow).ms, 240.0f) || !close(t.motion(MotionToken.Spring).ms, 220.0f)) { return fail("motion") }
    var d = Theme.light()
    d.set_density(Density.Compact)
    if(!close(d.space(Space.SM), 6.0f) || d.density() != Density.Compact) { return fail("default-compact") }
    d.set_density(Density.Comfortable)
    if(!close(d.space(Space.SM), 8.0f)) { return fail("comfortable") }
    return true
}
func theme_helpers() : bool {
    if(breakpoint_for_width(639u32) != Breakpoint.Sm || breakpoint_for_width(640u32) != Breakpoint.Md || breakpoint_for_width(1024u32) != Breakpoint.Lg || breakpoint_for_width(1440u32) != Breakpoint.Xl) { return fail("breakpoints") }
    if(control_height(Density.Comfortable, 0u8) != 32u32 || control_height(Density.Comfortable, 1u8) != 36u32 || control_height(Density.Comfortable, 2u8) != 40u32) { return fail("comfy-controls") }
    if(control_height(Density.Compact, 0u8) != 28u32 || control_height(Density.Compact, 1u8) != 32u32 || control_height(Density.Compact, 2u8) != 36u32) { return fail("compact-controls") }
    if(table_row_height(Density.Comfortable) != 36u32 || table_row_height(Density.Compact) != 32u32 || table_header_height(Density.Comfortable) != 40u32 || table_header_height(Density.Compact) != 36u32) { return fail("table-density") }
    if(page_padding(Breakpoint.Sm) != 16u32 || page_padding(Breakpoint.Md) != 24u32 || page_padding(Breakpoint.Lg) != 32u32 || page_padding(Breakpoint.Xl) != 32u32) { return fail("page-padding") }
    if(sidebar_width(Breakpoint.Lg) != 240u32 || sidebar_width(Breakpoint.Xl) != 280u32) { return fail("sidebar") }
    if(focus_width(ThemeMode.Light) != 2u32 || focus_width(ThemeMode.Dark) != 2u32 || focus_width(ThemeMode.HighContrast) != 3u32) { return fail("focus-width") }
    if(focus_offset(ThemeMode.Light) != 2u32 || focus_offset(ThemeMode.HighContrast) != 2u32) { return fail("focus-offset") }
    if(border_width(ThemeMode.Light) != 1u32 || border_width(ThemeMode.HighContrast) != 2u32) { return fail("border-width") }
    if(min_touch_target() != 24u32 || preferred_touch_target() != 44u32) { return fail("touch-target") }
    return true
}
func contrast_checks() : bool {
    var light = Theme.light()
    if(design::contrast(light.color(ColorToken.OnSurface), light.color(ColorToken.Surface)) < 4.5) { return fail("light-body-contrast") }
    if(design::contrast(light.color(ColorToken.OnSurfaceMuted), light.color(ColorToken.Surface)) < 4.5) { return fail("light-muted-contrast") }
    if(design::contrast(light.color(ColorToken.OnPrimary), light.color(ColorToken.Primary)) < 4.5) { return fail("light-primary-contrast") }
    if(design::contrast(light.color(ColorToken.FocusRing), light.color(ColorToken.Surface)) < 3.0) { return fail("light-focus-contrast") }
    var dark = Theme.dark()
    if(design::contrast(dark.color(ColorToken.OnSurface), dark.color(ColorToken.Surface)) < 4.5) { return fail("dark-body-contrast") }
    if(design::contrast(dark.color(ColorToken.OnSurfaceMuted), dark.color(ColorToken.Surface)) < 4.5) { return fail("dark-muted-contrast") }
    if(design::contrast(dark.color(ColorToken.FocusRing), dark.color(ColorToken.Surface)) < 3.0) { return fail("dark-focus-contrast") }
    var hc = Theme.high_contrast()
    if(design::contrast(hc.color(ColorToken.OnSurface), hc.color(ColorToken.Surface)) < 4.5) { return fail("hc-body-contrast") }
    if(design::contrast(hc.color(ColorToken.FocusRing), hc.color(ColorToken.Surface)) < 3.0) { return fail("hc-focus-contrast") }
    return true
}
func run_theme() : bool {
    if(!design::run()) { return false }
    if(!theme_features()) { return false }
    if(!theme_setters()) { return false }
    if(!theme_helpers()) { return false }
    if(!contrast_checks()) { return false }
    return true
}

func full_base(t : &Theme) : Style {
    return Style.new().padding(Insets.all(1.0f)).gap(Space.XS).width(Dim.Px(11.0f)).height(Dim.Px(12.0f)).min_width(2.0f).max_width(99.0f).grow(0.5f).shrink(0.25f).basis(Dim.Percent(40.0f)).align_items(Align.Start).justify(Justify.Start).align_self(Align.Start).background(ColorToken.Surface).foreground(ColorToken.OnSurface).border(Border { width : 1.0f, color : ColorToken.Border }).radius(Radius.SM).shadow(Elevation.Low).opacity(0.9f).clip(false).overflow(Overflow.Visible).text(TextRole.Body).weight(FontWeight.Regular).cursor(Cursor.Arrow).z(1i32).aspect(1.0f).position(Position.Relative()).inset(Insets.all(0.0f)).grid_column(1u16, 1u16).grid_row(1u16, 1u16).direction(Direction.Ltr).text_style(t.text(TextRole.Body)).text_align(TextAlign.Start).wrap(WrapMode.NoWrap).max_lines(1u16).text_overflow(TextOverflow.Clip).tab_size(4u8)
}
func check_merge() : bool {
    var t = Theme.light()
    var base = full_base(&t)
    var over = Style.new().padding_all(Space.LG).gap(Space.MD).width(Dim.Px(21.0f)).height(Dim.Px(22.0f)).min_width(3.0f).max_width(88.0f).grow(1.0f).shrink(0.5f).basis(Dim.Px(50.0f)).align_items(Align.Center).justify(Justify.End).align_self(Align.Stretch).background(ColorToken.Primary).foreground(ColorToken.OnPrimary).border(Border { width : 2.0f, color : ColorToken.Danger }).radius(Radius.LG).shadow(Elevation.High).opacity(0.7f).clip(true).overflow(Overflow.Scroll).text(TextRole.H1).weight(FontWeight.Bold).cursor(Cursor.Hand).z(2i32).aspect(2.0f).position(Position.Relative()).inset(Insets.all(4.0f)).grid_column(2u16, 3u16).grid_row(4u16, 5u16).direction(Direction.Rtl).text_style(t.text(TextRole.H1)).text_align(TextAlign.Center).wrap(WrapMode.Wrap).max_lines(3u16).text_overflow(TextOverflow.Ellipsis).tab_size(2u8)
    var r = style_resolve(base.merge(over), &t)
    if(!close(r.padding.left, 16.0f) || !close(r.gap, 12.0f)) { return fail("merge-space") }
    if(!close(dim_px(r.width), 21.0f) || !close(dim_px(r.height), 22.0f) || !close(r.min_width, 3.0f) || !close(r.max_width, 88.0f) || !close(r.grow, 1.0f) || !close(r.shrink, 0.5f) || !close(dim_px(r.basis), 50.0f)) { return fail("merge-layout") }
    if(r.align_items != Align.Center || r.justify != Justify.End || r.align_self != Align.Stretch || !close(r.inset.left, 4.0f) || r.grid_col != 2u16 || r.grid_col_span != 3u16 || r.grid_row != 4u16 || r.grid_row_span != 5u16) { return fail("merge-box") }
    if(!expect_color("merge-bg", r.background, t.color(ColorToken.Primary)) || !expect_color("merge-fg", r.foreground, t.color(ColorToken.OnPrimary))) { return false }
    if(!close(r.border_width, 2.0f) || !expect_color("merge-border", r.border_color, t.color(ColorToken.Danger))) { return false }
    if(!close(r.radius, 8.0f) || !close(r.shadow.blur, 24.0f) || !close(r.opacity, 0.7f) || !r.clip || r.overflow != Overflow.Scroll) { return fail("merge-paint") }
    if(r.text_role != TextRole.H1 || r.text_weight != FontWeight.Bold || !close(r.text.size, 32.0f) || r.text_align != TextAlign.Center || r.wrap != WrapMode.Wrap || r.max_lines != 3u16 || r.text_overflow != TextOverflow.Ellipsis || r.tab_size != 2u8) { return fail("merge-text") }
    if(r.cursor != Cursor.Hand || r.z != 2i32 || !close(r.aspect, 2.0f) || r.direction != Direction.Rtl) { return fail("merge-interaction") }
    // An unset field inherits the base; only gap changes.
    var partial = base.merge(Style.new().gap(Space.LG))
    var p = style_resolve(partial, &t)
    if(!close(p.gap, 16.0f) || !close(dim_px(p.width), 11.0f) || !color_eq(p.background, t.color(ColorToken.Surface)) || p.text_role != TextRole.Body) { return fail("merge-inherit") }
    // Compact density changes token-backed padding/gap without rebuilding the style.
    var compact = Theme.light(); compact.set_density(Density.Compact)
    var c = style_resolve(Style.new().padding_all(Space.SM).gap(Space.SM), &compact)
    if(!close(c.padding.left, 6.0f) || !close(c.gap, 6.0f)) { return fail("compact-resolve") }
    return true
}
func check_state() : bool {
    var t = Theme.light()
    var base = Style.new().background(ColorToken.Surface).opacity(0.8f)
    var states = state_styles_empty()
    states.hover = Style.new().background(ColorToken.PrimaryHover)
    states.focus = Style.new().background(ColorToken.Primary)
    states.pressed = Style.new().background(ColorToken.PrimaryPressed)
    states.selected = Style.new().background(ColorToken.Selection)
    states.checked = Style.new().background(ColorToken.Primary)
    states.invalid = Style.new().background(ColorToken.Danger)
    states.disabled = Style.new().background(ColorToken.SurfaceSunken)
    var f = state_flags_none()
    f.hover = true; var h = style_resolve_state(base, states, f, &t)
    if(!color_eq(h.background, t.color(ColorToken.PrimaryHover))) { return fail("state-hover") }
    f = state_flags_none(); f.focus = true; var fo = style_resolve_state(base, states, f, &t)
    if(!color_eq(fo.background, t.color(ColorToken.Primary))) { return fail("state-focus") }
    f = state_flags_none(); f.pressed = true; f.focus = true; var pr = style_resolve_state(base, states, f, &t)
    if(!color_eq(pr.background, t.color(ColorToken.PrimaryPressed))) { return fail("state-precedence-pressed") }
    f = state_flags_none(); f.invalid = true; f.checked = true; var iv = style_resolve_state(base, states, f, &t)
    if(!color_eq(iv.background, t.color(ColorToken.Danger))) { return fail("state-precedence-invalid") }
    f = state_flags_none(); f.hover = true; f.focus = true; f.pressed = true; f.selected = true; f.checked = true; f.invalid = true; f.disabled = true
    var all = style_resolve_state(base, states, f, &t)
    if(!color_eq(all.background, t.color(ColorToken.SurfaceSunken)) || !close(all.opacity, 0.8f * 0.4f)) { return fail("state-disabled") }
    f = state_flags_none(); f.disabled = true; var dis = style_resolve_state(base, states, f, &t)
    if(!close(dis.opacity, 0.8f * 0.4f)) { return fail("state-disabled-token") }
    return true
}
func check_default_and_compact() : bool {
    var t = Theme.light()
    var d = Style.new()
    if(style_id(d) != 0u32 || style_id(d.merge(Style.new())) != 0u32) { return fail("default-fast") }
    var r = style_resolve(d, &t)
    if(!color_eq(r.background, t.color(ColorToken.Transparent)) || r.text_role != TextRole.Body || !close(r.opacity, 1.0f) || !close(r.padding.left, 0.0f) || !close(r.radius, 0.0f)) { return fail("default-resolve") }
    if(style_store_size() > 512u32) { return fail("intern-bound") }
    return true
}
func check_intern_bound() : bool {
    var i : u32 = 0u32
    while(i < 600u32) {
        var s = Style.new().z(i as i32)
        var id = style_id(s)
        if(id > 512u32) { return fail("intern-id") }
        i = i + 1u32
    }
    if(style_store_size() > 512u32) { return fail("intern-size") }
    if(style_store_overflow() == 0u32) { return fail("intern-overflow") }
    return true
}
public func run_style() : bool {
    if(!check_merge()) { return false }
    if(!check_state()) { return false }
    if(!check_default_and_compact()) { return false }
    if(!check_intern_bound()) { return false }
    return true
}
}
