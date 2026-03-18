func append_style_js_node_text(node : *mut JsNode, out : &mut std::string) : bool {
    if(node == null) return false;
    switch(node.kind) {
        JsNodeKind.Literal => {
            out.append_view(strip_js_string_quotes((node as *mut JsLiteral).value));
            return true;
        }
        JsNodeKind.Identifier => {
            out.append_view((node as *mut JsIdentifier).value);
            return true;
        }
        JsNodeKind.MemberAccess => {
            const mem = node as *mut JsMemberAccess;
            if(!append_js_node_text(mem.object, out)) return false;
            out.append('.');
            out.append_view(mem.property);
            return true;
        }
        JsNodeKind.IndexAccess => {
            const idx = node as *mut JsIndexAccess;
            if(!append_js_node_text(idx.object, out)) return false;
            out.append('[');
            if(!append_js_node_text(idx.index, out)) return false;
            out.append(']');
            return true;
        }
        JsNodeKind.UnaryOp => {
            const unary = node as *mut JsUnaryOp;
            if(unary.prefix) {
                out.append_view(unary.operator);
                return append_js_node_text(unary.operand, out);
            }
            if(!append_js_node_text(unary.operand, out)) return false;
            out.append_view(unary.operator);
            return true;
        }
        JsNodeKind.BinaryOp => {
            const bin = node as *mut JsBinaryOp;
            if(!append_js_node_text(bin.left, out)) return false;
            out.append(' ');
            out.append_view(bin.op);
            out.append(' ');
            return append_js_node_text(bin.right, out);
        }
        JsNodeKind.Ternary => {
            const tern = node as *mut JsTernary;
            if(!append_js_node_text(tern.condition, out)) return false;
            out.append_view(" ? ");
            if(!append_js_node_text(tern.consequent, out)) return false;
            out.append_view(" : ");
            return append_js_node_text(tern.alternate, out);
        }
        JsNodeKind.FunctionCall => {
            const call = node as *mut JsFunctionCall;
            if(!append_js_node_text(call.callee, out)) return false;
            out.append('(');
            for(var i : uint = 0; i < call.args.size(); i++) {
                if(i > 0) out.append_view(", ");
                if(!append_js_node_text(call.args.get(i), out)) return false;
            }
            out.append(')');
            return true;
        }
        JsNodeKind.ArrayLiteral, JsNodeKind.ArrayDestructuring => {
            const arr = node as *mut JsArrayLiteral;
            out.append('[');
            for(var i : uint = 0; i < arr.elements.size(); i++) {
                if(i > 0) out.append_view(", ");
                const elem = arr.elements.get(i);
                if(elem != null && !append_js_node_text(elem, out)) return false;
            }
            out.append(']');
            return true;
        }
        JsNodeKind.ObjectLiteral => {
            const obj = node as *mut JsObjectLiteral;
            out.append('{');
            for(var i : uint = 0; i < obj.properties.size(); i++) {
                if(i > 0) out.append_view(", ");
                const prop = obj.properties.get(i);
                out.append_view(prop.key);
                out.append_view(": ");
                if(!append_js_node_text(prop.value, out)) return false;
            }
            out.append('}');
            return true;
        }
        default => return false
    }
}

func append_kebab_case(input: std::string_view, out: &mut std::string) {
    for (var i: uint = 0; i < input.size(); i++) {
        const c = input.get(i);
        if (c >= 'A' && c <= 'Z') {
            // Add a hyphen if it's not the very first character
            if (i > 0) {
                out.append('-');
            }
            // Convert to lowercase (ASCII offset)
            out.append((c + 32) as char);
        } else {
            out.append(c);
        }
    }
}

func build_js_node_text_view_style_attr(builder : *mut ASTBuilder, obj : *mut JsObjectLiteral) : std::string_view {
    var out = std::string();
    for(var i : uint = 0; i < obj.properties.size(); i++) {
        if(i > 0) out.append(';');
        const prop = obj.properties.get(i);
        const stripped = strip_js_string_quotes(prop.key);
        if(stripped.size() < prop.key.size()) {
            // quotes were stripped from property key
            out.append_view(stripped);
        } else {
            // quotes weren't stripped from property key
            // must make sure borderRadius translates to border-radius
            append_kebab_case(prop.key, out);
        }
        out.append(':');
        if(!append_style_js_node_text(prop.value, out)) break;
    }
    return builder.allocate_view(out.to_view());
}