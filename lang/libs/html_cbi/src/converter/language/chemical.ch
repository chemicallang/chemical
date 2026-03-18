func (converter : &mut ASTConverter) make_char_chain(value : char) : *mut FunctionCallNode {
    const builder = converter.builder
    const support = converter.support;
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);

    var name : std::string_view
    var fnPtr : *mut ASTNode
    if(converter.in_head) {
        name = std::string_view("append_head_char")
        fnPtr = support.appendHeadCharFn
    } else {
        name = std::string_view("append_html_char")
        fnPtr = support.appendHtmlCharFn
    }

    var id = builder.make_identifier(name, fnPtr, false, location);
    const chain = builder.make_access_chain(std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    const char_val = builder.make_char_value(value, location);
    args.push(char_val)
    return call;
}

// this makes the call 'page.fn_name(value)'
// value is the argument here
func (converter : &mut ASTConverter) make_value_call_with(value : *mut Value, fn_name : std::string_view, fnPtr : *mut ASTNode) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var id = builder.make_identifier(fn_name, fnPtr, false, location);
    const chain = builder.make_access_chain(std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

// value is the self arg here
// this makes the call 'value.fn_name(page)'
func (converter : &mut ASTConverter) make_call_inside(value : *mut Value, fn_name : std::string_view, fnPtr : *mut ASTNode) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var id = builder.make_identifier(fn_name, fnPtr, false, location);
    const chain = builder.make_access_chain(std::span<*mut Value>([ value as *mut Value, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(base)
    return call;
}

func (converter : &mut ASTConverter) make_char_ptr_value_call(value : *mut Value) : *mut FunctionCallNode {
    if(converter.in_head) {
        return converter.make_value_call_with(value, std::string_view("append_head_char_ptr"), converter.support.appendHeadCharPtrFn)
    }
    return converter.make_value_call_with(value, std::string_view("append_html_char_ptr"), converter.support.appendHtmlCharPtrFn)
}

func (converter : &mut ASTConverter) make_char_value_call(value : *mut Value) : *mut FunctionCallNode {
    if(converter.in_head) {
        return converter.make_value_call_with(value, std::string_view("append_head_char"), converter.support.appendHeadCharFn)
    }
    return converter.make_value_call_with(value, std::string_view("append_html_char"), converter.support.appendHtmlCharFn)
}

func (converter : &mut ASTConverter) make_integer_value_call(value : *mut Value) : *mut FunctionCallNode {
    if(converter.in_head) {
        return converter.make_value_call_with(value, std::string_view("append_head_integer"), converter.support.appendHeadIntFn)
    }
    return converter.make_value_call_with(value, std::string_view("append_html_integer"), converter.support.appendHtmlIntFn)
}

func (converter : &mut ASTConverter) make_uinteger_value_call(value : *mut Value) : *mut FunctionCallNode {
    if(converter.in_head) {
        return converter.make_value_call_with(value, std::string_view("append_head_uinteger"), converter.support.appendHeadUIntFn)
    }
    return converter.make_value_call_with(value, std::string_view("append_html_uinteger"), converter.support.appendHtmlUIntFn)
}

func (converter : &mut ASTConverter) make_float_value_call(value : *mut Value) : *mut FunctionCallNode {
    if(converter.in_head) {
        return converter.make_value_call_with(value, std::string_view("append_head_float"), converter.support.appendHeadFloatFn)
    }
    return converter.make_value_call_with(value, std::string_view("append_html_float"), converter.support.appendHtmlFloatFn)
}

func (converter : &mut ASTConverter) make_double_value_call(value : *mut Value) : *mut FunctionCallNode {
    if(converter.in_head) {
        return converter.make_value_call_with(value, std::string_view("append_head_double"), converter.support.appendHeadDoubleFn)
    }
    return converter.make_value_call_with(value, std::string_view("append_html_double"), converter.support.appendHtmlDoubleFn)
}

func (converter : &mut ASTConverter) make_value_call(value : *mut Value, len : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var name : std::string_view
    if(converter.in_head) {
        name = std::string_view("append_head")
    } else {
        name = std::string_view("append_html")
    }
    var node : *mut ASTNode
    if(converter.in_head) {
        node = converter.support.appendHeadFn
    } else {
        node = converter.support.appendHtmlFn
    }
    var id = builder.make_identifier(name, node, false, location);
    const chain = builder.make_access_chain(std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    args.push(builder.make_ubigint_value(len, location));
    return call;
}


func (converter : &mut ASTConverter) make_require_component_call(hash : size_t) : *mut FunctionCall {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var value = builder.make_ubigint_value(hash, location)
    const support = converter.support;
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("require_component"), support.requireComponentFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func (converter : &mut ASTConverter) make_set_component_hash_call(hash : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var value = builder.make_ubigint_value(hash, location)
    return converter.make_value_call_with(value, std::string_view("set_component_hash"), converter.support.setComponentHashFn)
}

func (converter : &mut ASTConverter) make_chain_of(str : &mut std::string) : *mut FunctionCallNode {
    const location = intrinsics::get_raw_location();
    const builder = converter.builder;
    const value = builder.make_string_value(builder.allocate_view(str.view()), location)
    const size = str.size()
    str.clear();
    return converter.make_value_call(value, size);
}

func (converter : &mut ASTConverter) put_char_chain(value : char) {
    const chain = converter.make_char_chain(value);
    converter.vec.push(chain);
}

func (converter : &mut ASTConverter) put_chain_in() {
    const chain = converter.make_chain_of(converter.str);
    converter.vec.push(chain);
}

func (converter : &mut ASTConverter) put_wrapping(value : *mut Value) {
    const wrapped = converter.builder.make_value_wrapper(value, converter.parent)
    converter.vec.push(wrapped);
}

var empty_string_val : *mut StringValue = null

func get_string_val(builder : *mut ASTBuilder) : *mut StringValue {
    if(empty_string_val != null) {
        return empty_string_val
    }
    const loc = intrinsics::get_raw_location();
    empty_string_val = builder.make_string_value(view(""), loc);
    return empty_string_val;
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

func make_ssr_text_val(builder : *mut ASTBuilder, val : &std::string_view, textNode : *mut ASTNode, location : ubigint) : *mut Value {
    const structVal = builder.make_struct_value(textNode, location);
    structVal.add_value(std::string_view("data"), builder.make_string_value(val, location));
    structVal.add_value(std::string_view("size"), builder.make_ubigint_value(val.size(), location));
    return structVal;
}