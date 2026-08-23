/**
 * Runtime css converter.
 *
 * Walks the shared css_parser AST (CSSOM) and re-emits CSS text into a
 * std::string. Reuses the pure string-writing functions from css_cbis
 * ASTConverter (writeValue, writeLength, writeColor, ...) so the emitted
 * text matches the compiler path, minus the AST-emission machinery
 * (page.append_css calls, class-name hashing).
 */
using namespace std;

func (str : &std::string) view() : std::string_view {
    return std::string_view(str.data(), str.size());
}

public struct CSSRuntimeConverter {
    var str : std::string
}

func (converter : &mut CSSRuntimeConverter) str_ref() : &mut std::string {
    return &mut converter.str;
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
        CSSLengthKind.LengthFR => {
            str.append_view(std::string_view("fr"))
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
        str.append_view(&ptr.value);
        str.append(')');
        return;
    }
    // writing the length
    str.append_view(&ptr.value)
    // writing the unit
    if(ptr.kind != CSSLengthKind.None && !writeUnitOfKind(str, ptr.kind)) {
        printf("unknown unit")
        fflush(null)
    }
}

func (converter : &mut CSSRuntimeConverter) writeBorderRadiusValueData(ptr : &mut CSSBorderRadiusValueData, str : &mut std::string) {

    if(ptr.first.kind != CSSValueKind.Unknown) {
        converter.writeValue(&mut ptr.first)
    }

    if(ptr.second.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(&mut ptr.second)
    }

    if(ptr.third.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(&mut ptr.third)
    }

    if(ptr.fourth.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(&mut ptr.fourth)
    }

    if(ptr.next != null) {
        str.append_view(std::string_view(" / "))
        converter.writeBorderRadiusValueData(&mut *ptr.next, str)
    }

}

func writeFontStyle(ptr : &CSSFontStyle, str : &mut std::string) {
    switch(ptr) {
        None => {}
        Keyword(keyword) => {
            str.append_view(&keyword.value)
        }
        Oblique(view) => {
            str.append_view(&view)
        }
    }
}

func writeFontWeight(ptr : &CSSFontWeight, str : &mut std::string) {
    switch(ptr) {
        None => {}
        Keyword(keyword) => {
            str.append(' ')
            str.append_view(&keyword.value)
        }
        Absolute(view) => {
            str.append(' ')
            str.append_view(&view)
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
        str.append_view(&*start)
        start++
    }
}

func (converter : &mut CSSRuntimeConverter) writeFontValueData(ptr : &CSSFontValueData, str : &mut std::string) {

    writeFontStyle(&ptr.style, str)

    if(ptr.fontVariant.kind != CSSKeywordKind.Unknown) {
        str.append(' ');
        str.append_view(&ptr.fontVariant.value)
    }

    writeFontWeight(&ptr.weight, str)

    if(ptr.stretch.kind != CSSKeywordKind.Unknown) {
        str.append(' ')
        str.append_view(&ptr.stretch.value)
    }

    if(ptr.size.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(&mut ptr.size)
    }

    if(ptr.lineHeight.kind != CSSValueKind.Unknown) {
        str.append('/')
        converter.writeValue(&mut ptr.lineHeight)
    }

    if(!ptr.family.families.empty()) {
        str.append(' ');
    }
    writeFontFamilyData(&mut ptr.family, str)

}

func (converter : &mut CSSRuntimeConverter) writeTextShadowValueData(value : &mut CSSTextShadowValueData, str : &mut std::string) {

    if(value.offsetX.kind != CSSValueKind.Unknown) {
        converter.writeValue(&mut value.offsetX)
    }

    if(value.offsetY.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(&mut value.offsetY)
    }

    if(value.blurRadius.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(&mut value.blurRadius)
    }

    if(value.color.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(&mut value.color)
    }

    if(value.next != null) {
        str.append(',')
        converter.writeTextShadowValueData(&mut *value.next, str)
    }

}

func (converter : &mut CSSRuntimeConverter) writeBoxShadowValueData(value : &mut CSSBoxShadowValueData, str : &mut std::string) {

    if(value.inset) {
        str.append_view(std::string_view("inset"))
    }

    if(value.offsetX.kind != CSSValueKind.Unknown) {
        if(value.inset) {
            str.append(' ')
        }
        converter.writeValue(&mut value.offsetX)
    }

    if(value.offsetY.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(&mut value.offsetY)
    }

    if(value.blurRadius.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(&mut value.blurRadius)
    }

    if(value.spreadRadius.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(&mut value.spreadRadius)
    }

    if(value.color.kind != CSSValueKind.Unknown) {
        str.append(' ')
        converter.writeValue(&mut value.color)
    }

    if(value.next != null) {
        str.append(',')
        converter.writeBoxShadowValueData(&mut *value.next, str)
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
    writeLengthOrNone(&mut ptr.red, str);
    writeLengthOrNoneNotFirst(&mut ptr.green, str);
    writeLengthOrNoneNotFirst(&mut ptr.blue, str);
    writeAlphaLengthOrNone(&mut ptr.alpha, str);
}

func writeHSLData(ptr : &mut CSSHSLColorData, str : &mut std::string) {
    writeLengthOrNone(&mut ptr.hue, str);
    writeLengthOrNoneNotFirst(&mut ptr.saturation, str);
    writeLengthOrNoneNotFirst(&mut ptr.lightness, str);
    writeAlphaLengthOrNone(&mut ptr.alpha, str);
}

func writeHWBData(ptr : &mut CSSHWBColorData, str : &mut std::string) {
    writeLengthOrNone(&mut ptr.hue, str);
    writeLengthOrNoneNotFirst(&mut ptr.whiteness, str);
    writeLengthOrNoneNotFirst(&mut ptr.blackness, str);
    writeAlphaLengthOrNone(&mut ptr.alpha, str);
}

func writeLABData(ptr : &mut CSSLABColorData, str : &mut std::string) {
    writeLengthOrNone(&mut ptr.lightness, str);
    writeLengthOrNoneNotFirst(&mut ptr.rgAxis, str);
    writeLengthOrNoneNotFirst(&mut ptr.byAxis, str);
    writeAlphaLengthOrNone(&mut ptr.alpha, str);
}

func writeLCHData(ptr : &mut CSSLCHColorData, str : &mut std::string) {
    writeLengthOrNone(&mut ptr.lightness, str);
    writeLengthOrNoneNotFirst(&mut ptr.chroma, str);
    writeLengthOrNoneNotFirst(&mut ptr.hue, str);
    writeAlphaLengthOrNone(&mut ptr.alpha, str);
}

func writeOKLABData(ptr : &mut CSSOKLABColorData, str : &mut std::string) {
    writeLengthOrNone(&mut ptr.lightness, str);
    writeLengthOrNoneNotFirst(&mut ptr.aAxis, str);
    writeLengthOrNoneNotFirst(&mut ptr.bAxis, str);
    writeAlphaLengthOrNone(&mut ptr.alpha, str);
}

func writeOKLCHData(ptr : &mut CSSOKLCHColorData, str : &mut std::string) {
    writeLengthOrNone(&mut ptr.lightness, str);
    writeLengthOrNoneNotFirst(&mut ptr.pChroma, str);
    writeLengthOrNoneNotFirst(&mut ptr.hue, str);
    writeAlphaLengthOrNone(&mut ptr.alpha, str);
}

func writeColor(ptr : &mut CSSColorValueData, str : &mut std::string) {
    switch(ptr.kind) {
        CSSColorKind.RGB => {
            str.append_view(std::string_view("rgb("))
            writeRGBData(&mut *ptr.value.rgbData, str)
            str.append(')')
        }
        CSSColorKind.RGBA => {
           str.append_view(std::string_view("rgba("))
           writeRGBData(&mut *ptr.value.rgbData, str)
           str.append(')')
        }
        CSSColorKind.HSL => {
            str.append_view(std::string_view("hsl("))
            writeHSLData(&mut *ptr.value.hslData, str)
            str.append(')')
        }
        CSSColorKind.HSLA => {
            str.append_view(std::string_view("hsla("))
            writeHSLData(&mut *ptr.value.hslData, str)
            str.append(')')
        }
        CSSColorKind.HWB => {
            str.append_view(std::string_view("hwb("))
            writeHWBData(&mut *ptr.value.hwbData, str)
            str.append(')')
        }
        CSSColorKind.LAB => {
            str.append_view(std::string_view("lab("))
            writeLABData(&mut *ptr.value.labData, str)
            str.append(')')
        }
        CSSColorKind.LCH => {
            str.append_view(std::string_view("lch("))
            writeLCHData(&mut *ptr.value.lchData, str)
            str.append(')')
        }
        CSSColorKind.OKLAB => {
            str.append_view(std::string_view("oklab("))
            writeOKLABData(&mut *ptr.value.oklabData, str)
            str.append(')')
        }
        CSSColorKind.OKLCH => {
            str.append_view(std::string_view("oklch("))
            writeOKLCHData(&mut *ptr.value.oklchData, str)
            str.append(')')
        }
        CSSColorKind.COLOR => {
            // TODO:
        }
        CSSColorKind.VAR => {
            str.append_view(std::string_view("var("))
            str.append_view(&ptr.value.view)
            str.append(')')
        }

        CSSColorKind.Unknown => {
            return;
        }

        default => {
            str.append_view(&ptr.value.view)
        }

    }
}

func writeLinearEasing(ptr : &mut CSSLinearEasingPoint, str : &mut std::string) {
    var has_value_before = false;
    if(ptr.point.kind != CSSLengthKind.Unknown) {
        writeLength(&mut ptr.point, str)
        has_value_before = true;
    }
    if(ptr.start.kind != CSSLengthKind.Unknown) {
        if(has_value_before) {
            str.append(' ');
        } else {
            has_value_before = true;
        }
        writeLength(&mut ptr.start, str)
    }
    if(ptr.stop.kind != CSSLengthKind.Unknown) {
        if(has_value_before) {
            str.append(' ');
        } else {
            has_value_before = true;
        }
        writeLength(&mut ptr.stop, str)
    }
    if(ptr.next != null) {
        str.append(',')
        str.append(' ')
        writeLinearEasing(&mut *ptr.next, str)
    }
}

func writeCubicBezierEasing(ptr : &mut CSSCubicBezierEasingData, str : &mut std::string) {
    writeLength(&mut ptr.x1, str)
    str.append(',')
    writeLength(&mut ptr.y1, str)
    str.append(',')
    writeLength(&mut ptr.x2, str)
    str.append(',')
    writeLength(&mut ptr.y2, str)
}

func writeStepsEasing(ptr : &mut CSSStepsEasingData, str : &mut std::string) {
    writeLength(&mut ptr.step, str)
    str.append(',')
    str.append_view(&ptr.position.value)
}

func writeEasing(ptr : &mut CSSEasingFunction, str : &mut std::string) {
    switch(ptr.kind) {
        CSSKeywordKind.Var => {
            str.append_view(std::string_view("var("))
            str.append_view(&ptr.data.keyword.value)
            str.append(')')
        }
        CSSKeywordKind.Ease, CSSKeywordKind.EaseIn, CSSKeywordKind.EaseOut,
        CSSKeywordKind.EaseInOut, CSSKeywordKind.StepStart, CSSKeywordKind.StepEnd => {
            str.append_view(&ptr.data.keyword.value)
        }
        CSSKeywordKind.Linear => {
            str.append_view(std::string_view("linear"))
            if(ptr.data.linear != null) {
                str.append('(')
                writeLinearEasing(&mut *ptr.data.linear, str)
                str.append(')')
            }
        }
        CSSKeywordKind.CubicBezier => {
            str.append_view(std::string_view("cubic-bezier("))
            writeCubicBezierEasing(&mut *ptr.data.bezier, str)
            str.append(')')
        }
        CSSKeywordKind.Steps => {
            str.append_view(std::string_view("steps("))
            writeStepsEasing(&mut *ptr.data.steps, str)
            str.append(')')
        }
    }
}

func writeTransition(ptr : &mut CSSTransitionValueData, str : &mut std::string) {

    var has_value_before = false;

    if(!ptr.property.empty()) {
        str.append_view(&ptr.property)
        has_value_before = true;
    }

    if(ptr.duration.kind != CSSLengthKind.Unknown) {
        if(has_value_before) {
            str.append(' ')
        } else {
            has_value_before = true;
        }
        writeLength(&mut ptr.duration, str)
    }

    if(ptr.easing.kind != CSSKeywordKind.Unknown) {
        if(has_value_before) {
            str.append(' ')
        } else {
            has_value_before = true;
        }
        writeEasing(&mut ptr.easing, str)
    }

    if(ptr.delay.kind != CSSLengthKind.Unknown) {
        if(has_value_before) {
            str.append(' ')
        } else {
            has_value_before = true;
        }
        writeLength(&mut ptr.delay, str)
    }

    if(ptr.behavior.kind != CSSLengthKind.Unknown) {
        if(has_value_before) {
            str.append(' ')
        } else {
            has_value_before = true;
        }
        str.append_view(&ptr.behavior.value)
    }

    if(ptr.next != null) {
        str.append(',')
        writeTransition(&mut *ptr.next, str)
    }

}

func (converter : &mut CSSRuntimeConverter) writeAnimationValueData(value : &mut CSSAnimationValueData, str : &mut std::string) {
    var has_val = false
    if(!value.name.empty()) {
        str.append_view(&value.name)
        has_val = true
    }
    if(value.duration.kind != CSSLengthKind.Unknown) {
        if(has_val) str.append(' ')
        writeLength(&mut value.duration, str)
        has_val = true
    }
    if(value.easing.kind != CSSKeywordKind.Unknown) {
        if(has_val) str.append(' ')
        writeEasing(&mut value.easing, str)
        has_val = true
    }
    if(value.delay.kind != CSSLengthKind.Unknown) {
        if(has_val) str.append(' ')
        writeLength(&mut value.delay, str)
        has_val = true
    }
    if(value.iterationCount.kind != CSSValueKind.Unknown) {
        if(has_val) str.append(' ')
        converter.writeValue(&mut value.iterationCount)
        has_val = true
    }
    if(value.next != null) {
        str.append(',')
        str.append(' ')
        converter.writeAnimationValueData(&mut *value.next, str)
    }
}

func (converter : &mut CSSRuntimeConverter) writeTransformNode(ptr : &mut CSSTransformLengthNode, str : &mut std::string) {

    converter.writeValue(&mut ptr.value)

    if(ptr.next != null && ptr.next.value.kind != CSSValueKind.Unknown) {
        str.append(',')
        converter.writeTransformNode(&mut *ptr.next, str)
    }

}

func (converter : &mut CSSRuntimeConverter) writeTransformValueData(ptr : &mut CSSTransformValueData, str : &mut std::string) {

    str.append_view(&ptr.transformFunction.value)
    str.append('(')
    if(ptr.node != null) {
        converter.writeTransformNode(&mut *ptr.node, str)
    } else {
        str.append_view(std::string_view("none"))
    }
    str.append(')')
    if(ptr.next != null) {
        str.append(' ')
        converter.writeTransformValueData(&mut *ptr.next, str)
    }

}

func writeBackgroundImageUrl(url : &mut UrlData, str : &mut std::string) {

    if(url.is_source) {
        str.append_view(std::string_view("src"))
    } else {
        str.append_view(std::string_view("url"))
    }
    str.append('(');
    str.append_view(&url.value)
    str.append(')');

}

func (converter : &mut CSSRuntimeConverter) writeLinearGradientData(data : &mut LinearGradientData, str : &mut std::string) {

    writeLength(&mut data.angle, str)
    if(data.angle.kind != CSSLengthKind.Unknown) {
        str.append(',');
    }

    if(data.to1.kind != CSSKeywordKind.Unknown) {
        str.append_view(std::string_view("to "))
        str.append_view(&data.to1.value)

        if(data.to2.kind != CSSKeywordKind.Unknown) {
            str.append(' ');
            str.append_view(&data.to2.value)
        }

        str.append(',');

    }

    var start = data.color_stop_list.data()
    const end = start + data.color_stop_list.size()
    while(start != end) {

        converter.writeValue(&mut start.hint)

        converter.writeValue(&mut start.stop.color)

        if(start.stop.length.kind != CSSValueKind.Unknown) {

            str.append(' ');
            converter.writeValue(&mut start.stop.length)

            if(start.stop.optSecLength.kind != CSSValueKind.Unknown) {
                str.append(' ');
                converter.writeValue(&mut start.stop.optSecLength)
            }

        }

        start++;

        if(start != end) str.append(',');

    }

}

func (converter : &mut CSSRuntimeConverter) writeRadialGradientData(data : &mut RadialGradientData, str : &mut std::string) {
    // Write shape, size, position
    // Syntax: [ <ending-shape> || <size> ]? [ at <position> ]? , <color-stop-list>
    
    var has_shape_or_size = false
    
    if(data.shape.kind != CSSKeywordKind.Unknown) {
        str.append_view(&data.shape.value)
        has_shape_or_size = true
    }
    
    if(data.size.extent.kind != CSSKeywordKind.Unknown) {
        if(has_shape_or_size) str.append(' ')
        str.append_view(&data.size.extent.value)
        has_shape_or_size = true
    } else if(data.size.length.kind != CSSValueKind.Unknown) {
        if(has_shape_or_size) str.append(' ')
        converter.writeValue(&mut data.size.length)
        has_shape_or_size = true
    }
    
    if(data.position.kind != CSSValueKind.Unknown) {
        if(has_shape_or_size) str.append(' ')
        str.append_view(std::string_view("at "))
        converter.writeValue(&mut data.position)
        has_shape_or_size = true // effectively
    }
    
    if(has_shape_or_size) {
        str.append(',')
    }
    
    var start = data.color_stop_list.data()
    const end = start + data.color_stop_list.size()
    while(start != end) {

        converter.writeValue(&mut start.hint)

        converter.writeValue(&mut start.stop.color)

        if(start.stop.length.kind != CSSValueKind.Unknown) {

            str.append(' ');
            converter.writeValue(&mut start.stop.length)

            if(start.stop.optSecLength.kind != CSSValueKind.Unknown) {
                str.append(' ');
                converter.writeValue(&mut start.stop.optSecLength)
            }

        }

        start++;

        if(start != end) str.append(',');

    }
}

func (converter : &mut CSSRuntimeConverter) writeConicGradientData(data : &mut ConicGradientData, str : &mut std::string) {
    // Write from <angle> at <position>
    
    var has_from_or_at = false
    
    if(data.from.kind != CSSValueKind.Unknown) {
        str.append_view(std::string_view("from "))
        converter.writeValue(&mut data.from)
        has_from_or_at = true
    }
    
    if(data.at.kind != CSSValueKind.Unknown) {
        if(has_from_or_at) str.append(' ')
        str.append_view(std::string_view("at "))
        converter.writeValue(&mut data.at)
        has_from_or_at = true
    }
    
    if(has_from_or_at) {
        str.append(',')
    }

    var start = data.color_stop_list.data()
    const end = start + data.color_stop_list.size()
    while(start != end) {

        converter.writeValue(&mut start.hint)

        converter.writeValue(&mut start.stop.color)

        if(start.stop.length.kind != CSSValueKind.Unknown) {

            str.append(' ');
            converter.writeValue(&mut start.stop.length)

            if(start.stop.optSecLength.kind != CSSValueKind.Unknown) {
                str.append(' ');
                converter.writeValue(&mut start.stop.optSecLength)
            }

        }

        start++;

        if(start != end) str.append(',');

    }
}

func (converter : &mut CSSRuntimeConverter) writeBackgroundImageData(ptr : &mut BackgroundImageData, str : &mut std::string) {

    if(ptr.is_url) {
        writeBackgroundImageUrl(&mut ptr.url, str)
    } else {
        switch(ptr.gradient.kind) {
            CSSGradientKind.RepeatingLinear => {
                str.append_view(std::string_view("repeating-linear-gradient("))
                converter.writeLinearGradientData(&mut *(ptr.gradient.data as *mut LinearGradientData), str)
                str.append(')')
            }
            CSSGradientKind.RepeatingRadial => {
                str.append_view(std::string_view("repeating-radial-gradient("))
                converter.writeRadialGradientData(&mut *(ptr.gradient.data as *mut RadialGradientData), str)
                str.append(')')
            }
            CSSGradientKind.RepeatingConic => {
                str.append_view(std::string_view("repeating-conic-gradient("))
                converter.writeConicGradientData(&mut *(ptr.gradient.data as *mut ConicGradientData), str)
                str.append(')')
            }
            CSSGradientKind.Linear => {
                str.append_view(std::string_view("linear-gradient("))
                converter.writeLinearGradientData(&mut *(ptr.gradient.data as *mut LinearGradientData), str)
                str.append(')')
            }
            CSSGradientKind.Radial => {
                str.append_view(std::string_view("radial-gradient("))
                converter.writeRadialGradientData(&mut *(ptr.gradient.data as *mut RadialGradientData), str)
                str.append(')')
            }
            CSSGradientKind.Conic => {
                str.append_view(std::string_view("conic-gradient("))
                converter.writeConicGradientData(&mut *(ptr.gradient.data as *mut ConicGradientData), str)
                str.append(')')
            }
            default => {
                // Unknown or not a gradient
            }
        }
    }

}

func (converter : &mut CSSRuntimeConverter) writeBackgroundValueData(ptr : &mut CSSBackgroundValueData, str : &mut std::string) {
    
    var start = ptr.layers.data();
    const end = start + ptr.layers.size();
    while(start != end) {

        if(start != ptr.layers.data()) {
            str.append(',');
            str.append(' ');
        }
        
        var has_val = false;

        if(start.image.kind != CSSValueKind.Unknown) {
            converter.writeValue(&mut start.image);
            has_val = true;
        }

        if(start.positionX.kind != CSSValueKind.Unknown) {

            if(has_val) str.append(' ');
            converter.writeValue(&mut start.positionX);
            if(start.positionY.kind != CSSValueKind.Unknown) {
                str.append(' ');
                converter.writeValue(&mut start.positionY);
            }

            if(start.size.kind != CSSValueKind.Unknown) {
                str.append('/');
                converter.writeValue(&mut start.size);
            }
            has_val = true;
        }

        if(start.repeat.kind != CSSValueKind.Unknown) {
            if(has_val) str.append(' ');
            converter.writeValue(&mut start.repeat);
            has_val = true;
        }

        if(start.attachment.kind != CSSValueKind.Unknown) {
            if(has_val) str.append(' ');
            converter.writeValue(&mut start.attachment);
            has_val = true;
        }

        if(start.origin.kind != CSSValueKind.Unknown) {
            if(has_val) str.append(' ');
            converter.writeValue(&mut start.origin);
            has_val = true;
        }

        if(start.clip.kind != CSSValueKind.Unknown) {
            if(has_val) str.append(' ');
            converter.writeValue(&mut start.clip);
            has_val = true;
        }


        start++;
    }

    if(ptr.color.kind != CSSValueKind.Unknown) {
        if(!ptr.layers.empty()) {
            str.append(' ');
        }
        converter.writeValue(&mut ptr.color);
    }

}

func (converter : &mut CSSRuntimeConverter) writeCalcExpression(expr : &CSSCalcExpression, str : &mut std::string) {
    switch(expr.kind) {
        CSSCalcExpressionKind.Literal => {
            const val = expr.data as *mut CSSValue
            converter.writeValue(&mut *val)
        }
        CSSCalcExpressionKind.Operation => {
            const data = expr.data as *mut CSSCalcOperationData
            converter.writeCalcExpression(&data.left, str)
            str.append(' ')
            str.append(data.op)
            str.append(' ')
            converter.writeCalcExpression(&data.right, str)
        }
        CSSCalcExpressionKind.Group => {
            const inner = expr.data as *mut CSSCalcExpression
            str.append('(')
            converter.writeCalcExpression(&*inner, str)
            str.append(')')
        }
    }
}

func (converter : &mut CSSRuntimeConverter) writeOutlineValueData(ptr : &mut CSSOutlineValueData, str : &mut std::string) {
    const has_style = ptr.style.kind != CSSValueKind.Unknown;
    const has_color = ptr.color.kind != CSSValueKind.Unknown;

    // width
    if(ptr.width.kind != CSSValueKind.Unknown) {
        converter.writeValue(&mut ptr.width)
        if(has_style || has_color) {
            str.append(' ')
        }
    }

    // style
    if(has_style) {
        converter.writeValue(&mut ptr.style)
        if(has_color) {
            str.append(' ')
        }
    }

    // color
    if(has_color) {
        converter.writeValue(&mut ptr.color)
    }
}

func (converter : &mut CSSRuntimeConverter) writeBackdropFilterValueData(ptr : &mut CSSBackdropFilterValueData, str : &mut std::string) {
    str.append_view(&ptr.function.value)
    if(!ptr.arguments.empty()) {
        str.append('(')
        var i : uint = 0
        while(i < ptr.arguments.size()) {
            if(i > 0) str.append(' ')
            converter.writeValue(&mut *ptr.arguments.get_ptr(i))
            i++
        }
        str.append(')')
    }

    if(ptr.next != null) {
        str.append(' ')
        converter.writeBackdropFilterValueData(&mut *ptr.next, str)
    }
}

func (converter : &mut CSSRuntimeConverter) append_hex(val : uint) {
    var str = converter.str_ref()
    const hex = "0123456789ABCDEF"
    if (val == 0) {
        str.append('0');
        return;
    }
    unsafe var buf : [16]char;
    var bi = 0;
    while(val > 0) {
        buf[bi++] = hex[val & 0xF]
        val >>= 4;
    }
    while(bi > 0) {
        str.append(buf[--bi])
    }
}

func (converter : &mut CSSRuntimeConverter) writeCssString(text : std::string_view) {
    var i = 0u;
    var str = converter.str_ref()
    while(i < text.size()) {
        const c1 = (text.data()[i] as uint) & 0xFF;
        if (c1 < 0x80) {
            const c = c1 as char;
            if (c == '\'' || c == '\\') {
                str.append('\\');
            }
            str.append(c);
            i++;
        } else if ((c1 & 0xE0) == 0xC0) {
            if (i + 1 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & (0xFF as uint);
                const codepoint = ((c1 & (0x1F as uint)) << 6u) | (c2 & (0x3F as uint));
                str.append('\\');
                converter.append_hex(codepoint);
                i += 2;
                if(i < text.size()) str.append(' ');
            } else { i++; }
        } else if ((c1 & 0xF0) == 0xE0) {
            if (i + 2 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & (0xFF as uint);
                const c3 = (text.data()[i+2] as uint) & (0xFF as uint);
                const codepoint = ((c1 & (0x0F as uint)) << 12u) | ((c2 & (0x3F as uint)) << 6u) | (c3 & (0x3F as uint));
                str.append('\\');
                converter.append_hex(codepoint);
                i += 3;
                if(i < text.size()) str.append(' ');
            } else { i++; }
        } else if ((c1 & 0xF8) == 0xF0) {
            if (i + 3 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & (0xFF as uint);
                const c3 = (text.data()[i+2] as uint) & (0xFF as uint);
                const c4 = (text.data()[i+3] as uint) & (0xFF as uint);
                const codepoint = ((c1 & (0x07 as uint)) << 18u) | ((c2 & (0x3F as uint)) << 12u) | ((c3 & (0x3F as uint)) << 6u) | (c4 & (0x3F as uint));
                str.append('\\');
                converter.append_hex(codepoint);
                i += 4;
                if(i < text.size()) str.append(' ');
            } else { i++; }
        } else {
            i++;
        }
    }
}

func (converter : &mut CSSRuntimeConverter) writeValue(value : &mut CSSValue) {

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
                converter.writeValue(&mut *value_ptr)
                if(i < last) {
                    str.append(' ');
                }
                i++;
            }
        }

        CSSValueKind.Pair => {

            const pair = value.data as *mut CSSValuePair
            converter.writeValue(&mut pair.first)
            if(pair.second.kind != CSSValueKind.Unknown) {
                str.append(' ');
                converter.writeValue(&mut pair.second)
            }

        }

        CSSValueKind.Keyword => {
            var ptr = value.data as *mut CSSKeywordValueData
            str.append_view(&ptr.value)
            return;
        }

        CSSValueKind.Length => {
            const ptr = value.data as *mut CSSLengthValueData
            writeLength(&mut *ptr, str)
        }

        CSSValueKind.Color => {

            var ptr = value.data as *mut CSSColorValueData

            writeColor(&mut *ptr, str)

        }

        CSSValueKind.SingleLengthFunctionCall => {
            const ptr = value.data as *SingleLengthFuncCall
            str.append_view(&ptr.name.value)
            str.append('(')
            writeLength(&mut ptr.length, str)
            str.append(')')
        }

        CSSValueKind.Font => {

            var ptr = value.data as *mut CSSFontValueData
            converter.writeFontValueData(&*ptr, str)

        }

        CSSValueKind.FontFamily => {

            var ptr = value.data as *mut CSSFontFamily
            writeFontFamilyData(&mut *ptr, str)

        }

        CSSValueKind.Border => {

            const ptr = value.data as *mut CSSBorderValueData

            const has_style = ptr.style.kind != CSSValueKind.Unknown;
            const has_color = ptr.color.kind != CSSValueKind.Unknown;

            // width
            if(ptr.width.kind != CSSValueKind.Unknown) {
                converter.writeValue(&mut ptr.width)
                if(has_style) {
                    str.append(' ')
                }
            }

            // style
            if(has_style) {
                converter.writeValue(&mut ptr.style)
                if(has_color) {
                    str.append(' ')
                }
            }

            // color
            if(has_color) {
                converter.writeValue(&mut ptr.color)
            }

        }

        CSSValueKind.BoxShadow => {

            const ptr = value.data as *mut CSSBoxShadowValueData

            if(ptr.isEmpty()) {

                str.append_view(std::string_view("none"))

            } else {

                converter.writeBoxShadowValueData(&mut *ptr, str)

            }

        }

        CSSValueKind.TextShadow => {

            const ptr = value.data as *mut CSSTextShadowValueData

            if(ptr.isEmpty()) {

                str.append_view(std::string_view("none"))

            } else {

                converter.writeTextShadowValueData(&mut *ptr, str)

            }

        }

        CSSValueKind.Transform => {

            const ptr = value.data as *mut CSSTransformValueData
            if(ptr.node == null && ptr.transformFunction.kind != CSSKeywordKind.Perspective) {
                str.append_view(std::string_view("none"))
            } else {
                converter.writeTransformValueData(&mut *ptr, str)
            }

        }

        CSSValueKind.BorderRadius => {

            const ptr = value.data as *mut CSSBorderRadiusValueData
            converter.writeBorderRadiusValueData(&mut *ptr, str)

        }

        CSSValueKind.Transition => {

            const ptr = value.data as *mut CSSTransitionValueData
            writeTransition(&mut *ptr, str)
        }

        CSSValueKind.TransitionTimingFunction => {

            const ptr = value.data as *mut CSSEasingFunction
            writeEasing(&mut *ptr, str)

        }

        CSSValueKind.MultipleBackgroundImage => {

            const ptr = value.data as *mut MultipleBackgroundImageData
            var start = ptr.images.data()
            const end = start + ptr.images.size()
            while(start != end) {
                converter.writeBackgroundImageData(&mut *start, str)
                start++;
            }

        }

        CSSValueKind.BackgroundImage => {

            const ptr = value.data as *mut BackgroundImageData
            converter.writeBackgroundImageData(&mut *ptr, str)

        }

        CSSValueKind.Background => {
            const ptr = value.data as *mut CSSBackgroundValueData
            converter.writeBackgroundValueData(&mut *ptr, str);
        }

        CSSValueKind.TextDecoration => {
            const ptr = value.data as *mut CSSTextDecorationValueData
            var first = true;
            if(ptr.line.kind != CSSValueKind.Unknown) {
                converter.writeValue(&mut ptr.line)
                first = false;
            }
            if(ptr.style.kind != CSSValueKind.Unknown) {
                if(!first) str.append(' ')
                converter.writeValue(&mut ptr.style)
                first = false;
            }
            if(ptr.color.kind != CSSValueKind.Unknown) {
                if(!first) str.append(' ')
                converter.writeValue(&mut ptr.color)
                first = false;
            }
            if(ptr.thickness.kind != CSSValueKind.Unknown) {
                if(!first) str.append(' ')
                converter.writeValue(&mut ptr.thickness)
            }
        }
        
        CSSValueKind.Outline => {
             var ptr = value.data as *mut CSSOutlineValueData
             converter.writeOutlineValueData(&mut *ptr, str)
        }

        CSSValueKind.BackdropFilter => {
             var ptr = value.data as *mut CSSBackdropFilterValueData
             converter.writeBackdropFilterValueData(&mut *ptr, str)
        }

        CSSValueKind.GridRepeat => {
            const ptr = value.data as *mut GridRepeatData
            str.append_view(std::string_view("repeat("))
            converter.writeValue(&mut ptr.count)
            str.append(',')
            str.append(' ')
            var i : uint = 0;
            const size = ptr.tracks.size();
            const last = size - 1;
            while(i < size) {
                const value_ptr = ptr.tracks.get_ptr(i);
                converter.writeValue(&mut *value_ptr)
                if(i < last) {
                    str.append(' ');
                }
                i++;
            }
            str.append(')')
        }

        CSSValueKind.GridLine => {
            const ptr = value.data as *mut GridLineData
            if(ptr.is_span) {
                str.append_view(std::string_view("span "))
            }
            converter.writeValue(&mut ptr.value)
        }

        CSSValueKind.Calc => {
            const ptr = value.data as *mut CSSCalcValueData
            str.append_view(std::string_view("calc("))
            converter.writeCalcExpression(&ptr.expression, str)
            str.append(')')
        }

        CSSValueKind.ChemicalValue => {
            // chemical values cannot be re-emitted at runtime without the compiler
        }

        CSSValueKind.String => {
            const ptr = value.data as *mut CSSStringValueData
            str.append('\'')
            converter.writeCssString(ptr.value)
            str.append('\'')
        }

        CSSValueKind.Raw => {
            const ptr = value.data as *mut CSSRawValueData
            str.append_view(&ptr.value)
        }

        CSSValueKind.Animation => {
            const ptr = value.data as *mut CSSAnimationValueData
            converter.writeAnimationValueData(&mut *ptr, str)
        }

        CSSValueKind.ListStyle => {
            const ptr = value.data as *mut CSSListStyleValueData
            var first = true
            if(ptr.type.kind != CSSKeywordKind.Unknown) {
                str.append_view(&ptr.type.value)
                first = false
            }
            if(ptr.position.kind != CSSKeywordKind.Unknown) {
                if(!first) str.append(' ')
                str.append_view(&ptr.position.value)
                first = false
            }
            if(ptr.image.kind != CSSValueKind.Unknown) {
                if(!first) str.append(' ')
                converter.writeValue(&mut ptr.image)
            }
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

func (converter : &mut CSSRuntimeConverter) convertDeclaration(decl : *mut CSSDeclaration) {

    const str = &mut converter.str;

    str.append_view(&decl.property.name)
    str.append(':')

    converter.writeValue(&mut decl.value);
    
    if(decl.important) {
        str.append_view(std::string_view(" !important"));
    }

    str.append(';')

}


func append_media_root_selector(out : &mut std::string, className : std::string_view) {
    if(className.empty()) return;
    if(className.data()[0] == '.') {
        out.append_view(&className);
    } else {
        out.append('.');
        out.append_view(&className);
    }
}

func (converter : &mut CSSRuntimeConverter) writeMediaNestedRule(rule : *mut CSSNestedRule, str : &mut std::string, parent_selectors : &mut std::vector<std::string>) {
    var current_selectors = std::vector<std::string>();

    if(rule.selector != null) {
        var i : uint = 0;
        while(i < rule.selector.selectors.size()) {
            var sel = rule.selector.selectors.get(i);
            if(has_ampersand_complex(sel) && !parent_selectors.empty()) {
                var p : uint = 0;
                while(p < parent_selectors.size()) {
                    var resolved = std::string();
                    serialize_complex(sel, &mut resolved, parent_selectors.get_ptr(p).view());
                    current_selectors.push(resolved);
                    p++;
                }
            } else if(!parent_selectors.empty()) {
                // No &: CSS nesting semantics — implicit descendant of parent(s)
                var p : uint = 0;
                while(p < parent_selectors.size()) {
                    var resolved = std::string();
                    resolved.append_view(parent_selectors.get_ptr(p).view());
                    resolved.append(' ');
                    serialize_complex(sel, &mut resolved, std::string_view("&"));
                    current_selectors.push(resolved);
                    p++;
                }
            } else {
                var resolved = std::string();
                serialize_complex(sel, &mut resolved, std::string_view("&"));
                current_selectors.push(resolved);
            }
            i++;
        }
    }

    if(!rule.declarations.empty()) {
        var i : uint = 0;
        while(i < current_selectors.size()) {
            if(i > 0) str.append_view(", ");
            str.append_view(current_selectors.get_ptr(i).view());
            i++;
        }
        str.append_view(std::string_view(" { "));
        i = 0;
        while(i < rule.declarations.size()) {
            converter.convertDeclaration(rule.declarations.get(i));
            i++;
        }
        str.append_view(std::string_view(" } "));
    }

    var nested_i : uint = 0;
    while(nested_i < rule.nested_rules.size()) {
        converter.writeMediaNestedRule(rule.nested_rules.get(nested_i), str, &mut current_selectors);
        nested_i++;
    }
}

func (converter : &mut CSSRuntimeConverter) writeKeyframesRule(rule : *mut CSSKeyframesRule, str : &mut std::string) {
    str.append_view(std::string_view("@keyframes "))
    str.append_view(&rule.name)
    str.append_view(std::string_view(" { "))

    var i : uint = 0
    while(i < rule.keyframes.size()) {
        var keyframe = rule.keyframes.get(i)
        str.append_view(&keyframe.selector)
        str.append_view(std::string_view(" { "))
        
        var j : uint = 0
        while(j < keyframe.declarations.size()) {
            converter.convertDeclaration(keyframe.declarations.get(j))
            j++
        }
        
        str.append_view(std::string_view(" } "))
        i++
    }

    str.append_view(std::string_view("}"))
}

func (converter : &mut CSSRuntimeConverter) writeMediaRule(rule : *mut CSSMediaRule, str : &mut std::string, className : std::string_view) {
    str.append_view(std::string_view("@media "))
    
    // Serialize the media query list AST
    converter.writeMediaQueryList(rule.queryList, str)
    
    str.append_view(std::string_view(" { "))

    var parent_selectors = std::vector<std::string>();
    if(!className.empty()) {
        var root_selector = std::string();
        append_media_root_selector(&mut root_selector, className);
        parent_selectors.push(root_selector);

        if(!rule.declarations.empty()) {
            append_media_root_selector(str, className);
            str.append_view(std::string_view(" { "))
            var i : uint = 0
            while(i < rule.declarations.size()) {
                converter.convertDeclaration(rule.declarations.get(i))
                i++;
            }
            str.append_view(std::string_view(" } "))
        }
    }

    var nested_i : uint = 0;
    while(nested_i < rule.nested_rules.size()) {
        converter.writeMediaNestedRule(rule.nested_rules.get(nested_i), str, &mut parent_selectors);
        nested_i++;
    }

    str.append_view(std::string_view("}"))
}


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
        out.append_view(&replacement);
        return;
    }
    if(s.kind == SimpleSelectorKind.Class) out.append('.');
    if(s.kind == SimpleSelectorKind.Id) out.append('#');
    if(s.kind == SimpleSelectorKind.PseudoClass) {
        out.append(':');
    }
    if(s.kind == SimpleSelectorKind.PseudoElement) {
        out.append_view("::");
    }
    out.append_view(&s.value);
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



// Serialize media type to string
func writeMediaType(type : MediaType, customType : std::string_view, str : &mut std::string) {
    switch(type) {
        MediaType.All => {
            str.append_view(std::string_view("all"))
        }
        MediaType.Screen => {
            str.append_view(std::string_view("screen"))
        }
        MediaType.Print => {
            str.append_view(std::string_view("print"))
        }
        MediaType.Speech => {
            str.append_view(std::string_view("speech"))
        }
        MediaType.Unknown => {
            if(!customType.empty()) {
                str.append_view(&customType)
            }
        }
    }
}

// Serialize comparison operator to string
func writeComparisonOperator(op : MediaFeatureComparison, str : &mut std::string) {
    switch(op) {
        MediaFeatureComparison.Equal => {
            str.append('=')
        }
        MediaFeatureComparison.LessThan => {
            str.append('<')
        }
        MediaFeatureComparison.LessThanEqual => {
            str.append_view(std::string_view("<="))
        }
        MediaFeatureComparison.GreaterThan => {
            str.append('>')
        }
        MediaFeatureComparison.GreaterThanEqual => {
            str.append_view(std::string_view(">="))
        }
        MediaFeatureComparison.None => {
            // No operator
        }
    }
}

// Serialize media feature to string
func (converter : &mut CSSRuntimeConverter) writeMediaFeature(feature : *mut MediaFeature, str : &mut std::string) {
    str.append('(')
    
    // Check if this is range syntax
    if(feature.leftOp != MediaFeatureComparison.None) {
        // Range syntax: leftValue op feature [op rightValue]
        if(feature.leftValue.kind != CSSValueKind.Unknown) {
            converter.writeValue(&mut feature.leftValue)
            str.append(' ')
            writeComparisonOperator(feature.leftOp, str)
            str.append(' ')
        }
        
        str.append_view(&feature.name)
        
        if(feature.rightOp != MediaFeatureComparison.None && feature.rightValue.kind != CSSValueKind.Unknown) {
            str.append(' ')
            writeComparisonOperator(feature.rightOp, str)
            str.append(' ')
            converter.writeValue(&mut feature.rightValue)
        }
    } else {
        // Legacy syntax: feature-name or feature-name: value
        str.append_view(&feature.name)
        
        if(feature.value.kind != CSSValueKind.Unknown) {
            str.append(':')
            str.append(' ')
            converter.writeValue(&mut feature.value)
        }
    }
    
    str.append(')')
}

// Serialize media condition to string (recursive)
func (converter : &mut CSSRuntimeConverter) writeMediaCondition(condition : *mut MediaCondition, str : &mut std::string) {
    if(condition == null) {
        return
    }
    
    if(condition.isNot) {
        str.append_view(std::string_view("not "))
    }
    
    if(condition.feature != null) {
        converter.writeMediaFeature(condition.feature, str)
    }
    
    if(condition.next != null) {
        switch(condition.op) {
            MediaConditionOp.And => {
                str.append_view(std::string_view(" and "))
            }
            MediaConditionOp.Or => {
                str.append_view(std::string_view(" or "))
            }
            MediaConditionOp.None => {
                // No operator
            }
        }
        converter.writeMediaCondition(condition.next, str)
    }
}

// Serialize a single media query to string
func (converter : &mut CSSRuntimeConverter) writeMediaQuery(query : *mut MediaQuery, str : &mut std::string) {
    // Write modifier
    switch(query.modifier) {
        MediaModifier.Only => {
            str.append_view(std::string_view("only "))
        }
        MediaModifier.Not => {
            str.append_view(std::string_view("not "))
        }
        MediaModifier.None => {
            // No modifier
        }
    }
    
    // Write media type
    if(query.mediaType != MediaType.Unknown || !query.customType.empty()) {
        writeMediaType(query.mediaType, query.customType, str)
        
        if(query.condition != null) {
            str.append_view(std::string_view(" and "))
        }
    }
    
    // Write condition
    if(query.condition != null) {
        converter.writeMediaCondition(query.condition, str)
    }
}

// Serialize media query list to string
func (converter : &mut CSSRuntimeConverter) writeMediaQueryList(queryList : *mut MediaQueryList, str : &mut std::string) {
    var i : uint = 0
    const size = queryList.queries.size()
    
    while(i < size) {
        if(i > 0) {
            str.append_view(std::string_view(", "))
        }
        
        var query = queryList.queries.get(i)
        converter.writeMediaQuery(query, str)
        
        i++
    }
}

/**
 * Runtime entry point: walks a parsed CSSOM and appends the CSS text into `out`.
 *
 * Root-level declarations (no selector) are emitted inside a bare `{ ... }`
 * block; nested rules, media queries and keyframes keep their selectors.
 */
public func convert_css_root(root : *mut CSSOM, out : &mut std::string) {
    var converter = CSSRuntimeConverter { str : std::string() }

    // root declarations -> bare block (no selector available at runtime)
    if(root.declarations.size() > 0) {
        converter.str.append('{')
        converter.str.append(' ')
        var i : uint = 0
        while(i < root.declarations.size()) {
            converter.convertDeclaration(root.declarations.get(i))
            i++
        }
        converter.str.append(' ')
        converter.str.append('}')
    }

    // nested rules
    var parents = std::vector<std::string>()
    var n : uint = 0
    while(n < root.nested_rules.size()) {
        converter.writeMediaNestedRule(root.nested_rules.get(n), &mut converter.str, &mut parents)
        n++
    }

    // media rules
    var m : uint = 0
    while(m < root.media_queries.size()) {
        converter.writeMediaRule(root.media_queries.get(m), &mut converter.str, std::string_view(""))
        m++
    }

    // keyframes
    var k : uint = 0
    while(k < root.keyframes.size()) {
        converter.writeKeyframesRule(root.keyframes.get(k), &mut converter.str)
        k++
    }

    const v = converter.str.view()
    out.append_view(&v)
}
