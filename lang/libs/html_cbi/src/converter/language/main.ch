
func (str : &std::string) view() : std::string_view {
    return std::string_view(str.data(), str.size());
}

struct ASTConverter {

    var builder : *mut ASTBuilder

    var support : *mut SymResSupport

    var vec : *mut VecRef<ASTNode>

    var parent : *mut ASTNode

    var str : std::string

    var in_head : bool = false

}

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
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
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
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
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
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ value as *mut ChainValue, id ]), location)
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
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    args.push(builder.make_ubigint_value(len, location));
    return call;
}

func (converter : &mut ASTConverter) is_string_type(type : *mut BaseType) : bool {
    const kind = type.getKind()
    if(kind == BaseTypeKind.String) return true;
    if(kind == BaseTypeKind.Pointer) {
        const ptrType = type as *PointerType;
        const child = ptrType.getChildType()
        if(child != null && child.getKind() == BaseTypeKind.IntN) {
            const intNType = child as *IntNType
            if(intNType.get_intn_type_kind() == IntNTypeKind.Char) {
                return true
            }
        }
    }
    return false;
}

@extern
public func rand() : int;

func generate_random_32bit() : uint32_t {
    return (rand() as uint32_t << 16) | rand() as uint32_t;
}

func (converter : &mut ASTConverter) escapeHtml(text : std::string_view) {
    var i = 0u;
    var str = &mut converter.str
    while(i < text.size()) {
        const c1 = (text.data()[i] as uint) & 0xFF;
        if (c1 < 0x80) {
            const c = c1 as char;
            switch(c) {
                '&' => str.append_view("&amp;")
                '<' => str.append_view("&lt;")
                '>' => str.append_view("&gt;")
                '"' => str.append_view("&quot;")
                '\'' => str.append_view("&#39;")
                default => str.append(c)
            }
            i++;
        } else if ((c1 & 0xE0) == 0xC0) {
            if (i + 1 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const codepoint = ((c1 & 0x1F) << 6) | (c2 & 0x3F);
                str.append_view("&#");
                str.append_uinteger(codepoint as ubigint);
                str.append(';');
                i += 2;
            } else { i++; }
        } else if ((c1 & 0xF0) == 0xE0) {
            if (i + 2 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const c3 = (text.data()[i+2] as uint) & 0xFF;
                const codepoint = ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
                str.append_view("&#");
                str.append_uinteger(codepoint as ubigint);
                str.append(';');
                i += 3;
            } else { i++; }
        } else if ((c1 & 0xF8) == 0xF0) {
            if (i + 3 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const c3 = (text.data()[i+2] as uint) & 0xFF;
                const c4 = (text.data()[i+3] as uint) & 0xFF;
                const codepoint = ((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
                str.append_view("&#");
                str.append_uinteger(codepoint as ubigint);
                str.append(';');
                i += 4;
            } else { i++; }
        } else {
            i++;
        }
    }
}

func fnv1a_hash_32(view : std::string_view) : uint32_t {
    var hash = 2166136261u;
    for (var i = 0u; i < view.size(); i++) {
        hash ^= view.get(i) as uint32_t;
        hash *= 16777619u;
    }
    return hash;
}

func (converter : &mut ASTConverter) make_require_component_call(hash : size_t) : *mut FunctionCall {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var value = builder.make_ubigint_value(hash, location)
    const support = converter.support;
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("require_component"), support.requireComponentFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
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

func (converter : &mut ASTConverter) put_by_node(type : *mut BaseType, node : *mut ASTNode, value : *mut Value) {
    switch(node.getKind()) {
        ASTNodeKind.StructDecl, ASTNodeKind.UnionDecl, ASTNodeKind.VariantDecl => {
            var fnName = if(converter.in_head) std::string_view("writeToPageHead") else std::string_view("writeToPageBody")
            const writeFn = node.child(fnName)
            if(writeFn == null) {
                converter.put_by_type(type, value)
                return;
            }
            if(writeFn.getKind() != ASTNodeKind.FunctionDecl) {
                converter.put_by_type(type, value)
                return;
            }
            const fn = writeFn as *mut FunctionDeclaration;
            const chain = converter.make_call_inside(value, fnName, writeFn);
            converter.vec.push(chain)
        }
        ASTNodeKind.TypealiasStmt => {
            const stmt = node as *mut TypealiasStatement
            const actual_type = stmt.getActualType();
            converter.put_by_type(actual_type, value);
        }
        default => {
            converter.put_by_type(type, value);
        }
    }
}

func (converter : &mut ASTConverter) put_ref_node(node : *mut ASTNode, type : *mut BaseType, value : *mut Value) {
    switch(node.getKind()) {
        ASTNodeKind.TypealiasStmt => {
            const stmt = node as *mut TypealiasStatement
            const actual_type = stmt.getActualType();
            converter.put_ref_child_type(actual_type, value);
        }
        ASTNodeKind.StructDecl, ASTNodeKind.UnionDecl, ASTNodeKind.VariantDecl => {
            converter.put_by_node(type, node, value)
        }
        default => {
            const loc = intrinsics::get_raw_location();
            const deref = converter.builder.make_dereference_value(value, type, loc);
            converter.put_by_type(type, deref);
        }
    }
}

func (converter : &mut ASTConverter) put_ref_child_type(childType : *mut BaseType, value : *mut Value) {
    switch(childType.getKind()) {
        BaseTypeKind.Linked => {
            const linked = childType as *mut LinkedType;
            const node = linked.getLinkedNode();
            converter.put_ref_node(node, childType, value)
        }
        BaseTypeKind.Generic => {
            const generic = childType as *mut GenericType;
            const linked = generic.getLinkedType();
            const node = linked.getLinkedNode();
            converter.put_ref_node(node, childType, value)
        }
        default => {
            const loc = intrinsics::get_raw_location();
            const deref = converter.builder.make_dereference_value(value, childType, loc);
            converter.put_by_type(childType, deref);
        }
    }
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
        BaseTypeKind.ExpressiveString => {
            if(value.getKind() == ValueKind.ExpressiveString) {
                const exprString = value as *mut ExpressiveString
                const values = exprString.getValues()
                const size = values.size()
                var i = 0u;
                while(i < size) {
                    const ptr = values.get(i)
                    converter.put_by_type(ptr.getType(), ptr);
                    i++;
                }
            } else {
                // TODO: error out, cannot handle comptime functions that return expressive strings yet !
            }
        }
        BaseTypeKind.Linked => {
            const linked = type as *mut LinkedType;
            const node = linked.getLinkedNode();
            converter.put_by_node(type, node, value);
        }
        BaseTypeKind.Generic => {
            const generic = type as *mut GenericType;
            const linked = generic.getLinkedType();
            const node = linked.getLinkedNode();
            converter.put_by_node(type, node, value);
        }
        BaseTypeKind.Reference => {
            const refType = type as *mut ReferenceType
            const childType = refType.getChildType()
            converter.put_ref_child_type(childType, value)
        }
        default => {
            converter.put_wrapped_chemical_value_in(value);
        }
    }
}

func (converter : &mut ASTConverter) put_chemical_value_in(value : *mut Value) {
    converter.put_by_type(value.getType(), value)
}

func (converter : &mut ASTConverter) convertHtmlAttribute(attr : *mut HtmlAttribute) {
    var str = &mut converter.str
    var builder = converter.builder
    var parent = converter.parent
    var vec = converter.vec

    str.append(' ')
    str.append_view(attr.name)
    if(attr.value != null) {
        str.append('=')
        switch(attr.value.kind) {
            AttributeValueKind.Text, AttributeValueKind.Number => {
                const value = attr.value as *mut TextAttributeValue
                str.append('\"')
                converter.escapeHtml(std::string_view(value.text.data() + 1, value.text.size() - 2))
                str.append('\"')
            }
            AttributeValueKind.Chemical => {
                str.append('"');
                if(!str.empty()) {
                    converter.put_chain_in();
                }
                const value = attr.value as *mut ChemicalAttributeValue
                converter.put_chemical_value_in(value.value)
                str.append('"');
            }
            AttributeValueKind.ChemicalValues => {
                if(!str.empty()) {
                    converter.put_chain_in();
                }
                converter.put_char_chain('"');
                const value = attr.value as *mut ChemicalAttributeValues
                const size = value.values.size();
                const last = size - 1;
                var i : uint = 0;
                while(i < size) {
                    converter.put_chemical_value_in(value.values.get(i))
                    if(i != last) {
                        converter.put_char_chain(' ');
                    }
                    i++;
                }
                converter.put_char_chain('"');
            }
        }
    }
}

func (converter : &mut ASTConverter) emit_append_html_call(value : *mut Value, len : size_t) {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const support = converter.support
    
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location)
    var name = if (converter.in_head) std::string_view("append_head") else std::string_view("append_html")
    var fnPtr = if (converter.in_head) support.appendHeadFn else support.appendHtmlFn
    var id = builder.make_identifier(name, fnPtr, false, location)
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base as *mut ChainValue, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    
    var args = call.get_args()
    args.push(value)
    args.push(builder.make_ubigint_value(len, location))
    
    converter.vec.push(call as *mut ASTNode)
}

func (converter : &mut ASTConverter) emit_append_html_from_str(s : &mut std::string) {
    if (s.empty()) return;
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const size = s.size()
    const value = builder.make_string_value(builder.allocate_view(s.view()), location)
    s.clear()
    converter.emit_append_html_call(value, size)
}

func (converter : &mut ASTConverter) convertHtmlComponent(element : *mut HtmlElement) {
    // 0. Flush any pending HTML
    converter.put_chain_in()

    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const signature = element.componentSignature
    
    // 1. Generate the hash based on component name
    const hash = signature.functionNode.getEncodedLocation()
    
    // 2. Generate the if(page.require_component(hash)) block
    var requireCall = converter.make_require_component_call(hash as size_t)
    var ifStmt = builder.make_if_stmt(requireCall as *mut Value, converter.parent, location)
    var body = ifStmt.get_body()
    
    // Inside if: page.set_component_hash(hash)
    body.push(converter.make_set_component_hash_call(hash as size_t))
    
    // Inside if: ComponentFunction(page)
    var base = builder.make_identifier(signature.name, signature.functionNode as *mut ASTNode, false, location)
    var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
    var call = builder.make_function_call_node(base as *mut ChainValue, converter.parent, location)
    call.get_args().push(pageId as *mut Value)
    body.push(call as *mut ASTNode)
    
    converter.vec.push(ifStmt as *mut ASTNode)
    
    // 3. Generate script block
    var s = &mut converter.str
    s.append_view("<script>")
    
    if(signature.mountStrategy == MountStrategy.Preact) {
        // Preact Mount Strategy
        s.append_view("$_pm(document.currentScript, ")
        s.append_view(signature.name)
        s.append_view(", {")
    } else if(signature.mountStrategy == MountStrategy.React) {
        // React Mount Strategy
        s.append_view("$_rm(document.currentScript, ")
        s.append_view(signature.name)
        s.append_view(", {")
    } else if(signature.mountStrategy == MountStrategy.Solid) {
        // Solid Mount Strategy
        s.append_view("$_sm(document.currentScript, ")
        s.append_view(signature.name)
        s.append_view(", {")
    } else {
        // Default Mount Strategy
        s.append_view("$_dm(document.currentScript, ")
        s.append_view(signature.name)
        s.append_view(", {")
    }
    
    const attrs = element.attributes.size()
    for (var i : uint = 0; i < attrs; i++) {
        if (i > 0) s.append_view(", ")
        const attr = element.attributes.get(i)
        s.append_view(attr.name)
        s.append_view(": ")
        
        if (attr.value != null) {
            switch(attr.value.kind) {
                AttributeValueKind.Text, AttributeValueKind.Number => {
                    const val = attr.value as *mut TextAttributeValue
                    s.append_view(val.text)
                }
                AttributeValueKind.Chemical => {
                    const val = attr.value as *mut ChemicalAttributeValue
                    const type = val.value.getType()
                    const is_str = converter.is_string_type(type)
                    
                    if(is_str) {
                         s.append('"')
                    }
                    
                    converter.emit_append_html_from_str(*s)
                    
                    converter.put_chemical_value_in(val.value)
                    
                    if(is_str) {
                        converter.put_char_chain('"')
                    }
                }
                AttributeValueKind.ChemicalValues => {
                    converter.emit_append_html_from_str(*s)
                    const valuesNode = attr.value as *mut ChemicalAttributeValues
                    converter.emit_append_html_call(builder.make_string_value(builder.allocate_view(std::string_view("\"")), location), 1)
                    for (var j : uint = 0; j < valuesNode.values.size(); j++) {
                        if (j > 0) converter.emit_append_html_call(builder.make_string_value(builder.allocate_view(std::string_view(" ")), location), 1)
                        converter.put_chemical_value_in(valuesNode.values.get(j))
                    }
                    converter.emit_append_html_call(builder.make_string_value(builder.allocate_view(std::string_view("\"")), location), 1)
                }
            }
        } else {
            s.append_view("true")
        }
    }
    
    s.append_view("});")

    s.append_view("</script>")
    converter.emit_append_html_from_str(*s)
}

func (converter : &mut ASTConverter) convertHtmlChild(child : *mut HtmlChild) {

    var str = &mut converter.str
    var builder = converter.builder
    var parent = converter.parent
    var vec = converter.vec

    switch(child.kind) {
        HtmlChildKind.Text => {
            var text = child as *mut HtmlText
            converter.escapeHtml(text.value);
        }
        HtmlChildKind.Element => {
            var element = child as *mut HtmlElement

            if(element.componentSignature != null) {
                converter.convertHtmlComponent(element)
                return
            }
            
            if(element.name.equals(std::string_view("head"))) {
                if(!str.empty()) {
                    converter.put_chain_in();
                }
                const old = converter.in_head
                converter.in_head = true
                var i : uint = 0;
                var s = element.children.size();
                while(i < s) {
                    var nested_child = element.children.get(i)
                    converter.convertHtmlChild(nested_child)
                    i++;
                }
                if(!str.empty()) {
                    converter.put_chain_in();
                }
                converter.in_head = old
                return;
            }

            str.append('<')
            str.append_view(element.name)

            // putting attributes
            var a : uint = 0;
            var attrs = element.attributes.size()
            while(a < attrs) {
                var attr = element.attributes.get(a)
                converter.convertHtmlAttribute(attr);
                a++
            }

            if(element.isSelfClosing) {
                str.append('/');
            }

            str.append('>')

            // doing children
            var i : uint = 0;
            var s = element.children.size();
            while(i < s) {
                var nested_child = element.children.get(i)
                converter.convertHtmlChild(nested_child)
                i++;
            }

            if(!element.isSelfClosing) {
                str.append('<')
                str.append('/')
                str.append_view(element.name)
                str.append('>')
            }

        }
        HtmlChildKind.Comment => {
            // we can just skip comments
            // TODO provide an option to write out comments
        }
        HtmlChildKind.ChemicalNode => {
            if(!str.empty()) {
                converter.put_chain_in();
            }
            const chem_child = child as *mut HtmlChemNodeChild
            converter.vec.push(chem_child.node)
        }
        HtmlChildKind.ChemicalValue => {
            if(!str.empty()) {
                converter.put_chain_in();
            }
            const chem_child = child as *mut HtmlChemValueChild
            converter.put_chemical_value_in(chem_child.value)
        }
        HtmlChildKind.IfStatement => {
            if(!str.empty()) {
                converter.put_chain_in();
            }
            const if_stmt = child as *mut HtmlIfStatement
            const loc = intrinsics::get_raw_location();
            
            var emit_if = converter.builder.make_if_stmt(if_stmt.condition, converter.parent, loc);
            
            // convert body
            const old_vec = converter.vec
            converter.vec = emit_if.get_body()
            var i : uint = 0;
            var s = if_stmt.body.size();
            while(i < s) {
                converter.convertHtmlChild(if_stmt.body.get(i))
                i++;
            }
            if(!converter.str.empty()) {
                converter.put_chain_in();
            }

            // else ifs
            i = 0;
            s = if_stmt.else_ifs.size();
            while(i < s) {
                const elseif = if_stmt.else_ifs.get(i);
                converter.vec = emit_if.add_else_if(elseif.condition);
                var j : uint = 0;
                var sj = elseif.body.size();
                while(j < sj) {
                    converter.convertHtmlChild(elseif.body.get(j))
                    j++;
                }
                if(!converter.str.empty()) {
                    converter.put_chain_in();
                }
                i++;
            }

            // else body
            if(!if_stmt.else_body.empty()) {
                converter.vec = emit_if.add_else_body();
                i = 0;
                s = if_stmt.else_body.size();
                while(i < s) {
                    converter.convertHtmlChild(if_stmt.else_body.get(i))
                    i++;
                }
                if(!converter.str.empty()) {
                    converter.put_chain_in();
                }
            }

            converter.vec = old_vec
            converter.vec.push(emit_if)
        }
    }
}

func (converter : &mut ASTConverter) convertHtmlRoot(root : *mut HtmlRoot) {
    if(!root.children.empty()) {
        var i = 0;
        const total = root.children.size();
        while(i < total) {
            const child = root.children.get(i as size_t)
            converter.convertHtmlChild(child);
            i++;
        }
        if(!converter.str.empty()) {
            converter.put_chain_in();
        }
    }
}