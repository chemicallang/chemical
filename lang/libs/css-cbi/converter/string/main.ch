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

func convertValue(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, value : &mut CSSValue, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, str : &mut std::string) {

    switch(value.kind) {
        CSSValueKind.Inherit => {
            var view = std::string_view("inherit")
            str.append_with_len(view.data(), view.size())
            return;
        }
        CSSValueKind.Initial => {
            var view = std::string_view("initial")
            str.append_with_len(view.data(), view.size())
            return;
        }
        CSSValueKind.Unset => {
            var view = std::string_view("unset")
            str.append_with_len(view.data(), view.size())
            return;
        }
        CSSValueKind.Length => {
            // writing the length
            var ptr = value.data as *mut CSSLengthValueData
            str.append_with_len(ptr.value.data(), ptr.value.size())

            // writing the unit
            if(!writeUnitOfKind(str, ptr.kind)) {
                printf("unknown unit")
                fflush(null)
            }

        }
        CSSValueKind.NamedColor => {

            var ptr = value.data as *mut CSSIdentifierData
            str.append_with_len(ptr.value.data(), ptr.value.size())

        }
        default => {
            printf("error no value found")
            fflush(null)
        }
    }

}

func convertDeclaration(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, decl : *mut CSSDeclaration, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, str : &mut std::string) {

    str.append_with_len(decl.property.name.data(), decl.property.name.size())
    str.append(':')

    // if(!str.empty()) {
    //     put_chain_in(resolver, builder, vec, parent, str);
    // }
    // const value = attr.value as *mut ChemicalAttributeValue
    // put_chemical_value_in(resolver, builder, vec, parent, value.value)

    // put_char_chain(resolver, builder, vec, parent, '\"');

    convertValue(resolver, builder, decl.value, vec, parent, str)

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