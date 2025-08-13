
func (str : &std::string) view() : std::string_view {
    return std::string_view(str.data(), str.size());
}

struct ASTConverter {

    var builder : *mut ASTBuilder

    var support : *mut SymResSupport

    var vec : *mut VecRef<ASTNode>

    var parent : *mut ASTNode

    var str : std::string

}

func (converter : &mut ASTConverter) make_char_chain(value : char) : *mut AccessChain {
    const builder = converter.builder
    const support = converter.support
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var name : std::string_view = std::string_view("append_css_char")
    var id = builder.make_identifier(name, support.appendCssCharFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    const char_val = builder.make_char_value(value, location);
    args.push(char_val)
    const new_chain = builder.make_access_chain(std::span<*mut ChainValue>([ call ]), location)
    return new_chain;
}

func (converter : &mut ASTConverter) make_append_css_value_chain(value : *mut Value, len : size_t, hash : size_t) : *mut AccessChain {
    const builder = converter.builder
    const support = converter.support
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("append_css"), support.appendCssFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    args.push(value)
    args.push(builder.make_number_value(len, location));
    args.push(builder.make_number_value(hash, location))
    const new_chain = builder.make_access_chain(std::span<*mut ChainValue>([ call ]), location)
    return new_chain;
}

func (converter : &mut ASTConverter) make_value_chain(value : *mut Value, len : size_t) : *mut AccessChain {
    const builder = converter.builder
    const support = converter.support
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var name : std::string_view
    if(len == 0) {
        name = std::string_view("append_css_char_ptr")
    } else {
        name = std::string_view("append_css")
    }
    var node : *mut ASTNode
    if(len == 0) {
        node = support.appendCssCharPtrFn
    } else {
        node = support.appendCssFn
    }
    var id = builder.make_identifier(name, node, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    args.push(value)
    if(len != 0) {
        args.push(builder.make_number_value(len, location));
    }
    const new_chain = builder.make_access_chain(std::span<*mut ChainValue>([ call ]), location)
    return new_chain;
}

func (converter : &mut ASTConverter) make_expr_chain_of(value : *mut Value) : *mut AccessChain {
    return converter.make_value_chain(value, 0);
}

func (converter : &mut ASTConverter) make_chain_of_view(view : &std::string_view) : *mut AccessChain {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    const value = builder.make_string_value(builder.allocate_view(view), location)
    return converter.make_value_chain(value, view.size());
}

func (converter : &mut ASTConverter) make_chain_of(builder : *mut ASTBuilder, str : &mut std::string) : *mut AccessChain {
    const location = intrinsics::get_raw_location();
    const value = builder.make_string_value(builder.allocate_view(str.view()), location)
    const size = str.size()
    str.clear();
    return converter.make_value_chain(value, size);
}

func (converter : &mut ASTConverter) put_link_wrap_chain(chain : *mut AccessChain) {
    const builder = converter.builder
    var wrapped = builder.make_value_wrapper(chain, converter.parent)
    converter.vec.push(wrapped);
}

func (converter : &mut ASTConverter) put_char_chain(value : char) {
    const chain = converter.make_char_chain(value);
    converter.put_link_wrap_chain(chain);
}

func (converter : &mut ASTConverter) put_view_chain(view : &std::string_view) {
    const builder = converter.builder
    const chain = converter.make_chain_of_view(view);
    var wrapped = builder.make_value_wrapper(chain, converter.parent)
    converter.vec.push(wrapped);
}

func (converter : &mut ASTConverter) put_append_css_value_chain(view : &std::string_view, hash : size_t) {
    const location = intrinsics::get_raw_location();
    const builder = converter.builder
    const value = builder.make_string_value(builder.allocate_view(view), location)
    const chain = converter.make_append_css_value_chain(value, view.size(), hash);
    var wrapped = builder.make_value_wrapper(chain, converter.parent)
    converter.vec.push(wrapped);
}

func (converter : &mut ASTConverter) put_chain_in() {
    const builder = converter.builder
    const chain = converter.make_chain_of(builder, converter.str);
    var wrapped = builder.make_value_wrapper(chain, converter.parent)
    converter.vec.push(wrapped);
}

func (converter : &mut ASTConverter) put_wrapping(value : *mut Value) {
    const builder = converter.builder
    const wrapped = builder.make_value_wrapper(value, converter.parent)
    converter.vec.push(wrapped);
}

// we link the expression chain with a dummy string value (to already linked value to be linked twice)
// then we replace the value in expression chain
func (converter : &mut ASTConverter) put_wrapped_chemical_value_in(value : *mut Value) {
    // first we link the expression chain with dummy empty string value
    var chain = converter.make_expr_chain_of(value);
    // then we replace the dummy string value with actual linked value
    converter.put_wrapping(chain)
}

func is_func_call_ret_void(builder : *mut ASTBuilder, call : *mut FunctionCall) : bool {
    const type = builder.createType(call)
    if(type) {
        const kind = type.getKind();
        return kind == BaseTypeKind.Void;
    } else {
        return false;
    }
}

func (converter : &mut ASTConverter) put_chemical_value_in(value_ptr : *mut Value) {

    const builder = converter.builder

    var value = value_ptr;
    const kind = value.getKind();
    if(kind == ValueKind.AccessChain) {
        const chain = value as *mut AccessChain
        const values = chain.get_values();
        const size = values.size();
        const last = values.get(size - 1)
        if(last.getKind() == ValueKind.FunctionCall) {
            if(is_func_call_ret_void(builder, last as *mut FunctionCall)) {
                converter.put_wrapping(value);
            } else {
                converter.put_wrapped_chemical_value_in(value);
            }
        } else {
            converter.put_wrapped_chemical_value_in(value);
        }
    } else if(kind == ValueKind.FunctionCall) {
        if(is_func_call_ret_void(builder, value as *mut FunctionCall)) {
            converter.put_wrapping(value);
        } else {
            converter.put_wrapped_chemical_value_in(value);
        }
    } else {
        converter.put_wrapped_chemical_value_in(value);
    }
}

func writeUnitOfKind(str : &mut std::string, kind : CSSLengthKind) : bool {
    switch(kind) {
        CSSLengthKind.LengthPX => {
            var view = std::string_view("px")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthEM => {
            var view = std::string_view("em")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthREM => {
            var view = std::string_view("rem")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthVH => {
            var view = std::string_view("vh")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthVW => {
            var view = std::string_view("vw")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthVMIN => {
            var view = std::string_view("vmin")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthVMAX => {
            var view = std::string_view("vmax")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthPERCENTAGE => {
            str.append('%')
        }
        CSSLengthKind.LengthCM => {
            var view = std::string_view("cm")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthMM => {
            var view = std::string_view("mm")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthIN => {
            var view = std::string_view("in")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthPT => {
            var view = std::string_view("pt")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthPC => {
            var view = std::string_view("pc")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthCH => {
            var view = std::string_view("ch")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthEX => {
            var view = std::string_view("ex")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthS => {
            str.append('s')
        }
        CSSLengthKind.LengthMS => {
            var view = std::string_view("ms")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthHZ => {
            var view = std::string_view("hz")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthKHZ => {
            var view = std::string_view("khz")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthDEG => {
            var view = std::string_view("deg")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthRAD => {
            var view = std::string_view("rad")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthGRAD => {
            var view = std::string_view("grad")
            str.append_with_len(view.data(), view.size())
        }
        CSSLengthKind.LengthTURN => {
            var view = std::string_view("turn")
            str.append_with_len(view.data(), view.size())
        }
        default => {
            return false;
        }
    }
    return true;
}

func writeLength(ptr : &mut CSSLengthValueData, str : &mut std::string) {
    // writing the length
    str.append_with_len(ptr.value.data(), ptr.value.size())
    // writing the unit
    if(ptr.kind != CSSLengthKind.None && !writeUnitOfKind(str, ptr.kind)) {
        printf("unknown unit")
        fflush(null)
    }
}

func writeBorderRadiusValueData(ptr : &mut CSSBorderRadiusValueData, str : &mut std::string) {

    if(ptr.first.kind != CSSLengthKind.Unknown) {
        writeLength(ptr.first, str)
    }

    if(ptr.second.kind != CSSLengthKind.Unknown) {
        str.append(' ')
        writeLength(ptr.second, str)
    }

    if(ptr.third.kind != CSSLengthKind.Unknown) {
        str.append(' ')
        writeLength(ptr.third, str)
    }

    if(ptr.fourth.kind != CSSLengthKind.Unknown) {
        str.append(' ')
        writeLength(ptr.fourth, str)
    }

    if(ptr.next != null) {
        str.append_with_len(" / ", 3)
        writeBorderRadiusValueData(*ptr.next, str)
    }

}

func writeFontStyle(ptr : &CSSFontStyle, str : &mut std::string) {
    switch(ptr) {
        None => {}
        Keyword(keyword) => {
            str.append_with_len(keyword.value.data(), keyword.value.size())
        }
        Oblique(view) => {
            str.append_with_len(view.data(), view.size())
        }
    }
}

func writeFontWeight(ptr : &CSSFontWeight, str : &mut std::string) {
    switch(ptr) {
        None => {}
        Keyword(keyword) => {
            str.append(' ')
            str.append_with_len(keyword.value.data(), keyword.value.size())
        }
        Absolute(view) => {
            str.append(' ')
            str.append_with_len(view.data(), view.size())
        }
    }
}

func writeFontValueData(ptr : &CSSFontValueData, str : &mut std::string) {

    writeFontStyle(ptr.style, str)

    if(ptr.fontVariant.kind != CSSKeywordKind.Unknown) {
        str.append_with_len(ptr.fontVariant.value.data(), ptr.fontVariant.value.size())
    }

    writeFontWeight(ptr.weight, str)

    if(ptr.stretch.kind != CSSKeywordKind.Unknown) {
        str.append(' ')
        str.append_with_len(ptr.stretch.value.data(), ptr.stretch.value.size())
    }

    if(ptr.size.kind != CSSValueKind.Unknown) {
        str.append(' ')
        writeValue(ptr.size, str)
    }

    if(ptr.lineHeight.kind != CSSValueKind.Unknown) {
        str.append('/')
        writeValue(ptr.lineHeight, str)
    }

    var fmSize = ptr.family.families.size()
    var i : uint = 0
    while(i < fmSize) {
        const fm = ptr.family.families.get_ptr(i)
        str.append(',')
        str.append(' ')
        str.append_with_len(fm.data(), fm.size())
        i++;
    }

}

func writeTextShadowValueData(value : &mut CSSTextShadowValueData, str : &mut std::string) {

    if(value.offsetX.kind != CSSValueKind.Unknown) {
        writeValue(value.offsetX, str)
    }

    if(value.offsetY.kind != CSSValueKind.Unknown) {
        str.append(' ')
        writeValue(value.offsetY, str)
    }

    if(value.blurRadius.kind != CSSValueKind.Unknown) {
        str.append(' ')
        writeValue(value.blurRadius, str)
    }

    if(value.color.kind != CSSValueKind.Unknown) {
        str.append(' ')
        writeValue(value.color, str)
    }

    if(value.next != null) {
        str.append(',')
        writeTextShadowValueData(*value.next, str)
    }

}

func writeBoxShadowValueData(value : &mut CSSBoxShadowValueData, str : &mut std::string) {

    if(value.inset) {
        const inset = std::string_view("inset")
        str.append_with_len(inset.data(), inset.size())
    }

    if(value.offsetX.kind != CSSValueKind.Unknown) {
        if(value.inset) {
            str.append(' ')
        }
        writeValue(value.offsetX, str)
    }

    if(value.offsetY.kind != CSSValueKind.Unknown) {
        str.append(' ')
        writeValue(value.offsetY, str)
    }

    if(value.blurRadius.kind != CSSValueKind.Unknown) {
        str.append(' ')
        writeValue(value.blurRadius, str)
    }

    if(value.spreadRadius.kind != CSSValueKind.Unknown) {
        str.append(' ')
        writeValue(value.spreadRadius, str)
    }

    if(value.color.kind != CSSValueKind.Unknown) {
        str.append(' ')
        writeValue(value.color, str)
    }

    if(value.next != null) {
        str.append(',')
        writeBoxShadowValueData(*value.next, str)
    }

}

func writeLengthOrNone(len : &mut CSSLengthValueData, str : &mut std::string) {
    switch(len.kind) {
        CSSLengthKind.Unknown => {

        }
        CSSLengthKind.None => {
            if(len.value.empty()) {
                const view = std::string_view("none")
                str.append_with_len(view.data(), view.size())
            } else {
                writeLength(len, str);
            }
        }
        default => {
            writeLength(len, str);
        }
    }
}

func writeLengthOrNoneNotFirst(len : &mut CSSLengthValueData, str : &mut std::string) {
    if(len.kind != CSSLengthKind.Unknown) {
        str.append(' ')
        writeLengthOrNone(len, str);
    }
}

func writeAlphaLengthOrNone(len : &mut CSSLengthValueData, str : &mut std::string) {
    if(len.kind != CSSLengthKind.Unknown) {
        str.append(' ')
        str.append('/')
        str.append(' ')
        writeLengthOrNone(len, str);
    }
}

func writeRGBData(ptr : &mut CSSRGBColorData, str : &mut std::string) {
    writeLengthOrNone(ptr.red, str);
    writeLengthOrNoneNotFirst(ptr.green, str);
    writeLengthOrNoneNotFirst(ptr.blue, str);
    writeAlphaLengthOrNone(ptr.alpha, str);
}

func writeHSLData(ptr : &mut CSSHSLColorData, str : &mut std::string) {
    writeLengthOrNone(ptr.hue, str);
    writeLengthOrNoneNotFirst(ptr.saturation, str);
    writeLengthOrNoneNotFirst(ptr.lightness, str);
    writeAlphaLengthOrNone(ptr.alpha, str);
}

func writeHWBData(ptr : &mut CSSHWBColorData, str : &mut std::string) {
    writeLengthOrNone(ptr.hue, str);
    writeLengthOrNoneNotFirst(ptr.whiteness, str);
    writeLengthOrNoneNotFirst(ptr.blackness, str);
    writeAlphaLengthOrNone(ptr.alpha, str);
}

func writeLABData(ptr : &mut CSSLABColorData, str : &mut std::string) {
    writeLengthOrNone(ptr.lightness, str);
    writeLengthOrNoneNotFirst(ptr.rgAxis, str);
    writeLengthOrNoneNotFirst(ptr.byAxis, str);
    writeAlphaLengthOrNone(ptr.alpha, str);
}

func writeLCHData(ptr : &mut CSSLCHColorData, str : &mut std::string) {
    writeLengthOrNone(ptr.lightness, str);
    writeLengthOrNoneNotFirst(ptr.chroma, str);
    writeLengthOrNoneNotFirst(ptr.hue, str);
    writeAlphaLengthOrNone(ptr.alpha, str);
}

func writeOKLABData(ptr : &mut CSSOKLABColorData, str : &mut std::string) {
    writeLengthOrNone(ptr.lightness, str);
    writeLengthOrNoneNotFirst(ptr.aAxis, str);
    writeLengthOrNoneNotFirst(ptr.bAxis, str);
    writeAlphaLengthOrNone(ptr.alpha, str);
}

func writeOKLCHData(ptr : &mut CSSOKLCHColorData, str : &mut std::string) {
    writeLengthOrNone(ptr.lightness, str);
    writeLengthOrNoneNotFirst(ptr.pChroma, str);
    writeLengthOrNoneNotFirst(ptr.hue, str);
    writeAlphaLengthOrNone(ptr.alpha, str);
}

func writeColor(ptr : &mut CSSColorValueData, str : &mut std::string) {
    switch(ptr.kind) {
        CSSColorKind.RGB => {
            const rgbL = std::string_view("rgb(")
            str.append_with_len(rgbL.data(), rgbL.size())
            writeRGBData(*ptr.value.rgbData, str)
            str.append(')')
        }
        CSSColorKind.RGBA => {
           const rgbL = std::string_view("rgba(")
           str.append_with_len(rgbL.data(), rgbL.size())
           writeRGBData(*ptr.value.rgbData, str)
           str.append(')')
        }
        CSSColorKind.HSL => {
            const rgbL = std::string_view("hsl(")
            str.append_with_len(rgbL.data(), rgbL.size())
            writeHSLData(*ptr.value.hslData, str)
            str.append(')')
        }
        CSSColorKind.HSLA => {
            const rgbL = std::string_view("hsla(")
            str.append_with_len(rgbL.data(), rgbL.size())
            writeHSLData(*ptr.value.hslData, str)
            str.append(')')
        }
        CSSColorKind.HWB => {
            const rgbL = std::string_view("hwb(")
            str.append_with_len(rgbL.data(), rgbL.size())
            writeHWBData(*ptr.value.hwbData, str)
            str.append(')')
        }
        CSSColorKind.LAB => {
            const rgbL = std::string_view("lab(")
            str.append_with_len(rgbL.data(), rgbL.size())
            writeLABData(*ptr.value.labData, str)
            str.append(')')
        }
        CSSColorKind.LCH => {
            const rgbL = std::string_view("lch(")
            str.append_with_len(rgbL.data(), rgbL.size())
            writeLCHData(*ptr.value.lchData, str)
            str.append(')')
        }
        CSSColorKind.OKLAB => {
            const rgbL = std::string_view("oklab(")
            str.append_with_len(rgbL.data(), rgbL.size())
            writeOKLABData(*ptr.value.oklabData, str)
            str.append(')')
        }
        CSSColorKind.OKLCH => {
            const rgbL = std::string_view("oklch(")
            str.append_with_len(rgbL.data(), rgbL.size())
            writeOKLCHData(*ptr.value.oklchData, str)
            str.append(')')
        }
        CSSColorKind.COLOR => {

        }

        CSSColorKind.Unknown => {
            return;
        }

        default => {
            str.append_with_len(ptr.value.view.data(), ptr.value.view.size())
        }

    }
}

func writeLinearEasing(ptr : &mut CSSLinearEasingPoint, str : &mut std::string) {
    var has_value_before = false;
    if(ptr.point.kind != CSSLengthKind.Unknown) {
        writeLength(ptr.point, str)
        has_value_before = true;
    }
    if(ptr.start.kind != CSSLengthKind.Unknown) {
        if(has_value_before) {
            str.append(' ');
        } else {
            has_value_before = true;
        }
        writeLength(ptr.start, str)
    }
    if(ptr.stop.kind != CSSLengthKind.Unknown) {
        if(has_value_before) {
            str.append(' ');
        } else {
            has_value_before = true;
        }
        writeLength(ptr.stop, str)
    }
    if(ptr.next != null) {
        str.append(',')
        str.append(' ')
        writeLinearEasing(*ptr.next, str)
    }
}

func writeCubicBezierEasing(ptr : &mut CSSCubicBezierEasingData, str : &mut std::string) {
    writeLength(ptr.x1, str)
    str.append(',')
    writeLength(ptr.y1, str)
    str.append(',')
    writeLength(ptr.x2, str)
    str.append(',')
    writeLength(ptr.y2, str)
}

func writeStepsEasing(ptr : &mut CSSStepsEasingData, str : &mut std::string) {
    writeLength(ptr.step, str)
    str.append(' ')
    str.append_with_len(ptr.position.value.data(), ptr.position.value.size())
}

func writeEasing(ptr : &mut CSSEasingFunction, str : &mut std::string) {
    switch(ptr.kind) {
        CSSKeywordKind.Ease, CSSKeywordKind.EaseIn, CSSKeywordKind.EaseOut,
        CSSKeywordKind.EaseInOut, CSSKeywordKind.StepStart, CSSKeywordKind.StepEnd => {
            str.append_with_len(ptr.data.keyword.value.data(), ptr.data.keyword.value.size())
        }
        CSSKeywordKind.Linear => {
            const call = std::string_view("linear")
            str.append_with_len(call.data(), call.size())
            if(ptr.data.linear != null) {
                str.append('(')
                writeLinearEasing(*ptr.data.linear, str)
                str.append(')')
            }
        }
        CSSKeywordKind.CubicBezier => {
            const call = std::string_view("cubic-bezier(")
            str.append_with_len(call.data(), call.size())
            writeCubicBezierEasing(*ptr.data.bezier, str)
            str.append(')')
        }
        CSSKeywordKind.Steps => {
            const call = std::string_view("steps(")
            str.append_with_len(call.data(), call.size())
            writeStepsEasing(*ptr.data.steps, str)
            str.append(')')
        }
    }
}

func writeTransition(ptr : &mut CSSTransitionValueData, str : &mut std::string) {

    var has_value_before = false;

    if(!ptr.property.empty()) {
        str.append_with_len(ptr.property.data(), ptr.property.size())
        has_value_before = true;
    }

    if(ptr.duration.kind != CSSLengthKind.Unknown) {
        if(has_value_before) {
            str.append(' ')
        } else {
            has_value_before = true;
        }
        writeLength(ptr.duration, str)
    }

    if(ptr.easing.kind != CSSKeywordKind.Unknown) {
        if(has_value_before) {
            str.append(' ')
        } else {
            has_value_before = true;
        }
        writeEasing(ptr.easing, str)
    }

    if(ptr.delay.kind != CSSLengthKind.Unknown) {
        if(has_value_before) {
            str.append(' ')
        } else {
            has_value_before = true;
        }
        writeLength(ptr.delay, str)
    }

    if(ptr.behavior.kind != CSSLengthKind.Unknown) {
        if(has_value_before) {
            str.append(' ')
        } else {
            has_value_before = true;
        }
        str.append_with_len(ptr.behavior.value.data(), ptr.behavior.value.size())
    }

    if(ptr.next != null) {
        str.append(',')
        writeTransition(*ptr.next, str)
    }

}

func writeTransformNode(ptr : &mut CSSTransformLengthNode, str : &mut std::string) {

    writeLength(ptr.length, str)

    if(ptr.next != null) {
        str.append(',')
        str.append(' ')
        writeTransformNode(*ptr.next, str)
    }

}

func writeTransformValueData(ptr : &mut CSSTransformValueData, str : &mut std::string) {

    const name = &ptr.transformFunction.value
    str.append_with_len(name.data(), name.size())
    str.append('(')
    if(ptr.node != null) {
        writeTransformNode(*ptr.node, str)
    } else {
        const noneKw = std::string_view("none")
        str.append_with_len(noneKw.data(), noneKw.size())
    }
    str.append(')')
    if(ptr.next != null) {
        str.append(' ')
        writeTransformValueData(*ptr.next, str)
    }

}

func writeValue(value : &mut CSSValue, str : &mut std::string) {
    switch(value.kind) {

        CSSValueKind.Multiple => {
            const ptr = value.data as *mut CSSMultipleValues
            var i : uint = 0;
            const size = ptr.values.size();
            const last = size - 1;
            while(i < size) {
                const value_ptr = ptr.values.get_ptr(i);
                writeValue(*value_ptr, str)
                if(i < last) {
                    str.append(' ');
                }
                i++;
            }
        }

        CSSValueKind.Keyword => {
            var ptr = value.data as *mut CSSKeywordValueData
            str.append_with_len(ptr.value.data(), ptr.value.size())
            return;
        }

        CSSValueKind.Length => {
            const ptr = value.data as *mut CSSLengthValueData
            writeLength(*ptr, str)
        }

        CSSValueKind.Color => {

            var ptr = value.data as *mut CSSColorValueData

            writeColor(*ptr, str)

        }

        CSSValueKind.SingleLengthFunctionCall => {
            const ptr = value.data as *SingleLengthFuncCall
            str.append_with_len(ptr.name.value.data(), ptr.name.value.size())
            str.append('(')
            writeLength(ptr.length, str)
            str.append(')')
        }

        CSSValueKind.Font => {

            var ptr = value.data as *mut CSSFontValueData
            writeFontValueData(*ptr, str)

        }

        CSSValueKind.Border => {

            const ptr = value.data as *mut CSSBorderValueData

            const has_style = ptr.style.kind != CSSValueKind.Unknown;
            const has_color = ptr.color.kind != CSSValueKind.Unknown;

            // width
            if(ptr.width.kind != CSSValueKind.Unknown) {
                writeValue(ptr.width, str)
                if(has_style) {
                    str.append(' ')
                }
            }

            // style
            if(has_style) {
                writeValue(ptr.style, str)
                if(has_color) {
                    str.append(' ')
                }
            }

            // color
            if(has_color) {
                writeValue(ptr.color, str)
            }

        }

        CSSValueKind.BoxShadow => {

            const ptr = value.data as *mut CSSBoxShadowValueData

            if(ptr.isEmpty()) {

                const none = std::string_view("none")
                str.append_with_len(none.data(), none.size())

            } else {

                writeBoxShadowValueData(*ptr, str)

            }

        }

        CSSValueKind.TextShadow => {

            const ptr = value.data as *mut CSSTextShadowValueData

            if(ptr.isEmpty()) {

                const none = std::string_view("none")
                str.append_with_len(none.data(), none.size())

            } else {

                writeTextShadowValueData(*ptr, str)

            }

        }

        CSSValueKind.Transform => {

            const ptr = value.data as *mut CSSTransformValueData
            if(ptr.node == null && ptr.transformFunction.kind != CSSKeywordKind.Perspective) {
                const noneStr = std::string_view("none")
                str.append_with_len(noneStr.data(), noneStr.size())
            } else {
                writeTransformValueData(*ptr, str)
            }

        }

        CSSValueKind.BorderRadius => {

            const ptr = value.data as *mut CSSBorderRadiusValueData
            writeBorderRadiusValueData(*ptr, str)

        }

        CSSValueKind.Transition => {

            const ptr = value.data as *mut CSSTransitionValueData
            writeTransition(*ptr, str)
        }

        default => {
            printf("error no value found, kind %d\n", value.kind)
            fflush(null)
        }
    }
}

func (converter : &mut ASTConverter) convertDeclaration(decl : *mut CSSDeclaration) {

    const builder = converter.builder
    const str = &converter.str;

    const propertyName = decl.property.name.data()
    str.append_with_len(propertyName, decl.property.name.size())
    str.append(':')

    // if(!str.empty()) {
    //     put_chain_in(resolver, builder, vec, parent, str);
    // }
    // const value = attr.value as *mut ChemicalAttributeValue
    // put_chemical_value_in(resolver, builder, vec, parent, value.value)

    // put_char_chain(resolver, builder, vec, parent, '\"');

    writeValue(decl.value, *str)

    str.append(';')

}

const BASE64_CHARS : char[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

func base64_encode_32bit(hash : uint32_t, out : *mut char) {
    for (var i = 0; i < 6; i++) {
        out[5 - i] = BASE64_CHARS[hash & 0x3F]; // Extract 6 bits
        hash >>= 6;
    }
}

func put_class_name(hash : uint32_t, prefix : char, ptr : *mut char) {
    *ptr = '.'
    *(ptr + 1) = prefix
    base64_encode_32bit(hash, ptr + 2)
    *(ptr + 8) = '{'
}

func allocate_view_with_classname(builder : *mut ASTBuilder, str : &mut std::string, hash : size_t) : std::string_view {
    // append the last '}' into string
    str.append('}')
    // 9 characters are : 2 for ".h", then base64 encoded class name is 6 characters, then 1 for '{'
    const total_size = str.size() + 9;
    // +1 because we need null terminator at the end
    const ptr = builder.allocate_str_size(total_size + 1)
    put_class_name(hash, 'h', ptr)
    const cssStart = ptr + 9
    memcpy(cssStart, str.data(), str.size())
    *(ptr + total_size) = '\0'
    return std::string_view(ptr, total_size)
}

func (converter : &mut ASTConverter) put_hashed_string_chain() {
    const builder = converter.builder
    // calculate the hash before making any changes
    const hash = fnv1a_hash_32(converter.str.data());
    const classNameView = allocate_view_with_classname(builder, converter.str, hash)
    converter.put_append_css_value_chain(classNameView, hash)
}

func (converter : &mut ASTConverter) put_class_name_chain(hash : uint32_t, prefix : char) {
    var className : char[10] = [];
    className[0] = '.'
    className[1] = prefix
    base64_encode_32bit(hash, &mut className[2])
    className[8] = '{'
    className[9] = '\0'
    converter.put_view_chain(std::string_view(&className[0], 9u))
}

@extern
public func rand() : int;

func generate_random_32bit() : uint32_t {
    return (rand() as uint32_t << 16) | rand() as uint32_t;
}

func (converter : &mut ASTConverter) str_ref() : &mut std::string {
    return converter.str;
}

func (converter : &mut ASTConverter) convertCSSOM(om : *mut CSSOM) {
    const builder = converter.builder
    const str = &converter.str
    var size = om.declarations.size()
    if(size > 0) {
        if(om.has_dynamic_values) {
            const hash = generate_random_32bit();
            var className : char[10] = [];
            className[0] = '.'
            className[1] = 'r'
            base64_encode_32bit(hash, &mut className[2])
            className[8] = '{'
            className[9] = '\0'
            const total = builder.allocate_view(std::string_view(&className[0], 9u));
            converter.put_view_chain(total)
            // TODO this doesn't work
            // const dataPtr = total.data() + 1
            // om.className = std::string_view(dataPtr, 7)
        }
        var i : uint = 0
        while(i < size) {
            var decl = om.declarations.get(i)
            converter.convertDeclaration(decl)
            i++;
        }
        if(str.empty()) {
            converter.put_char_chain('}')
        } else {
            // end the string here
            if(!om.has_dynamic_values) {
                // calculate the hash before making any changes
                const hash = fnv1a_hash_32(str.data());
                const totalView = allocate_view_with_classname(builder, *str, hash)
                om.className = std::string_view(totalView.data() + 1, 7u)
                converter.put_append_css_value_chain(totalView, hash)
            } else {
                str.append('}')
                converter.put_chain_in();
            }
        }
    }
}