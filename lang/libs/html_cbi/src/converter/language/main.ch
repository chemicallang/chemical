
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

func strip_js_string_quotes(value : std::string_view) : std::string_view {
    if(value.size() >= 2) {
        const first = value.data()[0];
        const last = value.data()[value.size() - 1];
        if((first == '"' || first == '\'' || first == '`') && first == last) {
            return std::string_view(value.data() + 1, value.size() - 2);
        }
    }
    return value;
}

func find_attribute(element : *mut HtmlElement, name : std::string_view) : *mut HtmlAttribute {
    for(var i : uint = 0; i < element.attributes.size(); i++) {
        const attr = element.attributes.get(i);
        if(attr.name.equals(name)) return attr;
    }
    return null;
}

func is_mergeable_attr_name(name : std::string_view) : bool {
    return name.equals(view("class")) || name.equals(view("className")) || name.equals(view("style"));
}

func append_escaped_single_quoted(out : &mut std::string, value : std::string_view) {
    for(var i : uint = 0; i < value.size(); i++) {
        const c = value.get(i);
        switch(c) {
            '\\' => out.append_view("\\\\")
            '\'' => out.append_view("\\'")
            '\n' => out.append_view("\\n")
            '\r' => out.append_view("\\r")
            '\t' => out.append_view("\\t")
            default => out.append(c)
        }
    }
}

func (converter : &mut ASTConverter) resolve_and_put_prop(element : *mut HtmlElement, path : std::string_view) {
    const builder = converter.builder
    const loc = intrinsics::get_raw_location()
    
    var dotPos : uint = 0;
    while(dotPos < path.size() && path.data()[dotPos] != '.') dotPos++;

    var attrName = if(dotPos == path.size()) path else std::string_view(path.data(), dotPos);
    
    var attr = find_attribute(element, attrName);
    if(attr == null) return;
    
    if(attr.value == null) return;
    
    if(dotPos == path.size()) {
        switch(attr.value.kind) {
            AttributeValueKind.Text, AttributeValueKind.Number => {
                const val = attr.value as *mut TextAttributeValue;
                converter.str.append_view(strip_js_string_quotes(val.text));
            }
            AttributeValueKind.Chemical => {
                const val = attr.value as *mut ChemicalAttributeValue;
                converter.put_chemical_value_in(val.value);
            }
            AttributeValueKind.ChemicalValues => {
                const val = attr.value as *mut ChemicalAttributeValues;
                for(var i : uint = 0; i < val.values.size(); i++) {
                    converter.put_chemical_value_in(val.values.get(i));
                }
            }
        }
    } else {
        if(attr.value.kind == AttributeValueKind.Chemical) {
            const val = attr.value as *mut ChemicalAttributeValue;
            var current : *mut Value = val.value;
            
            var remStart = dotPos + 1;
            while(remStart < path.size()) {
                var nextDot = remStart;
                while(nextDot < path.size() && path.data()[nextDot] != '.') nextDot++;
                
                var part = std::string_view(path.data() + remStart, nextDot - remStart);
                
                var id = builder.make_identifier(part, null, false, loc);
                const chain = builder.make_access_chain(std::span<*mut ChainValue>([ current as *mut ChainValue, id as *mut ChainValue ]), loc)
                current = chain as *mut Value;
                
                if(nextDot == path.size()) break;
                remStart = nextDot + 1;
            }
            converter.put_chemical_value_in(current);
        }
    }
}

func (converter : &mut ASTConverter) merged_attr_should_emit(element : *mut HtmlElement, merged : *mut MergedAttribute) : bool {
    if(merged == null) return false;
    for(var i : uint = 0; i < merged.segments.size(); i++) {
        const seg = merged.segments.get(i);
        switch(seg.kind) {
            MergedAttrSegmentKind.Text => {
                if(!seg.value.empty()) return true;
            }
            MergedAttrSegmentKind.ChemicalValue => { return true; }
            MergedAttrSegmentKind.PropAccess => {
                var dotPos : uint = 0;
                while(dotPos < seg.value.size() && seg.value.data()[dotPos] != '.') dotPos++;
                var attrName = if(dotPos == seg.value.size()) seg.value else std::string_view(seg.value.data(), dotPos);
                const attr = find_attribute(element, attrName);
                if(attr != null && attr.value != null) return true;
            }
            MergedAttrSegmentKind.SpreadAttr => {
                var attr = find_attribute(element, merged.name);
                if(attr == null && merged.name.equals(view("class"))) {
                    attr = find_attribute(element, view("className"));
                }
                if(attr != null && attr.value != null) return true;
            }
        }
    }
    return false;
}

func (converter : &mut ASTConverter) emit_merged_attribute(element : *mut HtmlElement, merged : *mut MergedAttribute) {
    if(merged == null) return;
    if(!converter.merged_attr_should_emit(element, merged)) return;

    var sep = if(merged.name.equals(view("style"))) ';' else ' ';

    var s = &mut converter.str;
    s.append(' ');
    s.append_view(merged.name);
    s.append_view("=\"");

    var hasValue = false;
    for(var i : uint = 0; i < merged.segments.size(); i++) {
        const seg = merged.segments.get(i);
        switch(seg.kind) {
            MergedAttrSegmentKind.Text => {
                if(seg.value.empty()) continue;
                if(hasValue) s.append(sep);
                converter.escapeHtml(seg.value);
                hasValue = true;
            }
            MergedAttrSegmentKind.PropAccess => {
                if(hasValue) s.append(sep);
                converter.emit_append_html_from_str(*s);
                converter.resolve_and_put_prop(element, seg.value);
                hasValue = true;
            }
            MergedAttrSegmentKind.ChemicalValue => {
                if(hasValue) s.append(sep);
                converter.emit_append_html_from_str(*s);
                converter.put_chemical_value_in(seg.chemicalValue);
                hasValue = true;
            }
            MergedAttrSegmentKind.SpreadAttr => {
                var attr = find_attribute(element, merged.name);
                if(attr == null && merged.name.equals(view("class"))) {
                    attr = find_attribute(element, view("className"));
                }
                if(attr == null || attr.value == null) continue;
                switch(attr.value.kind) {
                    AttributeValueKind.Text, AttributeValueKind.Number => {
                        const val = attr.value as *mut TextAttributeValue;
                        const txt = strip_js_string_quotes(val.text);
                        if(txt.empty()) continue;
                        if(hasValue) s.append(sep);
                        converter.escapeHtml(txt);
                        hasValue = true;
                    }
                    AttributeValueKind.Chemical => {
                        if(hasValue) s.append(sep);
                        converter.emit_append_html_from_str(*s);
                        const val = attr.value as *mut ChemicalAttributeValue;
                        converter.put_chemical_value_in(val.value);
                        hasValue = true;
                    }
                    AttributeValueKind.ChemicalValues => {
                        const vals = attr.value as *mut ChemicalAttributeValues;
                        for(var vi : uint = 0; vi < vals.values.size(); vi++) {
                            if(hasValue) s.append(sep);
                            converter.emit_append_html_from_str(*s);
                            converter.put_chemical_value_in(vals.values.get(vi));
                            hasValue = true;
                        }
                    }
                }
            }
        }
    }

    s.append('\"');
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

func generate_random_32bit() : uint32_t {
    return (rand() as uint32_t << 16) | rand() as uint32_t;
}

func (converter : &mut ASTConverter) is_html_entity(text : std::string_view, index : uint) : bool {
    if (index + 2 >= text.size()) return false;
    if (text.data()[index] != '&') return false;

    var i = index + 1;
    if (text.data()[i] == '#') {
        i++;
        if (i < text.size() && (text.data()[i] == 'x' || text.data()[i] == 'X')) {
            i++;
            // Hex entity: &#x...;
            var start = i;
            while (i < text.size() && i - start < 8) {
                const c = text.data()[i];
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                    i++;
                } else break;
            }
            return (i > start && i < text.size() && text.data()[i] == ';');
        } else {
            // Decimal entity: &#...;
            var start = i;
            while (i < text.size() && i - start < 8) {
                const c = text.data()[i];
                if (c >= '0' && c <= '9') {
                    i++;
                } else break;
            }
            return (i > start && i < text.size() && text.data()[i] == ';');
        }
    } else {
        // Named entity: &...;
        var start = i;
        while (i < text.size() && i - start < 32) {
            const c = text.data()[i];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                i++;
            } else break;
        }
        return (i > start && i < text.size() && text.data()[i] == ';');
    }
}

func (converter : &mut ASTConverter) escapeHtml(text : std::string_view) {
    var i = 0u;
    var str = &mut converter.str
    while(i < text.size()) {
        const c1 = (text.data()[i] as uint) & 0xFF;
        if (c1 < 0x80) {
            const c = c1 as char;
            switch(c) {
                '&' => {
                    if (converter.is_html_entity(text, i)) {
                        str.append('&');
                    } else {
                        str.append_view("&amp;");
                    }
                }
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

func (converter : &mut ASTConverter) emit_append_js_call(value : *mut Value, len : size_t) {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const support = converter.support

    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location)
    var id = builder.make_identifier(std::string_view("append_js"), support.appendJsFn, false, location)
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base as *mut ChainValue, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)

    var args = call.get_args()
    args.push(value)
    args.push(builder.make_ubigint_value(len, location))

    converter.vec.push(call as *mut ASTNode)
}

func (converter : &mut ASTConverter) emit_append_js_from_str(s : &mut std::string) {
    if (s.empty()) return;
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const size = s.size()
    const value = builder.make_string_value(builder.allocate_view(s.view()), location)
    s.clear()
    converter.emit_append_js_call(value, size)
}

func (converter : &mut ASTConverter) convertChildren(element : *mut HtmlElement) {
    var i : uint = 0;
    var s = element.children.size();
    while(i < s) {
        var nested_child = element.children.get(i)
        converter.convertHtmlChild(nested_child)
        i++;
    }
}

func (converter : &mut ASTConverter) emit_append_html_attributes_spread(element : *mut HtmlElement) {
    const builder = converter.builder;
    const location = intrinsics::get_raw_location();
    
    // Spread all attributes of the call site as HTML attributes
    for(var i : uint = 0; i < element.attributes.size(); i++) {
        const attr = element.attributes.get(i);
        // We skip 'children' as it's handled by TemplateTokenKind.Children
        if(attr.name.equals(view("children"))) continue;
        if(is_mergeable_attr_name(attr.name)) continue;
        
        var s = std::string();
        s.append(' ');
        s.append_view(attr.name);
        if(attr.value != null && attr.value.kind == AttributeValueKind.Text) {
            const val = attr.value as *mut TextAttributeValue;
            s.append_view("=\"");
            s.append_view(strip_js_string_quotes(val.text));
            s.append('\"');
            converter.emit_append_html_from_str(s);
        } else if(attr.value != null && attr.value.kind == AttributeValueKind.Chemical) {
            s.append_view("=\"");
            converter.emit_append_html_from_str(s);
            const val = attr.value as *mut ChemicalAttributeValue;
            converter.put_chemical_value_in(val.value);
            converter.emit_append_html_call(builder.make_string_value(builder.allocate_view(std::string_view("\"")), location), 1)
        } else if(attr.value == null) {
            // boolean attribute
            converter.emit_append_html_from_str(s);
        }
    }
}

func (converter : &mut ASTConverter) convertHtmlComponent(element : *mut HtmlElement) {
    // 0. Flush any pending HTML
    converter.put_chain_in()

    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const signature = element.componentSignature
    
    // 1. Generate the hash based on component name
    const hash = signature.functionNode.getEncodedLocation()
    
    // 2. Generate the if(page.require_component(hash)) block to emit component JS.
    if(signature.mountStrategy != MountStrategy.Universal) {
        var requireCall = converter.make_require_component_call(hash as size_t)
        var ifStmt = builder.make_if_stmt(requireCall as *mut Value, converter.parent, location)
        var body = ifStmt.get_body()
        body.push(converter.make_set_component_hash_call(hash as size_t))
        
        var base = builder.make_identifier(signature.name, signature.functionNode as *mut ASTNode, false, location)
        var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
        var call = builder.make_function_call_node(base as *mut ChainValue, converter.parent, location)
        call.get_args().push(pageId as *mut Value)
        body.push(call as *mut ASTNode)
        
        converter.vec.push(ifStmt as *mut ASTNode)
    }

    // 3. Generate script block
    var s = &mut converter.str
    if(signature.mountStrategy == MountStrategy.Universal && !signature.universalTemplate.empty()) {
        if(signature.jsEmitFunctionNode != null) {
            var reqCall = converter.make_require_component_call(hash as size_t)
            var ifStmt = builder.make_if_stmt(reqCall as *mut Value, converter.parent, location)
            var ifBody = ifStmt.get_body()
            ifBody.push(converter.make_set_component_hash_call(hash as size_t))
            var emitName = signature.jsEmitFunctionNode.getName();
            var emitBase = builder.make_identifier(emitName, signature.jsEmitFunctionNode as *mut ASTNode, false, location)
            var emitPageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
            var emitCall = builder.make_function_call_node(emitBase as *mut ChainValue, converter.parent, location)
            emitCall.get_args().push(emitPageId as *mut Value)
            ifBody.push(emitCall as *mut ASTNode)
            converter.vec.push(ifStmt as *mut ASTNode)
        }
        // HTML-first universal path: emit pre-rendered markup now and queue hydration in page JS.
        var idStr = std::string();
        idStr.append_view("u");
        idStr.append_uinteger(element.loc);

        s.append_view("<div id=\"");
        s.append_view(idStr.view());
        s.append_view("\" data-u-comp=\"");
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *s);
        s.append_view("\">");

        for(var i : uint = 0; i < signature.universalTemplate.size(); i++) {
            const tok = signature.universalTemplate.get(i);
            if(tok.kind == TemplateTokenKind.Text) {
                s.append_view(tok.value);
            } else if(tok.kind == TemplateTokenKind.PropAccess) {
                converter.emit_append_html_from_str(*s);
                converter.resolve_and_put_prop(element, tok.value);
            } else if(tok.kind == TemplateTokenKind.ChemicalValue) {
                converter.emit_append_html_from_str(*s);
                converter.put_chemical_value_in(tok.chemicalValue);
            } else if(tok.kind == TemplateTokenKind.Children) {
                converter.emit_append_html_from_str(*s);
                converter.convertChildren(element);
            } else if(tok.kind == TemplateTokenKind.NestedComponent) {
                // ... (existing placeholder)
                converter.emit_append_html_from_str(*s);
                s.append_view("<!-- Nested component not supported yet in Universal SSR -->");
            } else if(tok.kind == TemplateTokenKind.MergedAttribute) {
                converter.emit_append_html_from_str(*s);
                converter.emit_merged_attribute(element, tok.mergedAttr);
            } else if(tok.kind == TemplateTokenKind.Spread) {
                converter.emit_append_html_from_str(*s);
                converter.emit_append_html_attributes_spread(element);
            }
        }
        s.append_view("</div>");
        converter.emit_append_html_from_str(*s);

        var js = std::string();
        js.append_view("window.$_uq.push(['");
        js.append_view(idStr.view());
        js.append_view("','");
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, js);
        js.append_view("',{");
        const attrs = element.attributes.size();
        for(var i : uint = 0; i < attrs; i++) {
            if(i > 0) js.append_view(",");
            const attr = element.attributes.get(i);
            js.append_view("\"");
            js.append_view(attr.name);
            js.append_view("\":");
            if(attr.value != null && (attr.value.kind == AttributeValueKind.Text || attr.value.kind == AttributeValueKind.Number)) {
                const val = attr.value as *mut TextAttributeValue
                js.append_view(val.text)
            } else {
                js.append_view("true")
            }
        }
        // Append children as a static HTML string prop
        if(!element.children.empty()) {
            if(attrs > 0) js.append_view(",");
            js.append_view("\"children\":\"");
            // Collect static text/element children directly into childHtml
            var childHtml = std::string();
            for(var ci : uint = 0; ci < element.children.size(); ci++) {
                const ch = element.children.get(ci);
                if(ch.kind == HtmlChildKind.Text) {
                    const txt = ch as *mut HtmlText;
                    childHtml.append_view(txt.value);
                } else if(ch.kind == HtmlChildKind.Element) {
                    const el = ch as *mut HtmlElement;
                    childHtml.append('<');
                    childHtml.append_view(el.name);
                    // attributes
                    for(var ai : uint = 0; ai < el.attributes.size(); ai++) {
                        const attr2 = el.attributes.get(ai);
                        childHtml.append(' ');
                        childHtml.append_view(attr2.name);
                        if(attr2.value != null && attr2.value.kind == AttributeValueKind.Text) {
                            const tv = attr2.value as *mut TextAttributeValue;
                            childHtml.append_view("=");
                            childHtml.append_view(tv.text);
                        }
                    }
                    if(el.isSelfClosing) {
                        childHtml.append_view("/>");
                    } else {
                        childHtml.append('>');
                        // TODO: recurse children if needed
                        childHtml.append_view("</");
                        childHtml.append_view(el.name);
                        childHtml.append('>');
                    }
                }
            }
            // Escape childHtml for JS string: replace " -> \" and \ -> \\
            var ci2 : uint = 0;
            while(ci2 < childHtml.size()) {
                const c = childHtml.data()[ci2];
                if(c == '\"') { js.append_view("\\\""); }
                else if(c == '\\') { js.append_view("\\\\"); }
                else { js.append(c); }
                ci2++;
            }
            js.append('\"');
        }
        js.append_view("}]);if(window.$_uf)window.$_uf();");
        converter.emit_append_js_from_str(js);
        return;
    }

    if(signature.mountStrategy == MountStrategy.Universal) {
        if(signature.jsEmitFunctionNode != null) {
            var reqCall = converter.make_require_component_call(hash as size_t)
            var ifStmt = builder.make_if_stmt(reqCall as *mut Value, converter.parent, location)
            var ifBody = ifStmt.get_body()
            ifBody.push(converter.make_set_component_hash_call(hash as size_t))
            var emitName = signature.jsEmitFunctionNode.getName();
            var emitBase = builder.make_identifier(emitName, signature.jsEmitFunctionNode as *mut ASTNode, false, location)
            var emitPageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
            var emitCall = builder.make_function_call_node(emitBase as *mut ChainValue, converter.parent, location)
            emitCall.get_args().push(emitPageId as *mut Value)
            ifBody.push(emitCall as *mut ASTNode)
            converter.vec.push(ifStmt as *mut ASTNode)
        }
        // Universal fallback path: queue mount by component name so definition order does not matter.
        var idStr = std::string();
        idStr.append_view("u");
        idStr.append_uinteger(element.loc);

        // Emit: <div id="..."> 
        s.append_view("<div id=\"");
        s.append_view(idStr.view());
        s.append_view("\" data-u-comp=\"");
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *s);
        s.append_view("\">");
        converter.emit_append_html_from_str(*s);
        
        // Call ComponentFunction(page) to write the component's HTML
        var compBase = builder.make_identifier(signature.name, signature.functionNode as *mut ASTNode, false, location)
        var compPageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
        var compCall = builder.make_function_call_node(compBase as *mut ChainValue, converter.parent, location)
        compCall.get_args().push(compPageId as *mut Value)
        converter.vec.push(compCall as *mut ASTNode)
        
        // Emit: </div>
        converter.str.append_view("</div>");
        converter.emit_append_html_from_str(converter.str);

        var js = std::string();
        js.append_view("window.$_uq.push(['");
        js.append_view(idStr.view());
        js.append_view("','");
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, js);
        js.append_view("',{");
        const attrs = element.attributes.size();
        for(var i : uint = 0; i < attrs; i++) {
            if(i > 0) js.append_view(",");
            const attr = element.attributes.get(i);
            js.append_view("\"");
            js.append_view(attr.name);
            js.append_view("\":");
            if(attr.value != null && (attr.value.kind == AttributeValueKind.Text || attr.value.kind == AttributeValueKind.Number)) {
                const val = attr.value as *mut TextAttributeValue
                js.append_view(val.text)
            } else {
                js.append_view("true")
            }
        }
        // Append children as a static HTML string prop
        if(!element.children.empty()) {
            if(attrs > 0) js.append_view(",");
            js.append_view("\"children\":\"");
            for(var ci : uint = 0; ci < element.children.size(); ci++) {
                const ch = element.children.get(ci);
                if(ch.kind == HtmlChildKind.Text) {
                    const txt = ch as *mut HtmlText;
                    var ci2 : uint = 0;
                    while(ci2 < txt.value.size()) {
                        const c = txt.value.data()[ci2];
                        if(c == '\"') { js.append_view("\\\""); }
                        else if(c == '\\') { js.append_view("\\\\"); }
                        else { js.append(c); }
                        ci2++;
                    }
                } else if(ch.kind == HtmlChildKind.Element) {
                    const el = ch as *mut HtmlElement;
                    js.append('<');
                    js.append_view(el.name);
                    for(var ai : uint = 0; ai < el.attributes.size(); ai++) {
                        const attr2 = el.attributes.get(ai);
                        js.append(' ');
                        js.append_view(attr2.name);
                        if(attr2.value != null && attr2.value.kind == AttributeValueKind.Text) {
                            const tv = attr2.value as *mut TextAttributeValue;
                            js.append_view("=\\\"");
                            // escape attribute value
                            var vi : uint = 2;
                            while(vi < tv.text.size() - 1) {
                                const vc = tv.text.data()[vi];
                                if(vc == '\"') js.append_view("\\\"");
                                else js.append(vc);
                                vi++;
                            }
                            js.append_view("\\\"");
                        }
                    }
                    if(el.isSelfClosing) {
                        js.append_view("/>");
                    } else {
                        js.append('>');
                        js.append_view("</");
                        js.append_view(el.name);
                        js.append('>');
                    }
                }
            }
            js.append('\"');
        }
        js.append_view("}]);if(window.$_uf)window.$_uf();");
        converter.emit_append_js_from_str(js);
        return;
    }

    s.append_view("<script>")
    
    if(signature.mountStrategy == MountStrategy.Preact) {
        // Preact Mount Strategy
        s.append_view("$_pm(document.currentScript, ")
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *s)
        s.append_view(", {")
    } else if(signature.mountStrategy == MountStrategy.React) {
        // React Mount Strategy
        s.append_view("$_rm(document.currentScript, ")
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *s)
        s.append_view(", {")
    } else if(signature.mountStrategy == MountStrategy.Solid) {
        // Solid Mount Strategy
        s.append_view("$_sm(document.currentScript, ")
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *s)
        s.append_view(", {")
    } else if(signature.mountStrategy == MountStrategy.Universal) {
        // Universal Mount Strategy (HTML-first, no framework dependency)
        s.append_view("$_um(document.currentScript, ")
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *s)
        s.append_view(", {")
    } else {
        // Default Mount Strategy
        s.append_view("$_dm(document.currentScript, ")
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *s)
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
            converter.convertChildren(element);

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
