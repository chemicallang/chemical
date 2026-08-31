/**
 * Shared CSS AST -> CSS text conversion.
 *
 * All conversion functions take a `*mut CssEmitter` parameter instead of
 * methods on a struct, to avoid interface-pointer-in-struct issues.
 */

using namespace std;

public func (str : &std::string) view() : std::string_view {
    return std::string_view(str.data(), str.size());
}

// ─── Standalone helper functions ─────────────────────────────────────────────

public func css_write_unit_of_kind(str : &mut std::string, kind : CSSLengthKind) : bool {
    switch(kind) {
        CSSLengthKind.LengthPX => { str.append_view("px") }
        CSSLengthKind.LengthEM => { str.append_view("em") }
        CSSLengthKind.LengthREM => { str.append_view("rem") }
        CSSLengthKind.LengthVH => { str.append_view("vh") }
        CSSLengthKind.LengthVW => { str.append_view("vw") }
        CSSLengthKind.LengthVMIN => { str.append_view("vmin") }
        CSSLengthKind.LengthVMAX => { str.append_view("vmax") }
        CSSLengthKind.LengthPERCENTAGE => { str.append('%') }
        CSSLengthKind.LengthCM => { str.append_view("cm") }
        CSSLengthKind.LengthMM => { str.append_view("mm") }
        CSSLengthKind.LengthIN => { str.append_view("in") }
        CSSLengthKind.LengthPT => { str.append_view("pt") }
        CSSLengthKind.LengthPC => { str.append_view("pc") }
        CSSLengthKind.LengthCH => { str.append_view("ch") }
        CSSLengthKind.LengthEX => { str.append_view("ex") }
        CSSLengthKind.LengthS => { str.append('s') }
        CSSLengthKind.LengthMS => { str.append_view("ms") }
        CSSLengthKind.LengthHZ => { str.append_view("hz") }
        CSSLengthKind.LengthKHZ => { str.append_view("khz") }
        CSSLengthKind.LengthDEG => { str.append_view("deg") }
        CSSLengthKind.LengthRAD => { str.append_view("rad") }
        CSSLengthKind.LengthGRAD => { str.append_view("grad") }
        CSSLengthKind.LengthTURN => { str.append_view("turn") }
        CSSLengthKind.LengthFR => { str.append_view("fr") }
        default => { return false }
    }
    return true
}

public func css_write_length(ptr : &mut CSSLengthValueData, str : &mut std::string) {
    if(ptr.kind == CSSLengthKind.Variable) {
        str.append_view("var(")
        str.append_view(&ptr.value)
        str.append(')')
        return
    }
    str.append_view(&ptr.value)
    if(ptr.kind != CSSLengthKind.None && !css_write_unit_of_kind(str, ptr.kind)) {
        printf("unknown unit")
        fflush(null)
    }
}

public func css_write_length_or_none(len : &mut CSSLengthValueData, str : &mut std::string) {
    switch(len.kind) {
        CSSLengthKind.Unknown => {}
        CSSLengthKind.None => {
            if(len.value.empty()) { str.append_view("none") } else { css_write_length(len, str) }
        }
        default => { css_write_length(len, str) }
    }
}

public func css_write_length_or_none_not_first(len : &mut CSSLengthValueData, str : &mut std::string) {
    if(len.kind != CSSLengthKind.Unknown) { str.append(' '); css_write_length_or_none(len, str) }
}

public func css_write_alpha_length_or_none(len : &mut CSSLengthValueData, str : &mut std::string) {
    if(len.kind != CSSLengthKind.Unknown) { str.append(' '); str.append('/'); str.append(' '); css_write_length_or_none(len, str) }
}

public func css_write_rgb_data(ptr : &mut CSSRGBColorData, str : &mut std::string) {
    css_write_length_or_none(&mut ptr.red, str)
    css_write_length_or_none_not_first(&mut ptr.green, str)
    css_write_length_or_none_not_first(&mut ptr.blue, str)
    css_write_alpha_length_or_none(&mut ptr.alpha, str)
}

public func css_write_hsl_data(ptr : &mut CSSHSLColorData, str : &mut std::string) {
    css_write_length_or_none(&mut ptr.hue, str)
    css_write_length_or_none_not_first(&mut ptr.saturation, str)
    css_write_length_or_none_not_first(&mut ptr.lightness, str)
    css_write_alpha_length_or_none(&mut ptr.alpha, str)
}

public func css_write_hwb_data(ptr : &mut CSSHWBColorData, str : &mut std::string) {
    css_write_length_or_none(&mut ptr.hue, str)
    css_write_length_or_none_not_first(&mut ptr.whiteness, str)
    css_write_length_or_none_not_first(&mut ptr.blackness, str)
    css_write_alpha_length_or_none(&mut ptr.alpha, str)
}

public func css_write_lab_data(ptr : &mut CSSLABColorData, str : &mut std::string) {
    css_write_length_or_none(&mut ptr.lightness, str)
    css_write_length_or_none_not_first(&mut ptr.rgAxis, str)
    css_write_length_or_none_not_first(&mut ptr.byAxis, str)
    css_write_alpha_length_or_none(&mut ptr.alpha, str)
}

public func css_write_lch_data(ptr : &mut CSSLCHColorData, str : &mut std::string) {
    css_write_length_or_none(&mut ptr.lightness, str)
    css_write_length_or_none_not_first(&mut ptr.chroma, str)
    css_write_length_or_none_not_first(&mut ptr.hue, str)
    css_write_alpha_length_or_none(&mut ptr.alpha, str)
}

public func css_write_oklab_data(ptr : &mut CSSOKLABColorData, str : &mut std::string) {
    css_write_length_or_none(&mut ptr.lightness, str)
    css_write_length_or_none_not_first(&mut ptr.aAxis, str)
    css_write_length_or_none_not_first(&mut ptr.bAxis, str)
    css_write_alpha_length_or_none(&mut ptr.alpha, str)
}

public func css_write_oklch_data(ptr : &mut CSSOKLCHColorData, str : &mut std::string) {
    css_write_length_or_none(&mut ptr.lightness, str)
    css_write_length_or_none_not_first(&mut ptr.pChroma, str)
    css_write_length_or_none_not_first(&mut ptr.hue, str)
    css_write_alpha_length_or_none(&mut ptr.alpha, str)
}

public func css_write_color(ptr : &mut CSSColorValueData, str : &mut std::string) {
    switch(ptr.kind) {
        CSSColorKind.RGB => { str.append_view("rgb("); css_write_rgb_data(&mut *ptr.value.rgbData, str); str.append(')') }
        CSSColorKind.RGBA => { str.append_view("rgba("); css_write_rgb_data(&mut *ptr.value.rgbData, str); str.append(')') }
        CSSColorKind.HSL => { str.append_view("hsl("); css_write_hsl_data(&mut *ptr.value.hslData, str); str.append(')') }
        CSSColorKind.HSLA => { str.append_view("hsla("); css_write_hsl_data(&mut *ptr.value.hslData, str); str.append(')') }
        CSSColorKind.HWB => { str.append_view("hwb("); css_write_hwb_data(&mut *ptr.value.hwbData, str); str.append(')') }
        CSSColorKind.LAB => { str.append_view("lab("); css_write_lab_data(&mut *ptr.value.labData, str); str.append(')') }
        CSSColorKind.LCH => { str.append_view("lch("); css_write_lch_data(&mut *ptr.value.lchData, str); str.append(')') }
        CSSColorKind.OKLAB => { str.append_view("oklab("); css_write_oklab_data(&mut *ptr.value.oklabData, str); str.append(')') }
        CSSColorKind.OKLCH => { str.append_view("oklch("); css_write_oklch_data(&mut *ptr.value.oklchData, str); str.append(')') }
        CSSColorKind.VAR => { str.append_view("var("); str.append_view(&ptr.value.view); str.append(')') }
        CSSColorKind.Unknown => { return }
        default => { str.append_view(&ptr.value.view) }
    }
}

public func css_write_font_style(ptr : &CSSFontStyle, str : &mut std::string) {
    switch(ptr) {
        None => {}
        Keyword(keyword) => { str.append_view(&keyword.value) }
        Oblique(view) => { str.append_view(&view) }
    }
}

public func css_write_font_weight(ptr : &CSSFontWeight, str : &mut std::string) {
    switch(ptr) {
        None => {}
        Keyword(keyword) => { str.append(' '); str.append_view(&keyword.value) }
        Absolute(view) => { str.append(' '); str.append_view(&view) }
    }
}

public func css_write_font_family_data(family : &mut CSSFontFamily, str : &mut std::string) {
    const first = family.families.data()
    var start = first
    const end = start + family.families.size()
    while(start != end) {
        if(start != first) { str.append(','); }
        str.append_view(&*start)
        start++
    }
}

public func css_write_linear_easing(ptr : &mut CSSLinearEasingPoint, str : &mut std::string) {
    var has_value_before = false
    if(ptr.point.kind != CSSLengthKind.Unknown) { css_write_length(&mut ptr.point, str); has_value_before = true }
    if(ptr.start.kind != CSSLengthKind.Unknown) { if(has_value_before) { str.append(' '); } else { has_value_before = true }; css_write_length(&mut ptr.start, str) }
    if(ptr.stop.kind != CSSLengthKind.Unknown) { if(has_value_before) { str.append(' '); } else { has_value_before = true }; css_write_length(&mut ptr.stop, str) }
    if(ptr.next != null) { str.append(','); str.append(' '); css_write_linear_easing(&mut *ptr.next, str) }
}

public func css_write_cubic_bezier_easing(ptr : &mut CSSCubicBezierEasingData, str : &mut std::string) {
    css_write_length(&mut ptr.x1, str); str.append(',')
    css_write_length(&mut ptr.y1, str); str.append(',')
    css_write_length(&mut ptr.x2, str); str.append(',')
    css_write_length(&mut ptr.y2, str)
}

public func css_write_steps_easing(ptr : &mut CSSStepsEasingData, str : &mut std::string) {
    css_write_length(&mut ptr.step, str); str.append(','); str.append_view(&ptr.position.value)
}

public func css_write_easing(ptr : &mut CSSEasingFunction, str : &mut std::string) {
    switch(ptr.kind) {
        CSSKeywordKind.Var => { str.append_view("var("); str.append_view(&ptr.data.keyword.value); str.append(')') }
        CSSKeywordKind.Ease, CSSKeywordKind.EaseIn, CSSKeywordKind.EaseOut,
        CSSKeywordKind.EaseInOut, CSSKeywordKind.StepStart, CSSKeywordKind.StepEnd => { str.append_view(&ptr.data.keyword.value) }
        CSSKeywordKind.Linear => {
            str.append_view("linear")
            if(ptr.data.linear != null) { str.append('('); css_write_linear_easing(&mut *ptr.data.linear, str); str.append(')') }
        }
        CSSKeywordKind.CubicBezier => { str.append_view("cubic-bezier("); css_write_cubic_bezier_easing(&mut *ptr.data.bezier, str); str.append(')') }
        CSSKeywordKind.Steps => { str.append_view("steps("); css_write_steps_easing(&mut *ptr.data.steps, str); str.append(')') }
    }
}

public func css_write_transition(ptr : &mut CSSTransitionValueData, str : &mut std::string) {
    var has_value_before = false
    if(!ptr.property.empty()) { str.append_view(&ptr.property); has_value_before = true }
    if(ptr.duration.kind != CSSLengthKind.Unknown) { if(has_value_before) { str.append(' '); } else { has_value_before = true }; css_write_length(&mut ptr.duration, str) }
    if(ptr.easing.kind != CSSKeywordKind.Unknown) { if(has_value_before) { str.append(' '); } else { has_value_before = true }; css_write_easing(&mut ptr.easing, str) }
    if(ptr.delay.kind != CSSLengthKind.Unknown) { if(has_value_before) { str.append(' '); } else { has_value_before = true }; css_write_length(&mut ptr.delay, str) }
    if(ptr.behavior.kind != CSSLengthKind.Unknown) { if(has_value_before) { str.append(' '); } else { has_value_before = true }; str.append_view(&ptr.behavior.value) }
    if(ptr.next != null) { str.append(','); css_write_transition(&mut *ptr.next, str) }
}

public func css_append_hex(str : &mut std::string, val : uint) {
    const hex = "0123456789ABCDEF"
    if (val == 0) { str.append('0'); return }
    var buf : [16]char
    var bi = 0
    while(val > 0) { buf[bi++] = hex[val & 0xF]; val >>= 4 }
    while(bi > 0) { str.append(buf[--bi]) }
}

public func css_write_css_string_to_buffer(text : std::string_view, str : &mut std::string) {
    var i = 0u
    while(i < text.size()) {
        const c1 = (text.data()[i] as uint) & 0xFF
        if (c1 < 0x80) {
            const c = c1 as char
            if (c == '\'' || c == '\\') { str.append('\\'); }
            str.append(c)
            i++
        } else if ((c1 & 0xE0) == 0xC0) {
            if (i + 1 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF
                const codepoint = ((c1 & 0x1F) << 6) | (c2 & 0x3F)
                str.append('\\'); css_append_hex(str, codepoint)
                i += 2
                if(i < text.size()) str.append(' ')
            } else { i++ }
        } else if ((c1 & 0xF0) == 0xE0) {
            if (i + 2 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF
                const c3 = (text.data()[i+2] as uint) & 0xFF
                const codepoint = ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F)
                str.append('\\'); css_append_hex(str, codepoint)
                i += 3
                if(i < text.size()) str.append(' ')
            } else { i++ }
        } else if ((c1 & 0xF8) == 0xF0) {
            if (i + 3 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF
                const c3 = (text.data()[i+2] as uint) & 0xFF
                const c4 = (text.data()[i+3] as uint) & 0xFF
                const codepoint = ((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F)
                str.append('\\'); css_append_hex(str, codepoint)
                i += 4
                if(i < text.size()) str.append(' ')
            } else { i++ }
        } else { i++ }
    }
}

// ─── Selector serialization ──────────────────────────────────────────────────

public func css_has_ampersand_simple(s : *mut SimpleSelector) : bool { return s.kind == SimpleSelectorKind.Ampersand }

public func css_has_ampersand_compound(c : *mut CompoundSelector) : bool {
    var i : uint = 0
    while(i < c.simple_selectors.size()) {
        if(css_has_ampersand_simple(c.simple_selectors.get(i))) return true
        i++
    }
    return false
}

public func css_has_ampersand_complex(c : *mut ComplexSelector) : bool {
    if(css_has_ampersand_compound(c.compound)) return true
    if(c.next != null) return css_has_ampersand_complex(c.next)
    return false
}

public func css_serialize_simple(s : *mut SimpleSelector, out : &mut std::string, replacement : std::string_view) {
    if(s.kind == SimpleSelectorKind.Ampersand) { out.append_view(&replacement); return }
    if(s.kind == SimpleSelectorKind.Attribute) {
        out.append('['); out.append_view(&s.attr.name)
        if(!s.attr.operator.empty()) {
            out.append_view(&s.attr.operator); out.append('"'); out.append_view(&s.attr.value); out.append('"')
            if(!s.attr.flags.empty()) { out.append(' '); out.append_view(&s.attr.flags) }
        }
        out.append(']'); return
    }
    if(s.kind == SimpleSelectorKind.Class) out.append('.')
    if(s.kind == SimpleSelectorKind.Id) out.append('#')
    if(s.kind == SimpleSelectorKind.PseudoClass) { out.append(':'); }
    if(s.kind == SimpleSelectorKind.PseudoElement) { out.append_view("::"); }
    out.append_view(&s.value)
}

public func css_serialize_compound(c : *mut CompoundSelector, out : &mut std::string, replacement : std::string_view) {
    var i : uint = 0
    while(i < c.simple_selectors.size()) { css_serialize_simple(c.simple_selectors.get(i), out, replacement); i++ }
}

public func css_serialize_complex(c : *mut ComplexSelector, out : &mut std::string, replacement : std::string_view) {
    css_serialize_compound(c.compound, out, replacement)
    if(c.next != null) {
        if(c.combinator == Combinator.Descendant) out.append(' ')
        else if(c.combinator == Combinator.Child) out.append_view(" > ")
        else if(c.combinator == Combinator.NextSibling) out.append_view(" + ")
        else if(c.combinator == Combinator.SubsequentSibling) out.append_view(" ~ ")
        css_serialize_complex(c.next, out, replacement)
    }
}

public func css_append_media_root_selector(out : &mut std::string, className : std::string_view) {
    if(className.empty()) return
    if(className.data()[0] == '.') { out.append_view(&className) } else { out.append('.'); out.append_view(&className) }
}

// ─── WriteCssValueToBuffer — main value conversion ──────────────────────────

public func writeCssValueToBuffer(value : &mut CSSValue, str : &mut std::string, emitter : *mut CssEmitter) {
    switch(value.kind) {
        CSSValueKind.Multiple => {
            const ptr = value.data as *mut CSSMultipleValues
            var i : uint = 0; const size = ptr.values.size(); const last = size - 1
            while(i < size) { writeCssValueToBuffer(&mut *ptr.values.get_ptr(i), str, emitter); if(i < last) { str.append(' '); } i++ }
        }
        CSSValueKind.Pair => {
            const pair = value.data as *mut CSSValuePair
            writeCssValueToBuffer(&mut pair.first, str, emitter)
            if(pair.second.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut pair.second, str, emitter) }
        }
        CSSValueKind.Keyword => { var ptr = value.data as *mut CSSKeywordValueData; str.append_view(&ptr.value) }
        CSSValueKind.Length => { const ptr = value.data as *mut CSSLengthValueData; css_write_length(&mut *ptr, str) }
        CSSValueKind.Color => { var ptr = value.data as *mut CSSColorValueData; css_write_color(&mut *ptr, str) }
        CSSValueKind.SingleLengthFunctionCall => {
            const ptr = value.data as *mut SingleLengthFuncCall
            str.append_view(&ptr.name.value); str.append('('); css_write_length(&mut ptr.length, str); str.append(')')
        }
        CSSValueKind.Font => { var ptr = value.data as *mut CSSFontValueData; css_write_font_value_data(&*ptr, str, emitter) }
        CSSValueKind.FontFamily => { var ptr = value.data as *mut CSSFontFamily; css_write_font_family_data(&mut *ptr, str) }
        CSSValueKind.Border => {
            const ptr = value.data as *mut CSSBorderValueData
            const has_style = ptr.style.kind != CSSValueKind.Unknown; const has_color = ptr.color.kind != CSSValueKind.Unknown
            if(ptr.width.kind != CSSValueKind.Unknown) { writeCssValueToBuffer(&mut ptr.width, str, emitter); if(has_style) { str.append(' ') } }
            if(has_style) { writeCssValueToBuffer(&mut ptr.style, str, emitter); if(has_color) { str.append(' ') } }
            if(has_color) { writeCssValueToBuffer(&mut ptr.color, str, emitter) }
        }
        CSSValueKind.BoxShadow => {
            const ptr = value.data as *mut CSSBoxShadowValueData
            if(ptr.isEmpty()) { str.append_view("none") } else { css_write_box_shadow_value_data(&mut *ptr, str, emitter) }
        }
        CSSValueKind.TextShadow => {
            const ptr = value.data as *mut CSSTextShadowValueData
            if(ptr.isEmpty()) { str.append_view("none") } else { css_write_text_shadow_value_data(&mut *ptr, str, emitter) }
        }
        CSSValueKind.Transform => {
            const ptr = value.data as *mut CSSTransformValueData
            if(ptr.node == null && ptr.transformFunction.kind != CSSKeywordKind.Perspective) { str.append_view("none") } else { css_write_transform_value_data(&mut *ptr, str, emitter) }
        }
        CSSValueKind.BorderRadius => { const ptr = value.data as *mut CSSBorderRadiusValueData; css_write_border_radius_value_data(&mut *ptr, str, emitter) }
        CSSValueKind.Transition => { const ptr = value.data as *mut CSSTransitionValueData; css_write_transition(&mut *ptr, str) }
        CSSValueKind.TransitionTimingFunction => { const ptr = value.data as *mut CSSEasingFunction; css_write_easing(&mut *ptr, str) }
        CSSValueKind.MultipleBackgroundImage => {
            const ptr = value.data as *mut MultipleBackgroundImageData
            var start = ptr.images.data(); const end = start + ptr.images.size()
            while(start != end) { css_write_background_image_data(&mut *start, str, emitter); start++ }
        }
        CSSValueKind.BackgroundImage => { const ptr = value.data as *mut BackgroundImageData; css_write_background_image_data(&mut *ptr, str, emitter) }
        CSSValueKind.Background => { const ptr = value.data as *mut CSSBackgroundValueData; css_write_background_value_data(&mut *ptr, str, emitter) }
        CSSValueKind.TextDecoration => {
            const ptr = value.data as *mut CSSTextDecorationValueData
            var first = true
            if(ptr.line.kind != CSSValueKind.Unknown) { writeCssValueToBuffer(&mut ptr.line, str, emitter); first = false }
            if(ptr.style.kind != CSSValueKind.Unknown) { if(!first) str.append(' '); writeCssValueToBuffer(&mut ptr.style, str, emitter); first = false }
            if(ptr.color.kind != CSSValueKind.Unknown) { if(!first) str.append(' '); writeCssValueToBuffer(&mut ptr.color, str, emitter); first = false }
            if(ptr.thickness.kind != CSSValueKind.Unknown) { if(!first) str.append(' '); writeCssValueToBuffer(&mut ptr.thickness, str, emitter) }
        }
        CSSValueKind.Outline => { var ptr = value.data as *mut CSSOutlineValueData; css_write_outline_value_data(&mut *ptr, str, emitter) }
        CSSValueKind.BackdropFilter => { var ptr = value.data as *mut CSSBackdropFilterValueData; css_write_backdrop_filter_value_data(&mut *ptr, str, emitter) }
        CSSValueKind.GridRepeat => {
            const ptr = value.data as *mut GridRepeatData
            str.append_view("repeat("); writeCssValueToBuffer(&mut ptr.count, str, emitter); str.append(','); str.append(' ')
            var i : uint = 0; const size = ptr.tracks.size(); const last = size - 1
            while(i < size) { writeCssValueToBuffer(&mut *ptr.tracks.get_ptr(i), str, emitter); if(i < last) { str.append(' '); } i++ }
            str.append(')')
        }
        CSSValueKind.GridLine => { const ptr = value.data as *mut GridLineData; if(ptr.is_span) { str.append_view("span "); } writeCssValueToBuffer(&mut ptr.value, str, emitter) }
        CSSValueKind.Calc => { const ptr = value.data as *mut CSSCalcValueData; str.append_view("calc("); css_write_calc_expression(&ptr.expression, str, emitter); str.append(')') }
        CSSValueKind.ChemicalValue => {
            emitter.flush()
            const ptr = value.data as *mut Value; emitter.emit_chemical_value(ptr)
        }
        CSSValueKind.String => { const ptr = value.data as *mut CSSStringValueData; str.append('\''); css_write_css_string_to_buffer(ptr.value, str); str.append('\'') }
        CSSValueKind.Raw => { const ptr = value.data as *mut CSSRawValueData; str.append_view(&ptr.value) }
        CSSValueKind.Animation => { const ptr = value.data as *mut CSSAnimationValueData; css_write_animation_value_data(&mut *ptr, str, emitter) }
        CSSValueKind.ListStyle => {
            const ptr = value.data as *mut CSSListStyleValueData
            var first = true
            if(ptr.type.kind != CSSKeywordKind.Unknown) { str.append_view(&ptr.type.value); first = false }
            if(ptr.position.kind != CSSKeywordKind.Unknown) { if(!first) str.append(' '); str.append_view(&ptr.position.value); first = false }
            if(ptr.image.kind != CSSValueKind.Unknown) { if(!first) str.append(' '); writeCssValueToBuffer(&mut ptr.image, str, emitter) }
        }
        CSSValueKind.Unknown => {}
        default => { printf("error no value found, kind %d\n", value.kind); fflush(null) }
    }
}

// ─── Compound value writers ──────────────────────────────────────────────────

public func css_write_border_radius_value_data(ptr : &mut CSSBorderRadiusValueData, str : &mut std::string, emitter : *mut CssEmitter) {
    if(ptr.first.kind != CSSValueKind.Unknown) { writeCssValueToBuffer(&mut ptr.first, str, emitter) }
    if(ptr.second.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut ptr.second, str, emitter) }
    if(ptr.third.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut ptr.third, str, emitter) }
    if(ptr.fourth.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut ptr.fourth, str, emitter) }
    if(ptr.next != null) { str.append_view(" / "); css_write_border_radius_value_data(&mut *ptr.next, str, emitter) }
}

public func css_write_font_value_data(ptr : &CSSFontValueData, str : &mut std::string, emitter : *mut CssEmitter) {
    css_write_font_style(&ptr.style, str)
    if(ptr.fontVariant.kind != CSSKeywordKind.Unknown) { str.append(' '); str.append_view(&ptr.fontVariant.value) }
    css_write_font_weight(&ptr.weight, str)
    if(ptr.stretch.kind != CSSKeywordKind.Unknown) { str.append(' '); str.append_view(&ptr.stretch.value) }
    if(ptr.size.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut ptr.size, str, emitter) }
    if(ptr.lineHeight.kind != CSSValueKind.Unknown) { str.append('/'); writeCssValueToBuffer(&mut ptr.lineHeight, str, emitter) }
    if(!ptr.family.families.empty()) { str.append(' '); }
    css_write_font_family_data(&mut ptr.family, str)
}

public func css_write_text_shadow_value_data(value : &mut CSSTextShadowValueData, str : &mut std::string, emitter : *mut CssEmitter) {
    if(value.offsetX.kind != CSSValueKind.Unknown) { writeCssValueToBuffer(&mut value.offsetX, str, emitter) }
    if(value.offsetY.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut value.offsetY, str, emitter) }
    if(value.blurRadius.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut value.blurRadius, str, emitter) }
    if(value.color.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut value.color, str, emitter) }
    if(value.next != null) { str.append(','); css_write_text_shadow_value_data(&mut *value.next, str, emitter) }
}

public func css_write_box_shadow_value_data(value : &mut CSSBoxShadowValueData, str : &mut std::string, emitter : *mut CssEmitter) {
    if(value.inset) { str.append_view("inset"); }
    if(value.offsetX.kind != CSSValueKind.Unknown) { if(value.inset) { str.append(' '); } writeCssValueToBuffer(&mut value.offsetX, str, emitter) }
    if(value.offsetY.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut value.offsetY, str, emitter) }
    if(value.blurRadius.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut value.blurRadius, str, emitter) }
    if(value.spreadRadius.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut value.spreadRadius, str, emitter) }
    if(value.color.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut value.color, str, emitter) }
    if(value.next != null) { str.append(','); css_write_box_shadow_value_data(&mut *value.next, str, emitter) }
}

public func css_write_transform_node(ptr : &mut CSSTransformLengthNode, str : &mut std::string, emitter : *mut CssEmitter) {
    writeCssValueToBuffer(&mut ptr.value, str, emitter)
    if(ptr.next != null && ptr.next.value.kind != CSSValueKind.Unknown) { str.append(','); css_write_transform_node(&mut *ptr.next, str, emitter) }
}

public func css_write_transform_value_data(ptr : &mut CSSTransformValueData, str : &mut std::string, emitter : *mut CssEmitter) {
    str.append_view(&ptr.transformFunction.value); str.append('(')
    if(ptr.node != null) { css_write_transform_node(&mut *ptr.node, str, emitter) } else { str.append_view("none") }
    str.append(')')
    if(ptr.next != null) { str.append(' '); css_write_transform_value_data(&mut *ptr.next, str, emitter) }
}

public func css_write_background_image_url(url : &mut UrlData, str : &mut std::string) {
    if(url.is_source) { str.append_view("src") } else { str.append_view("url") }
    str.append('('); str.append_view(&url.value); str.append(')');
}

public func css_write_linear_gradient_data(data : &mut LinearGradientData, str : &mut std::string, emitter : *mut CssEmitter) {
    css_write_length(&mut data.angle, str)
    if(data.angle.kind != CSSLengthKind.Unknown) { str.append(','); }
    if(data.to1.kind != CSSKeywordKind.Unknown) {
        str.append_view("to "); str.append_view(&data.to1.value)
        if(data.to2.kind != CSSKeywordKind.Unknown) { str.append(' '); str.append_view(&data.to2.value) }
        str.append(',');
    }
    var start = data.color_stop_list.data(); const end = start + data.color_stop_list.size()
    while(start != end) {
        writeCssValueToBuffer(&mut start.hint, str, emitter); writeCssValueToBuffer(&mut start.stop.color, str, emitter)
        if(start.stop.length.kind != CSSValueKind.Unknown) {
            str.append(' '); writeCssValueToBuffer(&mut start.stop.length, str, emitter)
            if(start.stop.optSecLength.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut start.stop.optSecLength, str, emitter) }
        }
        start++; if(start != end) str.append(',');
    }
}

public func css_write_radial_gradient_data(data : &mut RadialGradientData, str : &mut std::string, emitter : *mut CssEmitter) {
    var has_shape_or_size = false
    if(data.shape.kind != CSSKeywordKind.Unknown) { str.append_view(&data.shape.value); has_shape_or_size = true }
    if(data.size.extent.kind != CSSKeywordKind.Unknown) { if(has_shape_or_size) str.append(' '); str.append_view(&data.size.extent.value); has_shape_or_size = true }
    else if(data.size.length.kind != CSSValueKind.Unknown) { if(has_shape_or_size) str.append(' '); writeCssValueToBuffer(&mut data.size.length, str, emitter); has_shape_or_size = true }
    if(data.position.kind != CSSValueKind.Unknown) { if(has_shape_or_size) str.append(' '); str.append_view("at "); writeCssValueToBuffer(&mut data.position, str, emitter); has_shape_or_size = true }
    if(has_shape_or_size) { str.append(','); }
    var start = data.color_stop_list.data(); const end = start + data.color_stop_list.size()
    while(start != end) {
        writeCssValueToBuffer(&mut start.hint, str, emitter); writeCssValueToBuffer(&mut start.stop.color, str, emitter)
        if(start.stop.length.kind != CSSValueKind.Unknown) {
            str.append(' '); writeCssValueToBuffer(&mut start.stop.length, str, emitter)
            if(start.stop.optSecLength.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut start.stop.optSecLength, str, emitter) }
        }
        start++; if(start != end) str.append(',');
    }
}

public func css_write_conic_gradient_data(data : &mut ConicGradientData, str : &mut std::string, emitter : *mut CssEmitter) {
    var has_from_or_at = false
    if(data.from.kind != CSSValueKind.Unknown) { str.append_view("from "); writeCssValueToBuffer(&mut data.from, str, emitter); has_from_or_at = true }
    if(data.at.kind != CSSValueKind.Unknown) { if(has_from_or_at) str.append(' '); str.append_view("at "); writeCssValueToBuffer(&mut data.at, str, emitter); has_from_or_at = true }
    if(has_from_or_at) { str.append(','); }
    var start = data.color_stop_list.data(); const end = start + data.color_stop_list.size()
    while(start != end) {
        writeCssValueToBuffer(&mut start.hint, str, emitter); writeCssValueToBuffer(&mut start.stop.color, str, emitter)
        if(start.stop.length.kind != CSSValueKind.Unknown) {
            str.append(' '); writeCssValueToBuffer(&mut start.stop.length, str, emitter)
            if(start.stop.optSecLength.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut start.stop.optSecLength, str, emitter) }
        }
        start++; if(start != end) str.append(',');
    }
}

public func css_write_background_image_data(ptr : &mut BackgroundImageData, str : &mut std::string, emitter : *mut CssEmitter) {
    if(ptr.is_url) { css_write_background_image_url(&mut ptr.url, str) } else {
        switch(ptr.gradient.kind) {
            CSSGradientKind.RepeatingLinear => { str.append_view("repeating-linear-gradient("); css_write_linear_gradient_data(&mut *(ptr.gradient.data as *mut LinearGradientData), str, emitter); str.append(')') }
            CSSGradientKind.RepeatingRadial => { str.append_view("repeating-radial-gradient("); css_write_radial_gradient_data(&mut *(ptr.gradient.data as *mut RadialGradientData), str, emitter); str.append(')') }
            CSSGradientKind.RepeatingConic => { str.append_view("repeating-conic-gradient("); css_write_conic_gradient_data(&mut *(ptr.gradient.data as *mut ConicGradientData), str, emitter); str.append(')') }
            CSSGradientKind.Linear => { str.append_view("linear-gradient("); css_write_linear_gradient_data(&mut *(ptr.gradient.data as *mut LinearGradientData), str, emitter); str.append(')') }
            CSSGradientKind.Radial => { str.append_view("radial-gradient("); css_write_radial_gradient_data(&mut *(ptr.gradient.data as *mut RadialGradientData), str, emitter); str.append(')') }
            CSSGradientKind.Conic => { str.append_view("conic-gradient("); css_write_conic_gradient_data(&mut *(ptr.gradient.data as *mut ConicGradientData), str, emitter); str.append(')') }
            default => {}
        }
    }
}

public func css_write_background_value_data(ptr : &mut CSSBackgroundValueData, str : &mut std::string, emitter : *mut CssEmitter) {
    var start = ptr.layers.data(); const end = start + ptr.layers.size()
    while(start != end) {
        if(start != ptr.layers.data()) { str.append(','); str.append(' '); }
        var has_val = false
        if(start.image.kind != CSSValueKind.Unknown) { writeCssValueToBuffer(&mut start.image, str, emitter); has_val = true }
        if(start.positionX.kind != CSSValueKind.Unknown) {
            if(has_val) str.append(' '); writeCssValueToBuffer(&mut start.positionX, str, emitter)
            if(start.positionY.kind != CSSValueKind.Unknown) { str.append(' '); writeCssValueToBuffer(&mut start.positionY, str, emitter) }
            if(start.size.kind != CSSValueKind.Unknown) { str.append('/'); writeCssValueToBuffer(&mut start.size, str, emitter) }
            has_val = true
        }
        if(start.repeat.kind != CSSValueKind.Unknown) { if(has_val) str.append(' '); writeCssValueToBuffer(&mut start.repeat, str, emitter); has_val = true }
        if(start.attachment.kind != CSSValueKind.Unknown) { if(has_val) str.append(' '); writeCssValueToBuffer(&mut start.attachment, str, emitter); has_val = true }
        if(start.origin.kind != CSSValueKind.Unknown) { if(has_val) str.append(' '); writeCssValueToBuffer(&mut start.origin, str, emitter); has_val = true }
        if(start.clip.kind != CSSValueKind.Unknown) { if(has_val) str.append(' '); writeCssValueToBuffer(&mut start.clip, str, emitter); has_val = true }
        start++
    }
    if(ptr.color.kind != CSSValueKind.Unknown) { if(!ptr.layers.empty()) { str.append(' '); } writeCssValueToBuffer(&mut ptr.color, str, emitter) }
}

public func css_write_calc_expression(expr : &CSSCalcExpression, str : &mut std::string, emitter : *mut CssEmitter) {
    switch(expr.kind) {
        CSSCalcExpressionKind.Literal => { const val = expr.data as *mut CSSValue; writeCssValueToBuffer(&mut *val, str, emitter) }
        CSSCalcExpressionKind.Operation => { const data = expr.data as *mut CSSCalcOperationData; css_write_calc_expression(&data.left, str, emitter); str.append(' '); str.append(data.op); str.append(' '); css_write_calc_expression(&data.right, str, emitter) }
        CSSCalcExpressionKind.Group => { const inner = expr.data as *mut CSSCalcExpression; str.append('('); css_write_calc_expression(&*inner, str, emitter); str.append(')') }
    }
}

public func css_write_outline_value_data(ptr : &mut CSSOutlineValueData, str : &mut std::string, emitter : *mut CssEmitter) {
    const has_style = ptr.style.kind != CSSValueKind.Unknown; const has_color = ptr.color.kind != CSSValueKind.Unknown
    if(ptr.width.kind != CSSValueKind.Unknown) { writeCssValueToBuffer(&mut ptr.width, str, emitter); if(has_style || has_color) { str.append(' ') } }
    if(has_style) { writeCssValueToBuffer(&mut ptr.style, str, emitter); if(has_color) { str.append(' ') } }
    if(has_color) { writeCssValueToBuffer(&mut ptr.color, str, emitter) }
}

public func css_write_backdrop_filter_value_data(ptr : &mut CSSBackdropFilterValueData, str : &mut std::string, emitter : *mut CssEmitter) {
    str.append_view(&ptr.function.value)
    if(!ptr.arguments.empty()) {
        str.append('(')
        var i : uint = 0
        while(i < ptr.arguments.size()) { if(i > 0) str.append(' '); writeCssValueToBuffer(&mut *ptr.arguments.get_ptr(i), str, emitter); i++ }
        str.append(')')
    }
    if(ptr.next != null) { str.append(' '); css_write_backdrop_filter_value_data(&mut *ptr.next, str, emitter) }
}

public func css_write_animation_value_data(value : &mut CSSAnimationValueData, str : &mut std::string, emitter : *mut CssEmitter) {
    var has_val = false
    if(!value.name.empty()) { str.append_view(&value.name); has_val = true }
    if(value.duration.kind != CSSLengthKind.Unknown) { if(has_val) str.append(' '); css_write_length(&mut value.duration, str); has_val = true }
    if(value.easing.kind != CSSKeywordKind.Unknown) { if(has_val) str.append(' '); css_write_easing(&mut value.easing, str); has_val = true }
    if(value.delay.kind != CSSLengthKind.Unknown) { if(has_val) str.append(' '); css_write_length(&mut value.delay, str); has_val = true }
    if(value.iterationCount.kind != CSSValueKind.Unknown) { if(has_val) str.append(' '); writeCssValueToBuffer(&mut value.iterationCount, str, emitter); has_val = true }
    if(value.direction.kind != CSSKeywordKind.Unknown) { if(has_val) str.append(' '); str.append_view(&value.direction.value); has_val = true }
    if(value.fillMode.kind != CSSKeywordKind.Unknown) { if(has_val) str.append(' '); str.append_view(&value.fillMode.value); has_val = true }
    if(value.playState.kind != CSSKeywordKind.Unknown) { if(has_val) str.append(' '); str.append_view(&value.playState.value); has_val = true }
    if(value.next != null) { str.append(','); str.append(' '); css_write_animation_value_data(&mut *value.next, str, emitter) }
}

// ─── Media query serialization ───────────────────────────────────────────────

public func css_write_media_type(type : MediaType, customType : std::string_view, str : &mut std::string) {
    switch(type) {
        MediaType.All => { str.append_view("all") }
        MediaType.Screen => { str.append_view("screen") }
        MediaType.Print => { str.append_view("print") }
        MediaType.Speech => { str.append_view("speech") }
        MediaType.Unknown => { if(!customType.empty()) { str.append_view(&customType); } }
    }
}

public func css_write_comparison_operator(op : MediaFeatureComparison, str : &mut std::string) {
    switch(op) {
        MediaFeatureComparison.Equal => { str.append('=') }
        MediaFeatureComparison.LessThan => { str.append('<') }
        MediaFeatureComparison.LessThanEqual => { str.append_view("<=") }
        MediaFeatureComparison.GreaterThan => { str.append('>') }
        MediaFeatureComparison.GreaterThanEqual => { str.append_view(">=") }
        MediaFeatureComparison.None => {}
    }
}

public func css_write_media_feature(feature : *mut MediaFeature, str : &mut std::string, emitter : *mut CssEmitter) {
    str.append('(')
    if(feature.leftOp != MediaFeatureComparison.None) {
        if(feature.leftValue.kind != CSSValueKind.Unknown) { writeCssValueToBuffer(&mut feature.leftValue, str, emitter); str.append(' '); css_write_comparison_operator(feature.leftOp, str); str.append(' ') }
        str.append_view(&feature.name)
        if(feature.rightOp != MediaFeatureComparison.None && feature.rightValue.kind != CSSValueKind.Unknown) { str.append(' '); css_write_comparison_operator(feature.rightOp, str); str.append(' '); writeCssValueToBuffer(&mut feature.rightValue, str, emitter) }
    } else {
        str.append_view(&feature.name)
        if(feature.value.kind != CSSValueKind.Unknown) { str.append(':'); str.append(' '); writeCssValueToBuffer(&mut feature.value, str, emitter) }
    }
    str.append(')')
}

public func css_write_media_condition(condition : *mut MediaCondition, str : &mut std::string, emitter : *mut CssEmitter) {
    if(condition == null) { return }
    if(condition.isNot) { str.append_view("not ") }
    if(condition.feature != null) { css_write_media_feature(condition.feature, str, emitter) }
    if(condition.next != null) {
        switch(condition.op) {
            MediaConditionOp.And => { str.append_view(" and ") }
            MediaConditionOp.Or => { str.append_view(" or ") }
            MediaConditionOp.None => {}
        }
        css_write_media_condition(condition.next, str, emitter)
    }
}

public func css_write_media_query(query : *mut MediaQuery, str : &mut std::string, emitter : *mut CssEmitter) {
    switch(query.modifier) {
        MediaModifier.Only => { str.append_view("only ") }
        MediaModifier.Not => { str.append_view("not ") }
        MediaModifier.None => {}
    }
    if(query.mediaType != MediaType.Unknown || !query.customType.empty()) {
        css_write_media_type(query.mediaType, query.customType, str)
        if(query.condition != null) { str.append_view(" and ") }
    }
    if(query.condition != null) { css_write_media_condition(query.condition, str, emitter) }
}

public func css_write_media_query_list(queryList : *mut MediaQueryList, str : &mut std::string, emitter : *mut CssEmitter) {
    var i : uint = 0; const size = queryList.queries.size()
    while(i < size) { if(i > 0) { str.append_view(", "); } css_write_media_query(queryList.queries.get(i), str, emitter); i++ }
}

public func css_write_media_nested_rule(rule : *mut CSSNestedRule, str : &mut std::string, parent_selectors : &mut std::vector<std::string>, emitter : *mut CssEmitter) {
    var current_selectors = std::vector<std::string>()
    if(rule.selector != null) {
        var i : uint = 0
        while(i < rule.selector.selectors.size()) {
            var sel = rule.selector.selectors.get(i)
            if(css_has_ampersand_complex(sel) && !parent_selectors.empty()) {
                var p : uint = 0
                while(p < parent_selectors.size()) {
                    var resolved = std::string()
                    var pdf = parent_selectors.get_ptr(p)
                    css_serialize_complex(sel, &mut resolved, pdf.view())
                    current_selectors.push(resolved); p++
                }
            } else if(!parent_selectors.empty()) {
                var p : uint = 0
                while(p < parent_selectors.size()) {
                    var resolved = std::string()
                    var pdf = parent_selectors.get_ptr(p)
                    resolved.append_view(pdf.view()); resolved.append(' ')
                    css_serialize_complex(sel, &mut resolved, std::string_view("&"))
                    current_selectors.push(resolved); p++
                }
            } else {
                var resolved = std::string()
                css_serialize_complex(sel, &mut resolved, std::string_view("&"))
                current_selectors.push(resolved)
            }
            i++
        }
    }
    if(!rule.declarations.empty()) {
        var i : uint = 0
        while(i < current_selectors.size()) { if(i > 0) str.append_view(", "); var sel_str_ptr = current_selectors.get_ptr(i); str.append_view(sel_str_ptr.view()); i++ }
        str.append_view(" { ")
        i = 0
        while(i < rule.declarations.size()) { css_write_declaration_text(rule.declarations.get(i), str, emitter); i++ }
        str.append_view(" } ")
    }
    var nested_i : uint = 0
    while(nested_i < rule.nested_rules.size()) { css_write_media_nested_rule(rule.nested_rules.get(nested_i), str, &mut current_selectors, emitter); nested_i++ }
}

public func css_write_keyframes_rule(rule : *mut CSSKeyframesRule, str : &mut std::string, emitter : *mut CssEmitter) {
    str.append_view("@keyframes "); str.append_view(&rule.name); str.append_view(" { ")
    var i : uint = 0
    while(i < rule.keyframes.size()) {
        var keyframe = rule.keyframes.get(i)
        str.append_view(&keyframe.selector); str.append_view(" { ")
        var j : uint = 0
        while(j < keyframe.declarations.size()) { css_write_declaration_text(keyframe.declarations.get(j), str, emitter); j++ }
        str.append_view(" } "); i++
    }
    str.append_view("}")
}

public func css_write_media_rule(rule : *mut CSSMediaRule, str : &mut std::string, className : std::string_view, emitter : *mut CssEmitter) {
    str.append_view("@media ")
    css_write_media_query_list(rule.queryList, str, emitter)
    str.append_view(" { ")
    var parent_selectors = std::vector<std::string>()
    if(!className.empty()) {
        var root_selector = std::string()
        css_append_media_root_selector(&mut root_selector, className)
        parent_selectors.push(root_selector)
        if(!rule.declarations.empty()) {
            css_append_media_root_selector(str, className); str.append_view(" { ")
            var i : uint = 0
            while(i < rule.declarations.size()) { css_write_declaration_text(rule.declarations.get(i), str, emitter); i++ }
            str.append_view(" } ")
        }
    }
    var nested_i : uint = 0
    while(nested_i < rule.nested_rules.size()) { css_write_media_nested_rule(rule.nested_rules.get(nested_i), str, &mut parent_selectors, emitter); nested_i++ }
    str.append_view("}")
}

public func css_write_declaration_text(decl : *mut CSSDeclaration, str : &mut std::string, emitter : *mut CssEmitter) {
    str.append_view(&decl.property.name); str.append(':')
    writeCssValueToBuffer(&mut decl.value, str, emitter)
    if(decl.important) { str.append_view(" !important"); }
    str.append(';')
}
