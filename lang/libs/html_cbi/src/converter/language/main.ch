
func (str : &std::string) view() : std::string_view {
    return std::string_view(str.data(), str.size());
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
                const chain = builder.make_access_chain(std::span<*mut Value>([ current as *mut Value, id as *mut Value ]), loc)
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



func (converter : &mut ASTConverter) emit_append_html_call(value : *mut Value, len : size_t) {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const support = converter.support
    
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location)
    var name = if (converter.in_head) std::string_view("append_head") else std::string_view("append_html")
    var fnPtr = if (converter.in_head) support.appendHeadFn else support.appendHtmlFn
    var id = builder.make_identifier(name, fnPtr, false, location)
    const chain = builder.make_access_chain(std::span<*mut Value>([ base as *mut Value, id ]), location)
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

func (converter : &mut ASTConverter) emit_universal_queue(element : *mut HtmlElement, signature : *mut ComponentSignature, idStr : &std::string) {
    var js = std::string();
    js.append_view("window.$_uq.push(['");
    js.append_view(idStr.view());
    js.append_view("','");
    get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, js);
    js.append_view("',{");
    converter.emit_append_js_from_str(js);

    const attrs = element.attributes.size();
    for(var i : uint = 0; i < attrs; i++) {
        var s = &mut js;
        if(i > 0) s.append_view(",");
        const attr = element.attributes.get(i);
        s.append_view("\"");
        s.append_view(attr.name);
        s.append_view("\":");
        if(attr.value != null && (attr.value.kind == AttributeValueKind.Text || attr.value.kind == AttributeValueKind.Number)) {
            const val = attr.value as *mut TextAttributeValue
            s.append_view(val.text)
        } else if(attr.value != null && attr.value.kind == AttributeValueKind.Chemical) {
            const val = attr.value as *mut ChemicalAttributeValue
            const type = val.value.getType()
            const is_str = converter.is_string_type(type)
            if(is_str) {
                s.append('"')
                converter.emit_append_js_from_str(*s)
                converter.put_js_value_in(val.value)
                s.append('"')
            } else {
                converter.emit_append_js_from_str(*s)
                converter.put_js_value_in(val.value)
            }
        } else if(attr.value != null && attr.value.kind == AttributeValueKind.ChemicalValues) {
            converter.emit_append_js_from_str(*s)
            const valuesNode = attr.value as *mut ChemicalAttributeValues
            var quote = std::string("\"");
            converter.emit_append_js_from_str(quote)
            for(var j : uint = 0; j < valuesNode.values.size(); j++) {
                if(j > 0) {
                    var space = std::string(" ");
                    converter.emit_append_js_from_str(space)
                }
                converter.put_js_value_in(valuesNode.values.get(j))
            }
            var quote2 = std::string("\"");
            converter.emit_append_js_from_str(quote2)
            continue;
        } else {
            s.append_view("true")
        }
        converter.emit_append_js_from_str(*s)
    }

    var tail = std::string();
    if(!element.children.empty()) {
        if(attrs > 0) tail.append_view(",");
        tail.append_view("\"children\":\"");
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
            if(c == '\"') { tail.append_view("\\\""); }
            else if(c == '\\') { tail.append_view("\\\\"); }
            else { tail.append(c); }
            ci2++;
        }
        tail.append('\"');
    }
    tail.append_view("}]);");
    converter.emit_append_js_from_str(tail);
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
