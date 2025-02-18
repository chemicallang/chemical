import "@std/string.ch"
import "@std/string_view.ch"
import "../../utils/stdutils.ch"
import "../../utils/comptime_utils.ch"
import "@compiler/ASTBuilder.ch"
import "@std/hashing/fnv1.ch"
import "@cstd/common/integer_types.ch"
import "/ast/CSSLengthKind.ch"

func (str : &std::string) view() : std::string_view {
    return std::string_view(str.data(), str.size());
}

func make_char_chain(builder : *mut ASTBuilder, value : char) : *mut AccessChain {
    const location = compiler::get_raw_location();
    const chain = builder.make_access_chain(false, location)
    var chain_values = chain.get_values()
    var base = builder.make_identifier(std::string_view("html"), false, location);
    chain_values.push(base)
    var name : std::string_view = std::string_view("append")
    var id = builder.make_identifier(name, false, location);
    chain_values.push(id)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    const char_val = builder.make_char_value(value, location);
    args.push(char_val)
    const new_chain = builder.make_access_chain(true, location)
    var new_chain_values = new_chain.get_values();
    new_chain_values.push(call);
    return new_chain;
}

func make_value_chain(builder : *mut ASTBuilder, value : *mut Value, len : size_t) : *mut AccessChain {
    const location = compiler::get_raw_location();
    const chain = builder.make_access_chain(false, location)
    var chain_values = chain.get_values()
    var base = builder.make_identifier(std::string_view("html"), false, location);
    chain_values.push(base)
    var name : std::string_view
    if(len == 0) {
        name = std::string_view("append_char_ptr")
    } else {
        name = std::string_view("append_with_len")
    }
    var id = builder.make_identifier(name, false, location);
    chain_values.push(id)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    args.push(value)
    if(len != 0) {
        args.push(builder.make_number_value(len, location));
    }
    const new_chain = builder.make_access_chain(true, location)
    var new_chain_values = new_chain.get_values();
    new_chain_values.push(call);
    return new_chain;
}

func make_expr_chain_of(builder : *mut ASTBuilder, value : *mut Value) : *mut AccessChain {
    return make_value_chain(builder, value, 0);
}

func replace_value_in_expr_chain(builder : *mut ASTBuilder, chain : *mut AccessChain, new_value : *mut Value) {
    const values = chain.get_values();
    const last = values.get(values.size() - 1)
    const call = last as *mut FunctionCall
    const args = call.get_args();
    args.set(args.size() - 1, new_value)
}

func make_chain_of_view(builder : *mut ASTBuilder, view : &std::string_view) : *mut AccessChain {
    const location = compiler::get_raw_location();
    const value = builder.make_string_value(builder.allocate_view(view), location)
    return make_value_chain(builder, value, view.size());
}

func make_chain_of(builder : *mut ASTBuilder, str : &mut std::string) : *mut AccessChain {
    const location = compiler::get_raw_location();
    const value = builder.make_string_value(builder.allocate_view(str.view()), location)
    const size = str.size()
    str.clear();
    return make_value_chain(builder, value, size);
}

func put_link_wrap_chain(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, chain : *mut AccessChain) {
    var wrapped = builder.make_value_wrapper(chain, parent)
    wrapped.declare_and_link(&wrapped, resolver);
    vec.push(wrapped);
}

func put_char_chain(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, value : char) {
    const chain = make_char_chain(builder, value);
    put_link_wrap_chain(resolver, builder, vec, parent, chain);
}

func put_view_chain(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, view : &std::string_view) {
    const chain = make_chain_of_view(builder, view);
    var wrapped = builder.make_value_wrapper(chain, parent)
    wrapped.declare_and_link(&wrapped, resolver);
    vec.push(wrapped);
}

func put_chain_in(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, str : &mut std::string) {
    const chain = make_chain_of(builder, str);
    var wrapped = builder.make_value_wrapper(chain, parent)
    wrapped.declare_and_link(&wrapped, resolver);
    vec.push(wrapped);
}

func put_wrapping(builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, value : *mut Value) {
    const wrapped = builder.make_value_wrapper(value, parent)
    vec.push(wrapped);
}

var empty_string_val : *mut StringValue = null

func get_string_val(builder : *mut ASTBuilder) : *mut StringValue {
    if(empty_string_val != null) {
        return empty_string_val
    }
    const loc = compiler::get_raw_location();
    empty_string_val = builder.make_string_value(view(""), loc);
    return empty_string_val;
}

// we link the expression chain with a dummy string value (to already linked value to be linked twice)
// then we replace the value in expression chain
func put_wrapped_chemical_value_in(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, value : *mut Value) {
    // first we link the expression chain with dummy empty string value
    var chain = make_expr_chain_of(builder, get_string_val(builder));
    // link the chain
    chain.link(&chain, null, resolver)
    // replace the value
    replace_value_in_expr_chain(builder, chain, value)
    // then we replace the dummy string value with actual linked value
    put_wrapping(builder, vec, parent, chain)
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

func put_chemical_value_in(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, value_ptr : *mut Value) {
    var value = value_ptr;
    if(!value.link(&value, null, resolver)) {
        put_wrapping(builder, vec, parent, value);
        return;
    }
    const kind = value.getKind();
    if(kind == ValueKind.AccessChain) {
        const chain = value as *mut AccessChain
        const values = chain.get_values();
        const size = values.size();
        printf("received size of access chain %d\n", size)
        fflush(null)
        const last = values.get(size - 1)
        if(last.getKind() == ValueKind.FunctionCall) {
            if(is_func_call_ret_void(builder, last as *mut FunctionCall)) {
                put_wrapping(builder, vec, parent, value);
            } else {
                put_wrapped_chemical_value_in(resolver, builder, vec, parent, value);
            }
        } else {
            put_wrapped_chemical_value_in(resolver, builder, vec, parent, value);
        }
    } else if(kind == ValueKind.FunctionCall) {
        if(is_func_call_ret_void(builder, value as *mut FunctionCall)) {
            put_wrapping(builder, vec, parent, value);
        } else {
            put_wrapped_chemical_value_in(resolver, builder, vec, parent, value);
        }
    } else {
        put_wrapped_chemical_value_in(resolver, builder, vec, parent, value);
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
    var i = 0
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

func writeValue(value : &mut CSSValue, str : &mut std::string) {
    switch(value.kind) {

        CSSValueKind.Multiple => {
            const ptr = value.data as *mut CSSMultipleValues
            var i = 0;
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

        CSSValueKind.BorderRadius => {

            const ptr = value.data as *mut CSSBorderRadiusValueData
            writeBorderRadiusValueData(*ptr, str)

        }

        default => {
            printf("error no value found")
            fflush(null)
        }
    }
}

func convertDeclaration(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, decl : *mut CSSDeclaration, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, str : &mut std::string) {

    const propertyName = decl.property.name.data()
    str.append_with_len(propertyName, decl.property.name.size())
    str.append(':')

    // if(!str.empty()) {
    //     put_chain_in(resolver, builder, vec, parent, str);
    // }
    // const value = attr.value as *mut ChemicalAttributeValue
    // put_chemical_value_in(resolver, builder, vec, parent, value.value)

    // put_char_chain(resolver, builder, vec, parent, '\"');

    writeValue(decl.value, str)

    str.append(';')

    printf("written a declaration with key '%s'\n", propertyName)
    fflush(null)

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

func allocate_view_with_classname(builder : *mut ASTBuilder, str : &mut std::string) : std::string_view {
    // calculate the hash before making any changes
    const hash = fnv1a_hash_32(str.data());
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

func put_hashed_string_chain(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, str : &mut std::string) {
    put_view_chain(resolver, builder, vec, parent, allocate_view_with_classname(builder, str))
}

func put_class_name_chain(hash : uint32_t, prefix : char, resolver : *mut SymbolResolver, builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode) {
    var className : char[10] = {};
    className[0] = '.'
    className[1] = prefix
    base64_encode_32bit(hash, &className[2])
    className[8] = '{'
    className[9] = '\0'
    put_view_chain(resolver, builder, vec, parent, std::string_view(&className[0], 9u))
}

func rand() : int;

func generate_random_32bit() : uint32_t {
    return (rand() as uint32_t << 16) | rand() as uint32_t;
}

func convertCSSOM(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, om : *mut CSSOM, vec : *mut VecRef<ASTNode>, str : &mut std::string) {
    var size = om.declarations.size()
    if(size > 0) {
        if(om.has_dynamic_values) {
            const hash = generate_random_32bit();
            put_class_name_chain(hash, 'r', resolver, builder, vec, om.parent)
        }
        var i = 0
        while(i < size) {
            var decl = om.declarations.get(i)
            convertDeclaration(resolver, builder, decl, vec, om.parent, str)
            i++;
        }
        if(str.empty()) {
            put_char_chain(resolver, builder, vec, om.parent, '}')
        } else {
            // end the string here
            if(!om.has_dynamic_values) {
                put_hashed_string_chain(resolver, builder, vec, om.parent, str)
            } else {
                str.append('}')
                put_chain_in(resolver, builder, vec, om.parent, str);
            }
        }
    }
}