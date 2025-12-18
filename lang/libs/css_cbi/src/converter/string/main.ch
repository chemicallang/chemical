
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

func (converter : &mut ASTConverter) make_char_call(value : char) : *mut FunctionCallNode {
    const builder = converter.builder
    const support = converter.support
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var name : std::string_view = std::string_view("append_css_char")
    var id = builder.make_identifier(name, support.appendCssCharFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    const char_val = builder.make_char_value(value, location);
    args.push(char_val)
    return call;
}

func (converter : &mut ASTConverter) make_append_css_value_chain(value : *mut Value, len : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const support = converter.support
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("append_css"), support.appendCssFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    args.push(builder.make_ubigint_value(len, location));
    return call;
}

func (converter : &mut ASTConverter) make_value_chain(value : *mut Value, len : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const support = converter.support
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var name : std::string_view = std::string_view("append_css_nh")
    var node : *mut ASTNode = support.appendCssFn
    var id = builder.make_identifier(name, node, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    args.push(builder.make_ubigint_value(len, location));
    return call;
}

func (converter : &mut ASTConverter) make_value_call_with(value : *mut Value, fn_name : std::string_view, fnPtr : *mut ASTNode) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var id = builder.make_identifier(fn_name, fnPtr, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func (converter : &mut ASTConverter) make_char_ptr_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_css_char_ptr"), converter.support.appendCssCharPtrFn)
}

func (converter : &mut ASTConverter) make_char_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_css_char"), converter.support.appendCssCharFn)
}

func (converter : &mut ASTConverter) make_integer_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_css_integer"), converter.support.appendCssIntFn)
}

func (converter : &mut ASTConverter) make_uinteger_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_css_uinteger"), converter.support.appendCssUIntFn)
}

func (converter : &mut ASTConverter) make_float_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_css_float"), converter.support.appendCssFloatFn)
}

func (converter : &mut ASTConverter) make_double_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_css_double"), converter.support.appendCssDoubleFn)
}


func (converter : &mut ASTConverter) make_chain_of_view(view : &std::string_view) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    const value = builder.make_string_value(builder.allocate_view(view), location)
    return converter.make_value_chain(value, view.size());
}

func (converter : &mut ASTConverter) make_chain_of(builder : *mut ASTBuilder, str : &mut std::string) : *mut FunctionCallNode {
    const location = intrinsics::get_raw_location();
    const value = builder.make_string_value(builder.allocate_view(str.view()), location)
    const size = str.size()
    str.clear();
    return converter.make_value_chain(value, size);
}

func (converter : &mut ASTConverter) put_char_chain(value : char) {
    const chain = converter.make_char_call(value);
    converter.vec.push(chain);
}

func (converter : &mut ASTConverter) put_view_chain(view : &std::string_view) {
    const builder = converter.builder
    const chain = converter.make_chain_of_view(view);
    converter.vec.push(chain);
}

func (converter : &mut ASTConverter) put_append_css_value_chain(view : &std::string_view) {
    const location = intrinsics::get_raw_location();
    const builder = converter.builder
    const value = builder.make_string_value(builder.allocate_view(view), location)
    const chain = converter.make_append_css_value_chain(value, view.size());
    converter.vec.push(chain);
}

func (converter : &mut ASTConverter) put_chain_in() {
    const builder = converter.builder
    const chain = converter.make_chain_of(builder, converter.str);
    converter.vec.push(chain);
}

func (converter : &mut ASTConverter) put_wrapping(value : *mut Value) {
    const builder = converter.builder
    const wrapped = builder.make_value_wrapper(value, converter.parent)
    converter.vec.push(wrapped);
}

func is_func_call_ret_void(builder : *mut ASTBuilder, call : *mut FunctionCall) : bool {
    return call.getType().getKind() == BaseTypeKind.Void;
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
            str.append_view(std::string_view("px"))
        }
        CSSLengthKind.LengthEM => {
            str.append_view(std::string_view("em"))
        }
        CSSLengthKind.LengthREM => {
            str.append_view(std::string_view("rem"))
        }
        CSSLengthKind.LengthVH => {
            str.append_view(std::string_view("vh"))
        }
        CSSLengthKind.LengthVW => {
            str.append_view(std::string_view("vw"))
        }
        CSSLengthKind.LengthVMIN => {
            str.append_view(std::string_view("vmin"))
        }
        CSSLengthKind.LengthVMAX => {
            str.append_view(std::string_view("vmax"))
        }
        CSSLengthKind.LengthPERCENTAGE => {
            str.append('%')
        }
        CSSLengthKind.LengthCM => {
            str.append_view(std::string_view("cm"))
        }
        CSSLengthKind.LengthMM => {
            str.append_view(std::string_view("mm"))
        }
        CSSLengthKind.LengthIN => {
            str.append_view(std::string_view("in"))
        }
        CSSLengthKind.LengthPT => {
            str.append_view(std::string_view("pt"))
        }
        CSSLengthKind.LengthPC => {
            str.append_view(std::string_view("pc"))
        }
        CSSLengthKind.LengthCH => {
            str.append_view(std::string_view("ch"))
        }
        CSSLengthKind.LengthEX => {
            str.append_view(std::string_view("ex"))
        }
        CSSLengthKind.LengthS => {
            str.append('s')
        }
        CSSLengthKind.LengthMS => {
            str.append_view(std::string_view("ms"))
        }
        CSSLengthKind.LengthHZ => {
            str.append_view(std::string_view("hz"))
        }
        CSSLengthKind.LengthKHZ => {
            str.append_view(std::string_view("khz"))
        }
        CSSLengthKind.LengthDEG => {
            str.append_view(std::string_view("deg"))
        }
        CSSLengthKind.LengthRAD => {
            str.append_view(std::string_view("rad"))
        }
        CSSLengthKind.LengthGRAD => {
            str.append_view(std::string_view("grad"))
        }
        CSSLengthKind.LengthTURN => {
            str.append_view(std::string_view("turn"))
        }
        default => {
            return false;
        }
    }
    return true;
}

func writeLength(ptr : &mut CSSLengthValueData, str : &mut std::string) {
    if(ptr.kind == CSSLengthKind.Variable) {
        str.append_view(std::string_view("var("))
        str.append_view(ptr.value);
        str.append(')');
        return;
    }
    // writing the length
    str.append_view(ptr.value)
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
        str.append_view(std::string_view(" / "))
        writeBorderRadiusValueData(*ptr.next, str)
    }

}

func writeFontStyle(ptr : &CSSFontStyle, str : &mut std::string) {
    switch(ptr) {
        None => {}
        Keyword(keyword) => {
            str.append_view(keyword.value)
        }
        Oblique(view) => {
            str.append_view(view)
        }
    }
}

func writeFontWeight(ptr : &CSSFontWeight, str : &mut std::string) {
    switch(ptr) {
        None => {}
        Keyword(keyword) => {
            str.append(' ')
            str.append_view(keyword.value)
        }
        Absolute(view) => {
            str.append(' ')
            str.append_view(view)
        }
    }
}

func writeFontFamilyData(family : &mut CSSFontFamily, str : &mut std::string) {
    const first = family.families.data();
    var start = first
    const end = start + family.families.size()
    while(start != end) {
        if(start != first) {
            str.append(',');
        }
        str.append_view(*start)
        start++
    }
}

func (converter : &mut ASTConverter) writeFontValueData(ptr : &CSSFontValueData, str : &mut std::string) {

    writeFontStyle(ptr.style, str)

    if(ptr.fontVariant.kind != CSSKeywordKind.Unknown) {
        str.append(' ');
        str.append_view(ptr.fontVariant.value)
    }

    writeFontWeight(ptr.weight, str)

    if(ptr.stretch.kind != CSSKeywordKind.Unknown) {
        str.append(' ')
        str.append_view(ptr.stretch.value)
    }

    if(ptr.size.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(ptr.size)
    }

    if(ptr.lineHeight.kind != CSSValueKind.Unknown) {
        str.append('/')
        converter.writeValue(ptr.lineHeight)
    }

    if(!ptr.family.families.empty()) {
        str.append(' ');
    }
    writeFontFamilyData(ptr.family, str)

}

func (converter : &mut ASTConverter) writeTextShadowValueData(value : &mut CSSTextShadowValueData, str : &mut std::string) {

    if(value.offsetX.kind != CSSValueKind.Unknown) {
        converter.writeValue(value.offsetX)
    }

    if(value.offsetY.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(value.offsetY)
    }

    if(value.blurRadius.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(value.blurRadius)
    }

    if(value.color.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(value.color)
    }

    if(value.next != null) {
        str.append(',')
        converter.writeTextShadowValueData(*value.next, str)
    }

}

func (converter : &mut ASTConverter) writeBoxShadowValueData(value : &mut CSSBoxShadowValueData, str : &mut std::string) {

    if(value.inset) {
        str.append_view(std::string_view("inset"))
    }

    if(value.offsetX.kind != CSSValueKind.Unknown) {
        if(value.inset) {
            str.append(' ')
        }
        converter.writeValue(value.offsetX)
    }

    if(value.offsetY.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(value.offsetY)
    }

    if(value.blurRadius.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(value.blurRadius)
    }

    if(value.spreadRadius.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(value.spreadRadius)
    }

    if(value.color.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(value.color)
    }

    if(value.next != null) {
        str.append(',')
        converter.writeBoxShadowValueData(*value.next, str)
    }

}

func writeLengthOrNone(len : &mut CSSLengthValueData, str : &mut std::string) {
    switch(len.kind) {
        CSSLengthKind.Unknown => {

        }
        CSSLengthKind.None => {
            if(len.value.empty()) {
                str.append_view(std::string_view("none"))
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
            str.append_view(std::string_view("rgb("))
            writeRGBData(*ptr.value.rgbData, str)
            str.append(')')
        }
        CSSColorKind.RGBA => {
           str.append_view(std::string_view("rgba("))
           writeRGBData(*ptr.value.rgbData, str)
           str.append(')')
        }
        CSSColorKind.HSL => {
            str.append_view(std::string_view("hsl("))
            writeHSLData(*ptr.value.hslData, str)
            str.append(')')
        }
        CSSColorKind.HSLA => {
            str.append_view(std::string_view("hsla("))
            writeHSLData(*ptr.value.hslData, str)
            str.append(')')
        }
        CSSColorKind.HWB => {
            str.append_view(std::string_view("hwb("))
            writeHWBData(*ptr.value.hwbData, str)
            str.append(')')
        }
        CSSColorKind.LAB => {
            str.append_view(std::string_view("lab("))
            writeLABData(*ptr.value.labData, str)
            str.append(')')
        }
        CSSColorKind.LCH => {
            str.append_view(std::string_view("lch("))
            writeLCHData(*ptr.value.lchData, str)
            str.append(')')
        }
        CSSColorKind.OKLAB => {
            str.append_view(std::string_view("oklab("))
            writeOKLABData(*ptr.value.oklabData, str)
            str.append(')')
        }
        CSSColorKind.OKLCH => {
            str.append_view(std::string_view("oklch("))
            writeOKLCHData(*ptr.value.oklchData, str)
            str.append(')')
        }
        CSSColorKind.COLOR => {
            // TODO:
        }
        CSSColorKind.VAR => {
            str.append_view(std::string_view("var("))
            str.append_view(ptr.value.view)
            str.append(')')
        }

        CSSColorKind.Unknown => {
            return;
        }

        default => {
            str.append_view(ptr.value.view)
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
    str.append(',')
    str.append_view(ptr.position.value)
}

func writeEasing(ptr : &mut CSSEasingFunction, str : &mut std::string) {
    switch(ptr.kind) {
        CSSKeywordKind.Ease, CSSKeywordKind.EaseIn, CSSKeywordKind.EaseOut,
        CSSKeywordKind.EaseInOut, CSSKeywordKind.StepStart, CSSKeywordKind.StepEnd => {
            str.append_view(ptr.data.keyword.value)
        }
        CSSKeywordKind.Linear => {
            str.append_view(std::string_view("linear"))
            if(ptr.data.linear != null) {
                str.append('(')
                writeLinearEasing(*ptr.data.linear, str)
                str.append(')')
            }
        }
        CSSKeywordKind.CubicBezier => {
            str.append_view(std::string_view("cubic-bezier("))
            writeCubicBezierEasing(*ptr.data.bezier, str)
            str.append(')')
        }
        CSSKeywordKind.Steps => {
            str.append_view(std::string_view("steps("))
            writeStepsEasing(*ptr.data.steps, str)
            str.append(')')
        }
    }
}

func writeTransition(ptr : &mut CSSTransitionValueData, str : &mut std::string) {

    var has_value_before = false;

    if(!ptr.property.empty()) {
        str.append_view(ptr.property)
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
        str.append_view(ptr.behavior.value)
    }

    if(ptr.next != null) {
        str.append(',')
        writeTransition(*ptr.next, str)
    }

}

func writeTransformNode(ptr : &mut CSSTransformLengthNode, str : &mut std::string) {

    writeLength(ptr.length, str)

    if(ptr.next != null && ptr.next.length.kind != CSSLengthKind.Unknown) {
        str.append(',')
        writeTransformNode(*ptr.next, str)
    }

}

func writeTransformValueData(ptr : &mut CSSTransformValueData, str : &mut std::string) {

    str.append_view(ptr.transformFunction.value)
    str.append('(')
    if(ptr.node != null) {
        writeTransformNode(*ptr.node, str)
    } else {
        str.append_view(std::string_view("none"))
    }
    str.append(')')
    if(ptr.next != null) {
        str.append(' ')
        writeTransformValueData(*ptr.next, str)
    }

}

func writeBackgroundImageUrl(url : &mut UrlData, str : &mut std::string) {

    if(url.is_source) {
        str.append_view(std::string_view("src"))
    } else {
        str.append_view(std::string_view("url"))
    }
    str.append('(');
    str.append_view(url.value)
    str.append(')');

}

func (converter : &mut ASTConverter) writeLinearGradientData(data : &mut LinearGradientData, str : &mut std::string) {

    writeLength(data.angle, str)
    if(data.angle.kind != CSSLengthKind.Unknown) {
        str.append(',');
    }

    if(data.to1.kind != CSSKeywordKind.Unknown) {
        str.append_view(std::string_view("to "))
        str.append_view(data.to1.value)

        if(data.to2.kind != CSSKeywordKind.Unknown) {
            str.append(' ');
            str.append_view(data.to2.value)
        }

        str.append(',');

    }

    var start = data.color_stop_list.data()
    const end = start + data.color_stop_list.size()
    while(start != end) {

        converter.writeValue(start.hint)

        converter.writeValue(start.stop.color)

        if(start.stop.length.kind != CSSValueKind.Unknown) {

            str.append(' ');
            converter.writeValue(start.stop.length)

            if(start.stop.optSecLength.kind != CSSValueKind.Unknown) {
                str.append(' ');
                converter.writeValue(start.stop.optSecLength)
            }

        }

        start++;

        if(start != end) str.append(',');

    }

}

func (converter : &mut ASTConverter) writeRadialGradientData(data : &mut RadialGradientData, str : &mut std::string) {
    // Write shape, size, position
    // Syntax: [ <ending-shape> || <size> ]? [ at <position> ]? , <color-stop-list>
    
    var has_shape_or_size = false
    
    if(data.shape.kind != CSSKeywordKind.Unknown) {
        str.append_view(data.shape.value)
        has_shape_or_size = true
    }
    
    if(data.size.extent.kind != CSSKeywordKind.Unknown) {
        if(has_shape_or_size) str.append(' ')
        str.append_view(data.size.extent.value)
        has_shape_or_size = true
    } else if(data.size.length.kind != CSSValueKind.Unknown) {
        if(has_shape_or_size) str.append(' ')
        converter.writeValue(data.size.length)
        has_shape_or_size = true
    }
    
    if(data.position.kind != CSSValueKind.Unknown) {
        if(has_shape_or_size) str.append(' ')
        str.append_view(std::string_view("at "))
        converter.writeValue(data.position)
        has_shape_or_size = true // effectively
    }
    
    if(has_shape_or_size) {
        str.append(',')
    }
    
    var start = data.color_stop_list.data()
    const end = start + data.color_stop_list.size()
    while(start != end) {

        converter.writeValue(start.hint)

        converter.writeValue(start.stop.color)

        if(start.stop.length.kind != CSSValueKind.Unknown) {

            str.append(' ');
            converter.writeValue(start.stop.length)

            if(start.stop.optSecLength.kind != CSSValueKind.Unknown) {
                str.append(' ');
                converter.writeValue(start.stop.optSecLength)
            }

        }

        start++;

        if(start != end) str.append(',');

    }
}

func (converter : &mut ASTConverter) writeConicGradientData(data : &mut ConicGradientData, str : &mut std::string) {
    // Write from <angle> at <position>
    
    var has_from_or_at = false
    
    if(data.from.kind != CSSValueKind.Unknown) {
        str.append_view(std::string_view("from "))
        converter.writeValue(data.from)
        has_from_or_at = true
    }
    
    if(data.at.kind != CSSValueKind.Unknown) {
        if(has_from_or_at) str.append(' ')
        str.append_view(std::string_view("at "))
        converter.writeValue(data.at)
        has_from_or_at = true
    }
    
    if(has_from_or_at) {
        str.append(',')
    }

    var start = data.color_stop_list.data()
    const end = start + data.color_stop_list.size()
    while(start != end) {

        converter.writeValue(start.hint)

        converter.writeValue(start.stop.color)

        if(start.stop.length.kind != CSSValueKind.Unknown) {

            str.append(' ');
            converter.writeValue(start.stop.length)

            if(start.stop.optSecLength.kind != CSSValueKind.Unknown) {
                str.append(' ');
                converter.writeValue(start.stop.optSecLength)
            }

        }

        start++;

        if(start != end) str.append(',');

    }
}

func (converter : &mut ASTConverter) writeBackgroundImageData(ptr : &mut BackgroundImageData, str : &mut std::string) {

    if(ptr.is_url) {
        writeBackgroundImageUrl(ptr.url, str)
    } else {
        switch(ptr.gradient.kind) {
            CSSGradientKind.RepeatingLinear => {
                str.append_view(std::string_view("repeating-linear-gradient("))
                converter.writeLinearGradientData(*(ptr.gradient.data as *mut LinearGradientData), str)
                str.append(')')
            }
            CSSGradientKind.RepeatingRadial => {
                str.append_view(std::string_view("repeating-radial-gradient("))
                converter.writeRadialGradientData(*(ptr.gradient.data as *mut RadialGradientData), str)
                str.append(')')
            }
            CSSGradientKind.RepeatingConic => {
                str.append_view(std::string_view("repeating-conic-gradient("))
                converter.writeConicGradientData(*(ptr.gradient.data as *mut ConicGradientData), str)
                str.append(')')
            }
            CSSGradientKind.Linear => {
                str.append_view(std::string_view("linear-gradient("))
                converter.writeLinearGradientData(*(ptr.gradient.data as *mut LinearGradientData), str)
                str.append(')')
            }
            CSSGradientKind.Radial => {
                str.append_view(std::string_view("radial-gradient("))
                converter.writeRadialGradientData(*(ptr.gradient.data as *mut RadialGradientData), str)
                str.append(')')
            }
            CSSGradientKind.Conic => {
                str.append_view(std::string_view("conic-gradient("))
                converter.writeConicGradientData(*(ptr.gradient.data as *mut ConicGradientData), str)
                str.append(')')
            }
            default => {
                // Unknown or not a gradient
            }
        }
    }

}

func (converter : &mut ASTConverter) put_wrapped_chemical_value_in(value : *mut Value) {
    const chain = converter.make_char_ptr_value_call(value)
    converter.vec.push(chain)
}

func (converter : &mut ASTConverter) put_wrapped_chemical_char_value_in(value : *mut Value) {
    var chain = converter.make_char_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut ASTConverter) put_wrapped_chemical_integer_value_in(value : *mut Value) {
    var chain = converter.make_integer_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut ASTConverter) put_wrapped_chemical_uinteger_value_in(value : *mut Value) {
    var chain = converter.make_uinteger_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut ASTConverter) put_wrapped_chemical_float_value_in(value : *mut Value) {
    var chain = converter.make_float_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut ASTConverter) put_wrapped_chemical_double_value_in(value : *mut Value) {
    var chain = converter.make_double_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut ASTConverter) put_by_type(type : *mut BaseType, value : *mut Value) {
    switch(type.getKind()) {
        BaseTypeKind.Void => {
            converter.put_wrapping(value);
        }
        BaseTypeKind.IntN => {
            const intN = type as *mut IntNType;
            const kind = intN.get_intn_type_kind()
            if(kind == IntNTypeKind.Char || kind == IntNTypeKind.UChar) {
                converter.put_wrapped_chemical_char_value_in(value)
            } else if(kind <= IntNTypeKind.Int128) {
                // signed
                converter.put_wrapped_chemical_integer_value_in(value)
            } else {
                // unsigned
                converter.put_wrapped_chemical_uinteger_value_in(value)
            }
        }
        BaseTypeKind.Float => {
            converter.put_wrapped_chemical_float_value_in(value)
        }
        BaseTypeKind.Double => {
            converter.put_wrapped_chemical_double_value_in(value)
        }
        default => {
            converter.put_wrapped_chemical_value_in(value);
        }
    }
}

func (converter : &mut ASTConverter) str_ref() : &mut std::string {
    return converter.str;
}

func (converter : &mut ASTConverter) writeBackgroundValueData(ptr : &mut CSSBackgroundValueData, str : &mut std::string) {
    
    var start = ptr.layers.data();
    const end = start + ptr.layers.size();
    while(start != end) {

        if(start != ptr.layers.data()) {
            str.append(',');
            str.append(' ');
        }
        
        var has_val = false;

        if(start.image.kind != CSSValueKind.Unknown) {
            converter.writeValue(start.image);
            has_val = true;
        }

        if(start.positionX.kind != CSSValueKind.Unknown) {

            if(has_val) str.append(' ');
            converter.writeValue(start.positionX);
            if(start.positionY.kind != CSSValueKind.Unknown) {
                str.append(' ');
                converter.writeValue(start.positionY);
            }

            if(start.size.kind != CSSValueKind.Unknown) {
                str.append('/');
                converter.writeValue(start.size);
            }
            has_val = true;
        }

        if(start.repeat.kind != CSSValueKind.Unknown) {
            if(has_val) str.append(' ');
            converter.writeValue(start.repeat);
            has_val = true;
        }

        if(start.attachment.kind != CSSValueKind.Unknown) {
            if(has_val) str.append(' ');
            converter.writeValue(start.attachment);
            has_val = true;
        }

        if(start.origin.kind != CSSValueKind.Unknown) {
            if(has_val) str.append(' ');
            converter.writeValue(start.origin);
            has_val = true;
        }

        if(start.clip.kind != CSSValueKind.Unknown) {
            if(has_val) str.append(' ');
            converter.writeValue(start.clip);
            has_val = true;
        }


        start++;
    }

    if(ptr.color.kind != CSSValueKind.Unknown) {
        if(!ptr.layers.empty()) {
            str.append(' ');
        }
        converter.writeValue(ptr.color);
    }

}

func (converter : &mut ASTConverter) writeValue(value : &mut CSSValue) {

    // make this a reference
    var str = converter.str_ref()

    switch(value.kind) {

        CSSValueKind.Multiple => {
            const ptr = value.data as *mut CSSMultipleValues
            var i : uint = 0;
            const size = ptr.values.size();
            const last = size - 1;
            while(i < size) {
                const value_ptr = ptr.values.get_ptr(i);
                converter.writeValue(*value_ptr)
                if(i < last) {
                    str.append(' ');
                }
                i++;
            }
        }

        CSSValueKind.Pair => {

            const pair = value.data as *mut CSSValuePair
            converter.writeValue(pair.first)
            str.append(' ');
            converter.writeValue(pair.second)

        }

        CSSValueKind.Keyword => {
            var ptr = value.data as *mut CSSKeywordValueData
            str.append_view(ptr.value)
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
            str.append_view(ptr.name.value)
            str.append('(')
            writeLength(ptr.length, str)
            str.append(')')
        }

        CSSValueKind.Font => {

            var ptr = value.data as *mut CSSFontValueData
            converter.writeFontValueData(*ptr, str)

        }

        CSSValueKind.FontFamily => {

            var ptr = value.data as *mut CSSFontFamily
            writeFontFamilyData(*ptr, str)

        }

        CSSValueKind.Border => {

            const ptr = value.data as *mut CSSBorderValueData

            const has_style = ptr.style.kind != CSSValueKind.Unknown;
            const has_color = ptr.color.kind != CSSValueKind.Unknown;

            // width
            if(ptr.width.kind != CSSValueKind.Unknown) {
                converter.writeValue(ptr.width)
                if(has_style) {
                    str.append(' ')
                }
            }

            // style
            if(has_style) {
                converter.writeValue(ptr.style)
                if(has_color) {
                    str.append(' ')
                }
            }

            // color
            if(has_color) {
                converter.writeValue(ptr.color)
            }

        }

        CSSValueKind.BoxShadow => {

            const ptr = value.data as *mut CSSBoxShadowValueData

            if(ptr.isEmpty()) {

                str.append_view(std::string_view("none"))

            } else {

                converter.writeBoxShadowValueData(*ptr, str)

            }

        }

        CSSValueKind.TextShadow => {

            const ptr = value.data as *mut CSSTextShadowValueData

            if(ptr.isEmpty()) {

                str.append_view(std::string_view("none"))

            } else {

                converter.writeTextShadowValueData(*ptr, str)

            }

        }

        CSSValueKind.Transform => {

            const ptr = value.data as *mut CSSTransformValueData
            if(ptr.node == null && ptr.transformFunction.kind != CSSKeywordKind.Perspective) {
                str.append_view(std::string_view("none"))
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

        CSSValueKind.TransitionTimingFunction => {

            const ptr = value.data as *mut CSSEasingFunction
            writeEasing(*ptr, str)

        }

        CSSValueKind.MultipleBackgroundImage => {

            const ptr = value.data as *mut MultipleBackgroundImageData
            var start = ptr.images.data()
            const end = start + ptr.images.size()
            while(start != end) {
                converter.writeBackgroundImageData(*start, str)
                start++;
            }

        }

        CSSValueKind.BackgroundImage => {

            const ptr = value.data as *mut BackgroundImageData
            converter.writeBackgroundImageData(*ptr, str)

        }

        CSSValueKind.Background => {
            const ptr = value.data as *mut CSSBackgroundValueData
            converter.writeBackgroundValueData(*ptr, str);
        }

        CSSValueKind.ChemicalValue => {

            converter.put_chain_in();
            const ptr = value.data as *mut Value
            converter.put_by_type(ptr.getType(), ptr)

        }

        CSSValueKind.Unknown => {
            // do nothing
        }

        default => {
            printf("error no value found, kind %d\n", value.kind)
            fflush(null)
        }
    }
}

func (converter : &mut ASTConverter) convertDeclaration(decl : *mut CSSDeclaration) {

    const builder = converter.builder
    const str = &mut converter.str;

    str.append_view(decl.property.name)
    str.append(':')

    // if(!str.empty()) {
    //     put_chain_in(resolver, builder, vec, parent, str);
    // }
    // const value = attr.value as *mut ChemicalAttributeValue
    // put_chemical_value_in(resolver, builder, vec, parent, value.value)

    // put_char_chain(resolver, builder, vec, parent, '\"');

    converter.writeValue(decl.value)

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

func (converter : &mut ASTConverter) make_func_call_with_arg(value : *mut Value, fn_name : std::string_view, fnPtr : *mut ASTNode) : *mut FunctionCall {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var id = builder.make_identifier(fn_name, fnPtr, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func (converter : &mut ASTConverter) make_require_css_hash_call(hash : size_t) : *mut FunctionCall {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var value = builder.make_ubigint_value(hash, location)
    return converter.make_func_call_with_arg(value, std::string_view("require_css_hash"), converter.support.requireCssHashFn)
}

func (converter : &mut ASTConverter) make_set_css_hash_call(hash : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var value = builder.make_ubigint_value(hash, location)
    return converter.make_value_call_with(value, std::string_view("set_css_hash"), converter.support.setCssHashFn)
}

func (converter : &mut ASTConverter) writeMediaRule(rule : *mut CSSMediaRule, str : &mut std::string, className : std::string_view) {
    str.append_view(std::string_view("@media "))
    str.append_view(rule.query)
    str.append_view(std::string_view(" { ."))
    str.append_view(className)
    str.append_view(std::string_view(" { "))
    
    var size = rule.declarations.size()
    var i : uint = 0
    while(i < size) {
        var decl = rule.declarations.get(i)
        converter.convertDeclaration(decl)
        i++;
    }

    str.append_view(std::string_view(" } }"))
}

func (converter : &mut ASTConverter) make_require_random_css_hash_call(hash : size_t) : *mut FunctionCall {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var value = builder.make_ubigint_value(hash, location)
    return converter.make_func_call_with_arg(value, std::string_view("require_random_css_hash"), converter.support.requireRandomCssHashFn)
}

func (converter : &mut ASTConverter) make_set_random_css_hash_call(hash : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var value = builder.make_ubigint_value(hash, location)
    return converter.make_value_call_with(value, std::string_view("set_random_css_hash"), converter.support.setRandomCssHashFn)
}

// AST Helper: Check if selector has ampersand
func has_ampersand_simple(s : *mut SimpleSelector) : bool {
    return s.kind == SimpleSelectorKind.Ampersand;
}
func has_ampersand_compound(c : *mut CompoundSelector) : bool {
    var i : uint = 0;
    while(i < c.simple_selectors.size()) {
        if(has_ampersand_simple(c.simple_selectors.get(i))) return true;
        i++;
    }
    return false;
}
func has_ampersand_complex(c : *mut ComplexSelector) : bool {
    if(has_ampersand_compound(c.compound)) return true;
    if(c.next != null) return has_ampersand_complex(c.next);
    return false;
}

// AST Serializers
func serialize_simple(s : *mut SimpleSelector, out : &mut std::string, replacement : std::string_view) {
    if(s.kind == SimpleSelectorKind.Ampersand) {
        out.append_view(replacement);
        return;
    }
    if(s.kind == SimpleSelectorKind.Class) out.append('.');
    if(s.kind == SimpleSelectorKind.Id) out.append('#');
    out.append_view(s.value);
    // TODO: Attribute selectors
}
func serialize_compound(c : *mut CompoundSelector, out : &mut std::string, replacement : std::string_view) {
    var i : uint = 0;
    while(i < c.simple_selectors.size()) {
        serialize_simple(c.simple_selectors.get(i), out, replacement);
        i++;
    }
}
func serialize_complex(c : *mut ComplexSelector, out : &mut std::string, replacement : std::string_view) {
    serialize_compound(c.compound, out, replacement);
    if(c.next != null) {
        if(c.combinator == Combinator.Descendant) out.append(' ');
        else if(c.combinator == Combinator.Child) out.append_view(" > ");
        else if(c.combinator == Combinator.NextSibling) out.append_view(" + ");
        else if(c.combinator == Combinator.SubsequentSibling) out.append_view(" ~ ");
        
        serialize_complex(c.next, out, replacement);
    }
}

func (converter : &mut ASTConverter) generate_css_recurse(om : *CSSNestedRule, parent_selectors : &mut std::vector<std::string>) {
    const str = &mut converter.str
    // Wait, CSSNestedRule doesn't have declarations? Yes it does.
    // But `om` here is `CSSNestedRule`.
    // The main loop calls with `CSSOM` (wrapped/adapted?).
    // We need logic that works for both.
    
    // Resolve selectors for THIS rule against parent_selectors
    var current_selectors = std::vector<std::string>();
    
    if(om.selector != null) {
        // Nested rule
        var i : uint = 0;
        const size = om.selector.selectors.size();
        while(i < size) {
             var sel = om.selector.selectors.get(i);
             var has_amp = has_ampersand_complex(sel);
             
             var p : uint = 0;
             while(p < parent_selectors.size()) {
                 var pdf = parent_selectors.get(p);
                 var res = std::string();
                 if(has_amp) {
                     serialize_complex(sel, res, pdf.view());
                 } else {
                     res.append_view(pdf.view());
                     res.append(' ');
                     serialize_complex(sel, res, std::string_view("&")); // replacement ignored
                 }
                 current_selectors.push(res);
                 p++;
             }
             i++;
        }
    } else {
        // Root CSSOM case (passed as if it had no selector logic, just parents)
        // Usually called with `parent_selectors` being the hash class.
        // We just copy parent_selectors.
        current_selectors = std::replace(parent_selectors, std::vector<std::string>());
    }
    
    // 1. Output Declarations
    if(!om.declarations.empty()) {
        // Join selectors
        var sel_str = std::string();
        var k : uint = 0;
        while(k < current_selectors.size()) {
            if(k > 0) sel_str.append_view(", ");
            sel_str.append_view(current_selectors.get(k).view());
            k++;
        }
        
        str.append_view(sel_str.view());
        str.append_view(" { ");
        var idx : uint = 0;
        while(idx < om.declarations.size()) {
             var decl = om.declarations.get(idx);
             converter.convertDeclaration(decl);
             idx++;
        }
        str.append('}');
        converter.put_chain_in();
    }
    
    // 2. Nested Rules
    var n : uint = 0;
    while(n < om.nested_rules.size()) {
        var rule = om.nested_rules.get(n);
        converter.generate_css_recurse(rule, current_selectors);
        n++;
    }
    
    // 3. Media Queries (CSSOM only?) 
    // CSSNestedRule from `nested_rule.ch` doesn't store media queries?
    // User revert removed `media_queries` from `CSSNestedRule`? Or I never added it?
    // Current `nested_rule.ch` struct def has `selector`, `declarations`, `nested_rules`. 
    // It does NOT have media queries.
    // If I want nested media queries, I need to parse them.
    // User requirements include "media.ch hack fix later".
    // I will skip nested media queries inside nested rules for now as struct doesn't support it yet.
    // BUT `om` passed here can be `CSSNestedRule` or `CSSOM`?
    // I need `generate_css_recurse` to take `CSSNestedRule`.
    // Root `CSSOM` has `nested_rules` (vector of CSSNestedRule).
    // So distinct types.
}

func (converter : &mut ASTConverter) generate_css_root(om : *mut CSSOM, root_selector : std::string_view) {
     var parents = std::vector<std::string>();

     var root_sel_str = std::string()
     root_sel_str.append_view(root_selector)
     parents.push(root_sel_str);
     
     // Root Declarations
     // Similar to generate_css_recurse but for CSSOM struct
     const str = &mut converter.str
     
     if(om.declarations.size() > 0) {
         str.append_view(root_selector);
         str.append_view(" { ");
         var i : uint = 0;
         while(i < om.declarations.size()) {
             converter.convertDeclaration(om.declarations.get(i));
             i++;
         }
         str.append('}');
         converter.put_chain_in();
     }
     
     // Nested Rules from Root
     var j : uint = 0;
     while(j < om.nested_rules.size()) {
         converter.generate_css_recurse(om.nested_rules.get(j), parents);
         j++;
     }
     
     // Media (Root only for now)
     var m : uint = 0;
     while(m < om.media_queries.size()) {
            var rule = om.media_queries.get(m);
            converter.writeMediaRule(rule, *str, root_selector);
            if(!str.empty()) {
                converter.put_chain_in();
            }
            m++;
     }
}

func (converter : &mut ASTConverter) convertCSSOM(om : *mut CSSOM) {

    const builder = converter.builder
    const str = &mut converter.str
    var size = om.declarations.size()
    
    if(size == 0 && om.media_queries.empty()) {
        // no declarations, no media queries exist
        return;
    }

    const location = intrinsics::get_raw_location();

    if(!om.is_hashable()) {
        
        // not hashable, so we use random 32 bits as class name
        // maybe it should not be called 'hash'
        const hash = generate_random_32bit();
        
        // if(page.require_random_css_hash(hash))
        var ifStmt = builder.make_if_stmt(converter.make_require_random_css_hash_call(hash), converter.parent, location);
        var body = ifStmt.get_body();
        
        // page.set_random_css_hash(hash)
        body.push(converter.make_set_random_css_hash_call(hash));
        
        // redirect output to if body
        var oldVec = converter.vec;
        converter.vec = body;

        var className : char[10] = [];
        className[0] = '.'
        className[1] = 'r'
        base64_encode_32bit(hash, &mut className[2])
        className[8] = '{'
        className[9] = '\0'
        const total = builder.allocate_view(std::string_view(&className[0], 9u));
        const classView = std::string_view(total.data() + 1, 7u);

        converter.put_view_chain(total)
        om.className = classView
        
        var i : uint = 0
        while(i < size) {
            var decl = om.declarations.get(i)
            converter.convertDeclaration(decl)
            i++;
        }
        
        // Main class block end
        if(str.empty()) {
            converter.put_char_chain('}')
        } else {
            str.append('}')
            converter.put_chain_in();
        }

        // Media queries
        var media_size = om.media_queries.size()
        var j : uint = 0
        while(j < media_size) {
            var rule = om.media_queries.get(j)
            converter.writeMediaRule(rule, *str, classView)
            if(!str.empty()) {
                converter.put_chain_in();
            }
            j++;
        }

        // Nested Rules
        var parents = std::vector<std::string>();
        var root_sel = std::string();
        root_sel.append('.');
        root_sel.append_view(classView);
        parents.push(root_sel);

        var n_idx : uint = 0;
        const n_size = om.nested_rules.size();
        while(n_idx < n_size) {
            var rule = om.nested_rules.get(n_idx);
            converter.generate_css_recurse(rule, parents);
            n_idx++;
        }
        
        // restore output
        converter.vec = oldVec;
        converter.vec.push(ifStmt);

    } else {

        var i : uint = 0
        while(i < size) {
            var decl = om.declarations.get(i)
            converter.convertDeclaration(decl)
            i++;
        }

        // end the string here
        // calculate the hash before making any changes
        const hash = fnv1a_hash_32(str.data());
        
        // if(page.require_css_hash(hash))
        var ifStmt = builder.make_if_stmt(converter.make_require_css_hash_call(hash), converter.parent, location);
        var body = ifStmt.get_body();
        
        // page.set_css_hash(hash)
        body.push(converter.make_set_css_hash_call(hash));
        
        // redirect output to if body
        var oldVec = converter.vec;
        converter.vec = body;

        const totalView = allocate_view_with_classname(builder, *str, hash)
        om.className = std::string_view(totalView.data() + 1, 7u)
        const classView = om.className
        converter.put_append_css_value_chain(totalView)
        
        converter.vec = oldVec;
        converter.vec.push(ifStmt);

    }
}