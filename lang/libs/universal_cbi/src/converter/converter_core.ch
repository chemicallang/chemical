func (converter : &mut JsConverter) convertJsNode(node : *mut JsNode) {
    if(node == null) return;
    switch(node.kind) {
        JsNodeKind.Literal => {
            var lit = node as *mut JsLiteral
            const val = lit.value
            if (val.size() >= 2 && (val.get(0) == '\'' || val.get(0) == '\"' || val.get(0) == '`')) {
                converter.str.append(val.get(0));
                converter.escapeJs(val.subview(1, val.size() - 1));
                converter.str.append(val.get(0));
            } else {
                converter.str.append_view(lit.value);
            }
        }
        JsNodeKind.FunctionDecl => {
             var func = node as *mut JsFunctionDecl
             if(func.is_async) converter.str.append_view("async ");
             converter.str.append_view("function ");
             if(func.is_generator) converter.str.append('*');
             if(!func.name.empty()) {
                 converter.str.append_view(func.name);
             }
             converter.str.append_view("(");
             for(var i : uint = 0; i < func.params.size(); i++) {
                 if(i > 0) converter.str.append_view(", ");
                 converter.str.append_view(func.params.get(i));
             }
             converter.str.append_view(") ");
             converter.convertJsNode(func.body);
        }
        JsNodeKind.Identifier => {
            var id = node as *mut JsIdentifier
            if(converter.is_state_var(id.value)) {
                converter.str.append_view(id.value);
                converter.str.append_view(".value");
            } else {
                converter.str.append_view(id.value);
            }
        }
        JsNodeKind.ChemicalValue => {
            converter.convertChemicalValue(node as *mut JsChemicalValue);
        }
        JsNodeKind.UnaryOp => {
            var unary = node as *mut JsUnaryOp
            if(unary.operand != null && unary.operand.kind == JsNodeKind.Identifier) {
                const id = unary.operand as *mut JsIdentifier
                const is_update = unary.operator.equals(view("++")) || unary.operator.equals(view("--"))
                if(is_update && converter.is_state_var(id.value)) {
                    if(unary.prefix) {
                        converter.str.append_view(unary.operator);
                        converter.str.append_view(id.value);
                        converter.str.append_view(".value");
                    } else {
                        converter.str.append_view(id.value);
                        converter.str.append_view(".value");
                        converter.str.append_view(unary.operator);
                    }
                    return;
                }
            }
            if(unary.prefix) {
                converter.str.append_view(unary.operator);
                converter.convertJsNode(unary.operand);
            } else {
                converter.convertJsNode(unary.operand);
                converter.str.append_view(unary.operator);
            }
        }
        JsNodeKind.BinaryOp => {
            var bin = node as *mut JsBinaryOp
            if(bin.left != null && bin.left.kind == JsNodeKind.Identifier) {
                const id = bin.left as *mut JsIdentifier
                if(converter.is_state_var(id.value) &&
                    (bin.op.equals(view("=")) || bin.op.equals(view("+=")) || bin.op.equals(view("-=")) || bin.op.equals(view("*=")) || bin.op.equals(view("/=")))) {
                    converter.str.append_view(id.value);
                    converter.str.append_view(".value");
                    converter.str.append_view(" ");
                    converter.str.append_view(bin.op);
                    converter.str.append_view(" ");
                    converter.convertJsNode(bin.right);
                } else {
                    converter.convertJsNode(bin.left);
                    converter.str.append_view(" ");
                    converter.str.append_view(bin.op);
                    converter.str.append_view(" ");
                    converter.convertJsNode(bin.right);
                }
            } else {
                converter.convertJsNode(bin.left);
                converter.str.append_view(" ");
                converter.str.append_view(bin.op);
                converter.str.append_view(" ");
                converter.convertJsNode(bin.right);
            }
        }
        JsNodeKind.Ternary => {
            var tern = node as *mut JsTernary
            converter.convertJsNode(tern.condition);
            converter.str.append_view(" ? ");
            converter.convertJsNode(tern.consequent);
            converter.str.append_view(" : ");
            converter.convertJsNode(tern.alternate);
        }
        JsNodeKind.FunctionCall => {
            var call = node as *mut JsFunctionCall
            if(call.callee.kind == JsNodeKind.Identifier) {
                var id = call.callee as *mut JsIdentifier
                var name = id.value
                switch(fnv1_hash_view(name)) {
                    comptime_fnv1_hash("useState"),
                    comptime_fnv1_hash("useEffect"),
                    comptime_fnv1_hash("useMemo"),
                    comptime_fnv1_hash("useCallback"),
                    comptime_fnv1_hash("useRef"),
                    comptime_fnv1_hash("useContext"),
                    comptime_fnv1_hash("useReducer"),
                    comptime_fnv1_hash("useLayoutEffect"),
                    comptime_fnv1_hash("useErrorBoundary") => {
                         converter.str.append_view("$_r.")
                    }
                    default => {}
                }
            }
            converter.convertJsNode(call.callee);
            converter.str.append_view("(");
            for(var i : uint = 0; i < call.args.size(); i++) {
                if(i > 0) converter.str.append_view(", ");
                converter.convertJsNode(call.args.get(i));
            }
            converter.str.append_view(")");
        }
        JsNodeKind.MemberAccess => {
            var mem = node as *mut JsMemberAccess
            if(mem.object != null && mem.object.kind == JsNodeKind.Identifier) {
                const id = mem.object as *mut JsIdentifier
                if(converter.is_state_var(id.value) && mem.property.equals(view("value"))) {
                    converter.str.append_view(id.value);
                    converter.str.append_view(".value");
                } else {
                    converter.convertJsNode(mem.object);
                    converter.str.append_view(".");
                    converter.str.append_view(mem.property);
                }
            } else {
                converter.convertJsNode(mem.object);
                converter.str.append_view(".");
                converter.str.append_view(mem.property);
            }
        }
        JsNodeKind.IndexAccess => {
            var idx = node as *mut JsIndexAccess
            converter.convertJsNode(idx.object);
            converter.str.append_view("[");
            converter.convertJsNode(idx.index);
            converter.str.append_view("]");
        }
        JsNodeKind.ArrayLiteral, JsNodeKind.ArrayDestructuring => {
            var arr = node as *mut JsArrayLiteral
            converter.str.append_view("[");
            for(var i : uint = 0; i < arr.elements.size(); i++) {
                if(i > 0) converter.str.append_view(", ");
                const el = arr.elements.get(i);
                if(el != null) converter.convertJsNode(el);
            }
            converter.str.append_view("]");
        }
        JsNodeKind.ObjectLiteral => {
            var obj = node as *mut JsObjectLiteral
            converter.str.append_view("{");
            for(var i : uint = 0; i < obj.properties.size(); i++) {
                if(i > 0) converter.str.append_view(", ");
                converter.str.append_view(obj.properties.get(i).key);
                converter.str.append_view(": ");
                converter.convertJsNode(obj.properties.get(i).value);
            }
            converter.str.append_view("}");
        }
        JsNodeKind.ArrowFunction => {
             var arrow = node as *mut JsArrowFunction
             if(arrow.is_async) converter.str.append_view("async ");
             converter.str.append_view("(");
             for(var i : uint = 0; i < arrow.params.size(); i++) {
                 if(i > 0) converter.str.append_view(", ");
                 converter.str.append_view(arrow.params.get(i));
             }
             converter.str.append_view(") => ");
             converter.convertJsNode(arrow.body);
        }
        JsNodeKind.Block => {
             var block = node as *mut JsBlock
             converter.str.append_view("{ ");
             for(var i : uint = 0; i < block.statements.size(); i++) {
                 converter.convertJsNode(block.statements.get(i));
                 converter.str.append_view(" ");
             }
             converter.str.append_view("}");
        }
        JsNodeKind.ExpressionStatement => {
             var expr = node as *mut JsExpressionStatement
             converter.convertJsNode(expr.expression);
             converter.str.append_view(";");
        }
        JsNodeKind.VarDecl => {
             var decl = node as *mut JsVarDecl
             if(decl.keyword.equals(view("state")) && decl.pattern == null && !decl.name.empty()) {
                 converter.state_vars.push(decl.name);
                 var initText = view("undefined");
                 if(decl.value != null) {
                     initText = build_js_node_text_view(converter.builder, decl.value);
                 }
                 converter.state_inits.push(JsStateInit {
                     name : decl.name,
                     init : initText
                 });
                 converter.str.append_view("const ");
                 converter.str.append_view(decl.name);
                 converter.str.append_view(" = $_us(");
                 if(decl.value != null) {
                     converter.convertJsNode(decl.value);
                 } else {
                     converter.str.append_view("undefined");
                 }
                 converter.str.append_view(");");
             } else {
                 converter.str.append_view(decl.keyword);
                 converter.str.append_view(" ");
                 if(decl.pattern != null) {
                     converter.convertJsNode(decl.pattern);
                 } else {
                     converter.str.append_view(decl.name);
                 }
                 if(decl.value != null) {
                     converter.str.append_view(" = ");
                     converter.convertJsNode(decl.value);
                 }
                 converter.str.append_view(";");
             }
        }
        JsNodeKind.If => {
             var ifStmt = node as *mut JsIf
             converter.str.append_view("if(");
             converter.convertJsNode(ifStmt.condition);
             converter.str.append_view(") ");
             converter.convertJsNode(ifStmt.thenBlock);
             if(ifStmt.elseBlock != null) {
                 converter.str.append_view(" else ");
                 converter.convertJsNode(ifStmt.elseBlock);
             }
        }
        JsNodeKind.Return => {
             var ret = node as *mut JsReturn
             converter.str.append_view("return ");
             if(ret.value != null) converter.convertJsNode(ret.value);
             converter.str.append_view(";");
        }
        JsNodeKind.For => {
             var f = node as *mut JsFor
             converter.str.append_view("for(");
             if(f.init != null) converter.convertJsNode(f.init); else converter.str.append_view(";");
             if(f.condition != null) converter.convertJsNode(f.condition);
             converter.str.append_view(";");
             if(f.update != null) converter.convertJsNode(f.update);
             converter.str.append_view(") ");
             converter.convertJsNode(f.body);
        }
        JsNodeKind.ForIn => {
             var f = node as *mut JsForIn
             converter.str.append_view("for(");
             converter.convertJsNode(f.left);
             converter.str.append_view(" in ");
             converter.convertJsNode(f.right);
             converter.str.append_view(") ");
             converter.convertJsNode(f.body);
        }
        JsNodeKind.JSXElement => {
             converter.convertJSXElement(node as *mut JsJSXElement);
        }
        JsNodeKind.JSXFragment => {
             converter.convertJSXFragment(node as *mut JsJSXFragment);
        }
        JsNodeKind.JSXExpressionContainer => {
             var container = node as *mut JsJSXExpressionContainer
             if(converter.target == BufferType.HTML) {
                 if(container.expression != null) {
                     if(container.expression.kind == JsNodeKind.ChemicalValue) {
                         converter.convertChemicalValue(container.expression as *mut JsChemicalValue);
                     } else if(converter.is_props_children(container.expression)) {
                         const builder = converter.builder;
                         const location = intrinsics::get_raw_location();
                          var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
                          var childrenId = builder.make_identifier(std::string_view("children"), converter.support.childrenParamNode, false, location);
                          
                          var appendCall = builder.make_function_call_node(
                              builder.make_access_chain(std::span<*mut Value>([ pageId, builder.make_identifier(std::string_view("append_html"), converter.support.appendHtmlFn, false, location) ]), location),
                              converter.parent,
                              location
                          );
                          const dataIdNode = converter.support.childrenParamNode.child("data");
                          const sizeIdNode = converter.support.childrenParamNode.child("size");
                          const childrenDataAccess = builder.make_access_chain(std::span<*mut Value>([ childrenId, builder.make_identifier(view("data"), dataIdNode, false, location) ]), location);
                          const childrenSizeAccess = builder.make_access_chain(std::span<*mut Value>([ childrenId, builder.make_identifier(view("size"), sizeIdNode, false, location) ]), location);
                          const appendCallParams = appendCall.get_args();
                          appendCallParams.push(childrenDataAccess);
                          appendCallParams.push(childrenSizeAccess);
                          
                          converter.vec.push(appendCall);
                     } else if(container.expression.kind == JsNodeKind.MemberAccess) {
                         const mem = container.expression as *mut JsMemberAccess;
                         if(mem.object != null && mem.object.kind == JsNodeKind.Identifier && (mem.object as *mut JsIdentifier).value.equals("props")) {
                             const v = converter.make_ssr_prop_v_call(mem.property);
                             const call = converter.render_ssr_value_call(v);
                             converter.vec.push(call);
                         }
                     }
                 }
                 return;
             }
             if(container.expression != null) {
                 if(container.expression.kind == JsNodeKind.Identifier) {
                     const id = container.expression as *mut JsIdentifier
                     if(converter.is_state_var(id.value)) {
                         converter.str.append_view(id.value);
                         return;
                     }
                 } else if(container.expression.kind == JsNodeKind.MemberAccess) {
                     const mem = container.expression as *mut JsMemberAccess
                     if(mem.object != null && mem.object.kind == JsNodeKind.Identifier && mem.property.equals(view("value"))) {
                         const id = mem.object as *mut JsIdentifier
                         if(converter.is_state_var(id.value)) {
                             converter.str.append_view(id.value);
                             return;
                         }
                     }
                 }
                 converter.convert_jsx_runtime_expr(container.expression);
                 return;
             }
        }
        JsNodeKind.JSXText => {
             var text = node as *mut JsJSXText
             if(converter.target == BufferType.HTML) {
                 converter.escapeHtml(text.value);
                 converter.put_chain_in();
             } else {
                 converter.str.append_view("` "); // Use backticks to support multi-line strings
                 converter.escapeJs(text.value);
                 converter.str.append_view(" `");
             }
        }
        default => {
        }
    }
}

func (converter : &mut JsConverter) is_props_children(node : *mut JsNode) : bool {
    if(node == null || node.kind != JsNodeKind.MemberAccess) return false;
    const mem = node as *mut JsMemberAccess;
    if(mem.object == null || mem.object.kind != JsNodeKind.Identifier) return false;
    const id = mem.object as *mut JsIdentifier;
    return id.value.equals(view("props")) && mem.property.equals(view("children"));
}
