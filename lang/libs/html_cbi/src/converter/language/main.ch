
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

func html_named_entity_code(name : std::string_view) : int {
    if(name.equals(std::string_view("lt"))) { return 60; }
    if(name.equals(std::string_view("gt"))) { return 62; }
    if(name.equals(std::string_view("amp"))) { return 38; }
    if(name.equals(std::string_view("quot"))) { return 34; }
    if(name.equals(std::string_view("apos"))) { return 39; }
    if(name.equals(std::string_view("#39"))) { return 39; }
    if(name.equals(std::string_view("nbsp"))) { return 160; }
    return -1;
}

func html_entity_numeric_code(name : std::string_view) : int {
    if(name.size() < 2 || name.get(0) != '#') { return -1; }
    var hex = false;
    if(name.get(1) == 'x' || name.get(1) == 'X') { hex = true; }
    var start : size_t = 1;
    if(hex) { start = 2; }
    if(start >= name.size()) { return -1; }
    var value : int = 0;
    var i : size_t = start;
    while(i < name.size()) {
        const c = name.get(i);
        var d : int = -1;
        if(c >= '0' && c <= '9') { d = (c as int) - 48; }
        else if(hex && c >= 'a' && c <= 'f') { d = (c as int) - 87; }
        else if(hex && c >= 'A' && c <= 'F') { d = (c as int) - 55; }
        else { return -1; }
        var base : int = 10;
        if(hex) { base = 16; }
        value = value * base + d;
        i++;
    }
    return value;
}

func append_utf8_codepoint_html(str : &mut std::string, cp : int) {
    const c = cp as uint32_t;
    if(c < 0x80) {
        str.append(c as char);
    } else if(c < 0x800) {
        str.append((0xC0 | (c >> 6)) as char);
        str.append((0x80 | (c & 0x3F)) as char);
    } else if(c < 0x10000) {
        str.append((0xE0 | (c >> 12)) as char);
        str.append((0x80 | ((c >> 6) & 0x3F)) as char);
        str.append((0x80 | (c & 0x3F)) as char);
    } else {
        str.append((0xF0 | (c >> 18)) as char);
        str.append((0x80 | ((c >> 12) & 0x3F)) as char);
        str.append((0x80 | ((c >> 6) & 0x3F)) as char);
        str.append((0x80 | (c & 0x3F)) as char);
    }
}

// Decodes HTML character references in #html text/attribute source so that the
// emitted client vnode strings match what the browser renders from the SSR
// markup (e.g. "&lt;" -> "<", "&#8594;" -> "->"). A bare "&" and unknown
// references are preserved. Only known named and numeric forms are replaced.
func decode_html_entities(text : std::string_view) : std::string {
    var out = std::string();
    out.reserve(text.size());
    var i : size_t = 0;
    while(i < text.size()) {
        const ch = text.get(i);
        if(ch != '&') { out.append(ch); i++; continue; }
        var semi : size_t = i + 1;
        var maxi : size_t = i + 12;
        if(maxi > text.size()) { maxi = text.size(); }
        while(semi < maxi && text.get(semi) != ';') { semi++; }
        if(semi >= maxi) { out.append(ch); i++; continue; }
        const name = std::string_view(text.data() + i + 1, semi - i - 1);
        var code = html_named_entity_code(name);
        if(code < 0) { code = html_entity_numeric_code(name); }
        if(code < 0) { out.append(ch); i++; continue; }
        append_utf8_codepoint_html(&mut out, code);
        i = semi + 1;
    }
    return out;
}

// Table-structure elements whose direct children must not be generic wrapper
// elements (a <span> inside <tr>/<table> is foster-parented and corrupts the
// table). Universal components directly inside one of these use a comment
// hydration boundary instead of a <span>.
func is_table_structure_element(name : std::string_view) : bool {
    if(name.equals(std::string_view("table"))) { return true; }
    if(name.equals(std::string_view("thead"))) { return true; }
    if(name.equals(std::string_view("tbody"))) { return true; }
    if(name.equals(std::string_view("tfoot"))) { return true; }
    if(name.equals(std::string_view("tr"))) { return true; }
    if(name.equals(std::string_view("colgroup"))) { return true; }
    return false;
}

func find_attribute(element : *mut HtmlElement, name : std::string_view) : *mut HtmlAttribute {
    for(var i : uint = 0; i < element.attributes.size(); i++) {
        const attr = element.attributes.get(i);
        if(attr.name.equals(&name)) return attr;
    }
    return null;
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
                
                var id = builder.make_identifier(&part, null, false, loc);
                const chain = builder.make_access_chain(&std::span<*mut Value>([ current, id ]), loc)
                current = chain as *mut Value;
                
                if(nextDot == path.size()) break;
                remStart = nextDot + 1;
            }
            converter.put_chemical_value_in(current);
        }
    }
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

func (converter : &mut ASTConverter) escapeHtml(text : std::string_view) {
    html_escape_append(&mut converter.str, text);
}



func (converter : &mut ASTConverter) emit_append_html_call(value : *mut Value, len : size_t) {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const support = converter.support
    
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location)
    var name = if (converter.in_head) std::string_view("append_head") else std::string_view("append_html")
    var fnPtr = if (converter.in_head) support.appendHeadFn else support.appendHtmlFn
    var id = builder.make_identifier(&name, fnPtr, false, location)
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
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

// Recursively collect the static HTML representation of a child node into childHtml.
// Used by emit_universal_queue, which passes top-level component children to the
// client as `window.$__uni_html(...)`. That still transports child markup through
// JS; replacing it with boundary templates is part of the Phase 2 marker work.
func collect_child_html(ch : *mut HtmlChild, childHtml : *mut std::string) {
    if(ch.kind == HtmlChildKind.Text) {
        const txt = ch as *mut HtmlText;
        childHtml.append_view(&txt.value);
    } else if(ch.kind == HtmlChildKind.Element) {
        const el = ch as *mut HtmlElement;
        childHtml.append('<');
        childHtml.append_view(&el.name);
        for(var ai : uint = 0; ai < el.attributes.size(); ai++) {
            const attr2 = el.attributes.get(ai);
            childHtml.append(' ');
            childHtml.append_view(&attr2.name);
            if(attr2.value != null && attr2.value.kind == AttributeValueKind.Text) {
                const tv = attr2.value as *mut TextAttributeValue;
                childHtml.append_view("=");
                childHtml.append_view(&tv.text);
            }
        }
        if(el.isSelfClosing) {
            childHtml.append_view("/>");
        } else {
            childHtml.append('>');
            for(var ci : uint = 0; ci < el.children.size(); ci++) {
                collect_child_html(el.children.get(ci), childHtml);
            }
            childHtml.append_view("</");
            childHtml.append_view(&el.name);
            childHtml.append('>');
        }
    } else if(ch.kind == HtmlChildKind.ChemicalValue) {
        childHtml.append_view("${}");
    } else if(ch.kind == HtmlChildKind.ChemicalNode) {
        childHtml.append_view("${}");
    }
}

// Returns true when every child can be represented as a plain client vnode:
// text, comments, and non-component elements whose attributes are all literal
// (Text/Number) and whose descendants are equally static. Dynamic children --
// Chemical values/statements, @if blocks, or nested universal components -- are
// not representable as static vnodes and take the legacy $__uni_html fallback.
// Counts the top-level DOM nodes a children list produces at SSR, so the client
// `$__uni_html` blob can advance past exactly those nodes during hydration.
// Whitespace-only text between elements is dropped by the HTML parser, so it is
// not counted.
func count_child_nodes(children : &std::vector<*mut HtmlChild>) : ubigint {
    var n : ubigint = 0;
    for(var i : uint = 0; i < children.size(); i++) {
        const ch = children.get(i);
        if(ch.kind == HtmlChildKind.Element) n++;
        else if(ch.kind == HtmlChildKind.Text) {
            const t = ch as *mut HtmlText;
            var has = false;
            for(var j : size_t = 0; j < t.value.size(); j++) {
                const c = t.value.get(j);
                if(c != ' ' && c != '\n' && c != '\t' && c != '\r') { has = true; break; }
            }
            if(has) n++;
        }
    }
    return n;
}

func children_are_static(children : &std::vector<*mut HtmlChild>) : bool {
    for(var i : uint = 0; i < children.size(); i++) {
        const child = children.get(i);
        if(child.kind == HtmlChildKind.Text || child.kind == HtmlChildKind.Comment) continue;
        if(child.kind == HtmlChildKind.Element) {
            const element = child as *mut HtmlElement;
            // Component children stay on the $__uni_html path for now: emitting
            // them as client vnodes requires suppressing each child's dispatch in
            // every path and was not reliably correct yet (see plan).
            if(element.componentSignature != null) return false;
            if(element.name.equals(std::string_view("head"))) return false;
            for(var a : uint = 0; a < element.attributes.size(); a++) {
                const attr = element.attributes.get(a);
                if(attr.value != null && attr.value.kind != AttributeValueKind.Text && attr.value.kind != AttributeValueKind.Number) {
                    return false;
                }
            }
            if(!children_are_static(&element.children)) return false;
            continue;
        }
        return false;
    }
    return true;
}

// Appends a JS string literal for `text` to `out`, escaping quotes, backslashes,
// control characters, and `</` (so the bundle cannot break out of a <script>).
func append_js_string_literal(out : &mut std::string, text : std::string_view) {
    out.append('"');
    var i : size_t = 0;
    while(i < text.size()) {
        const c = text.get(i);
        if(c == '"') out.append_view("\\\"");
        else if(c == '\\') out.append_view("\\\\");
        else if(c == '\n') out.append_view("\\n");
        else if(c == '\r') out.append_view("\\r");
        else if(c == '\t') out.append_view("\\t");
        else if(c == '<' && i + 1 < text.size() && text.get(i + 1) == '/') out.append_view("\\u003C");
        else out.append(c);
        i++;
    }
    out.append('"');
}

// Appends client vnodes for static children (see children_are_static) to `out`.
// Must only be called when children_are_static returned true.
func append_static_child_vnodes(out : &mut std::string, children : &std::vector<*mut HtmlChild>) {
    var first = true;
    for(var i : uint = 0; i < children.size(); i++) {
        const child = children.get(i);
        if(child.kind == HtmlChildKind.Comment) continue;
        if(child.kind == HtmlChildKind.Text) {
            if(!first) out.append(',');
            first = false;
            const text = child as *mut HtmlText;
            const decoded = decode_html_entities(text.value);
            append_js_string_literal(out, decoded.to_view());
        } else if(child.kind == HtmlChildKind.Element) {
            if(!first) out.append(',');
            first = false;
            const element = child as *mut HtmlElement;
            const isComponent = element.componentSignature != null;
            if(isComponent) {
                // Nested component child: emit a component vnode the parent mounts,
                // so hydration/context work like a normal parent/child tree.
                out.append_view("$_uc_c(");
                get_module_scoped_name(element.componentSignature.functionNode, element.componentSignature.name, out);
                out.append_view(", {");
            } else {
                out.append_view("$_ur.createElement(\"");
                out.append_view(&element.name);
                out.append_view("\", {");
            }
            var afirst = true;
            for(var a : uint = 0; a < element.attributes.size(); a++) {
                const attr = element.attributes.get(a);
                if(!afirst) out.append(',');
                afirst = false;
                out.append('"');
                out.append_view(&attr.name);
                out.append_view("\":");
                if(attr.value == null) {
                    out.append_view("true");
                } else {
                    const tv = attr.value as *mut TextAttributeValue;
                    var rawText = strip_js_string_quotes(tv.text);
                    if(attr.value.kind == AttributeValueKind.Number) {
                        out.append_view(&rawText);
                    } else {
                        const decodedAttr = decode_html_entities(rawText);
                        append_js_string_literal(out, decodedAttr.to_view());
                    }
                }
            }
            if(isComponent) {
                if(!element.children.empty()) {
                    if(!afirst) out.append(',');
                    out.append_view("children:[");
                    append_static_child_vnodes(out, &element.children);
                    out.append(']');
                }
                out.append_view("})");
            } else {
                out.append('}');
                if(!element.isSelfClosing && !element.children.empty()) {
                    out.append(',');
                    append_static_child_vnodes(out, &element.children);
                }
                out.append(')');
            }
        }
    }
}

func (converter : &mut ASTConverter) emit_universal_queue(element : *mut HtmlElement, signature : *mut ComponentSignature, idStr : &std::string, markerBoundary : bool = false) {
    var js = std::string();
    js.append_view("window.$__uni_dispatch('");
    // For a styled wrap over a universal component, hydration must target the
    // inner universal component (which owns the client JS); the styled
    // component's own `functionNode` is only used for SSR (to emit its CSS).
    var hydrateNode : *mut ASTNode;
    var hydrateName : std::string_view;
    if(signature.hydrateFunctionNode != null) {
        hydrateNode = signature.hydrateFunctionNode as *mut ASTNode;
        hydrateName = signature.hydrateName;
    } else {
        hydrateNode = signature.functionNode as *mut ASTNode;
        hydrateName = signature.name;
    }
    get_module_scoped_name(hydrateNode, hydrateName, &mut js);
    if(markerBoundary) {
        // Table-context components have no wrapper element; the id names a
        // comment boundary resolved at hydration time.
        js.append_view("', window.$__uni_boundary('");
    } else {
        js.append_view("', document.getElementById('");
    }
    js.append_view(idStr.view());
    js.append_view("'), {");
    converter.emit_append_js_from_str(&mut js);

    const attrs = element.attributes.size();
    var emittedCount : uint = 0;
    for(var i : uint = 0; i < attrs; i++) {
        const attr = element.attributes.get(i);
        // Skip event attributes that are not function values.
        // has_non_ssr_attr_value returns true only for ValueKind.LambdaFunc
        // (the only valid event handler type in hydration JS).
        if(is_event_attribute_name(attr.name) && !has_non_ssr_attr_value(attr.value)) {
            continue;
        }
        var s = &mut js;
        if(emittedCount > 0) s.append_view(",");
        emittedCount++;
        s.append_view("\"");
        s.append_view(&attr.name);
        s.append_view("\":");
        if(attr.value != null && (attr.value.kind == AttributeValueKind.Text || attr.value.kind == AttributeValueKind.Number)) {
            const val = attr.value as *mut TextAttributeValue
            s.append_view(&val.text)
        } else if(attr.value != null && attr.value.kind == AttributeValueKind.Chemical) {
            const val = attr.value as *mut ChemicalAttributeValue
            const type = val.value.getType()
            const is_str = converter.is_string_type(type)
            if(is_str) {
                s.append('"')
                converter.emit_append_js_from_str(&mut *s)
                converter.put_js_value_in(val.value)
                s.append('"')
            } else {
                converter.emit_append_js_from_str(&mut *s)
                converter.put_js_value_in(val.value)
            }
        } else if(attr.value != null && attr.value.kind == AttributeValueKind.ChemicalValues) {
            converter.emit_append_js_from_str(&mut *s)
            const valuesNode = attr.value as *mut ChemicalAttributeValues
            var quote = std::string("\"");
            converter.emit_append_js_from_str(&mut quote)
            for(var j : uint = 0; j < valuesNode.values.size(); j++) {
                if(j > 0) {
                    var space = std::string(" ");
                    converter.emit_append_js_from_str(&mut space)
                }
                converter.put_js_value_in(valuesNode.values.get(j))
            }
            var quote2 = std::string("\"");
            converter.emit_append_js_from_str(&mut quote2)
            continue;
        } else {
            s.append_view("true")
        }
        converter.emit_append_js_from_str(&mut *s)
    }

    var tail = std::string();
    if(!element.children.empty()) {
        if(emittedCount > 0) tail.append_view(",");
        if(children_are_static(&element.children)) {
            // Phase 2: emit real client vnodes for static children instead of
            // transporting their SSR HTML through the JS bundle. Dynamic children
            // (Chemical values/statements, @if blocks, nested universal
            // components) still fall back to the legacy $__uni_html blob.
            tail.append_view("\"children\":[");
            append_static_child_vnodes(&mut tail, &element.children);
            tail.append_view("]");
        } else {
            tail.append_view("\"children\":window.$__uni_html(\"");
            // Collect static text/element children directly into childHtml
            var childHtml = std::string();
            for(var ci : uint = 0; ci < element.children.size(); ci++) {
                collect_child_html(element.children.get(ci), &raw mut childHtml);
            }
            // Escape childHtml for JS string: replace " -> \" and \ -> \\
            var ci2 : uint = 0;
            while(ci2 < childHtml.size()) {
                const c = childHtml.data()[ci2];
                if(c == '\"') { tail.append_view("\\\""); }
                else if(c == '\\') { tail.append_view("\\\\"); }
                else if(c == '\n') { tail.append_view("\\n"); }
                else if(c == '\r') { tail.append_view("\\r"); }
                else if(c == '\t') { tail.append_view("\\t"); }
                else { tail.append(c); }
                ci2++;
            }
            tail.append_view("\", ");
            tail.append_uinteger(count_child_nodes(&element.children));
            tail.append_view(")");
        }
    }
    if(markerBoundary) {
        // Root-mode hydration: the comment resolves to the component's own root
        // element, which is hydrated in place (no wrapper container).
        tail.append_view("}, \"root\");\n");
    } else {
        tail.append_view("});\n");
    }
    converter.emit_append_js_from_str(&mut tail);
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
            str.append_view(&element.name)

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
            const prevInTable = converter.in_table_context;
            if(is_table_structure_element(element.name)) {
                converter.in_table_context = true;
            }
            converter.convertChildren(element);
            converter.in_table_context = prevInTable;

            if(!element.isSelfClosing) {
                str.append('<')
                str.append('/')
                str.append_view(&element.name)
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
