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
             const is_anon = func.name.empty()
             if(is_anon) converter.str.append('(');
             if(func.is_async) converter.str.append_view("async ");
             converter.str.append_view("function");
             if(func.is_generator) converter.str.append('*');
             if(!is_anon) {
                 converter.str.append(' ');
                 converter.str.append_view(func.name);
             }
             converter.str.append_view("(");
             for(var i : uint = 0; i < func.params.size(); i++) {
                 if(i > 0) converter.str.append_view(", ");
                 var param = func.params.get_ptr(i);
                 converter.str.append_view(param.name);
                 if(param.default_value != null) {
                     converter.str.append_view(" = ");
                     converter.convertJsNode(param.default_value);
                 }
             }
             converter.str.append_view(") ");
             converter.convertJsNode(func.body);
             if(is_anon) converter.str.append(')');
        }
        JsNodeKind.Identifier => {
            var id = node as *mut JsIdentifier
            if(converter.is_reactive_var(id.value) && !converter.skip_reactive_deref) {
                converter.str.append_view(id.value);
                converter.str.append_view(".value");
            } else if(converter.is_component_props_name(id.value)) {
                converter.append_component_prop_value(node);
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
                if(is_update && converter.is_reactive_var(id.value)) {
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
                if(unary.operator.size() > 2 && isalpha(unary.operator.get(0) as int)) {
                    converter.str.append_view(" ")
                }
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
                if(converter.is_reactive_var(id.value) &&
                    (bin.op.equals(view("=")) || bin.op.equals(view("+=")) || bin.op.equals(view("-=")) || bin.op.equals(view("*=")) || bin.op.equals(view("/=")))) {
                    converter.str.append_view(id.value);
                    converter.str.append_view(".value");
                    converter.str.append_view(" ");
                    converter.str.append_view(bin.op);
                    converter.str.append_view(" ");
                    if(bin.right != null && bin.right.kind == JsNodeKind.Ternary) {
                        converter.str.append_view("(");
                        converter.convertJsNode(bin.right);
                        converter.str.append_view(")");
                    } else {
                        converter.convertJsNode(bin.right);
                    }
                } else {
                    if(bin.left != null && bin.left.kind == JsNodeKind.Ternary) {
                        converter.str.append_view("(");
                        converter.convertJsNode(bin.left);
                        converter.str.append_view(")");
                    } else {
                        converter.convertJsNode(bin.left);
                    }
                    converter.str.append_view(" ");
                    converter.str.append_view(bin.op);
                    converter.str.append_view(" ");
                    if(bin.right != null && bin.right.kind == JsNodeKind.Ternary) {
                        converter.str.append_view("(");
                        converter.convertJsNode(bin.right);
                        converter.str.append_view(")");
                    } else {
                        converter.convertJsNode(bin.right);
                    }
                }
            } else {
                if(bin.left != null && bin.left.kind == JsNodeKind.Ternary) {
                    converter.str.append_view("(");
                    converter.convertJsNode(bin.left);
                    converter.str.append_view(")");
                } else {
                    converter.convertJsNode(bin.left);
                }
                converter.str.append_view(" ");
                converter.str.append_view(bin.op);
                converter.str.append_view(" ");
                if(bin.right != null && bin.right.kind == JsNodeKind.Ternary) {
                    converter.str.append_view("(");
                    converter.convertJsNode(bin.right);
                    converter.str.append_view(")");
                } else {
                    converter.convertJsNode(bin.right);
                }
            }
        }
        JsNodeKind.Ternary => {
            var tern = node as *mut JsTernary
            converter.str.append_view("(");
            converter.convertJsNode(tern.condition);
            converter.str.append_view(" ? ");
            converter.convertJsNode(tern.consequent);
            converter.str.append_view(" : ");
            converter.convertJsNode(tern.alternate);
            converter.str.append_view(")");
        }
        JsNodeKind.FunctionCall => {
            var call = node as *mut JsFunctionCall
            var is_hook = false
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
                         is_hook = true
                    }
                    default => {}
                }
            }
            converter.convertJsNode(call.callee);
            converter.str.append_view("(");
            for(var i : uint = 0; i < call.args.size(); i++) {
                if(i > 0) converter.str.append_view(", ");
                if(is_hook && i == call.args.size() - 1 && call.args.size() >= 2) {
                    converter.skip_reactive_deref = true;
                }
                converter.convertJsNode(call.args.get(i));
                converter.skip_reactive_deref = false;
            }
            converter.str.append_view(")");
        }
        JsNodeKind.MemberAccess => {
            var mem = node as *mut JsMemberAccess
            if(mem.object != null && mem.object.kind == JsNodeKind.Identifier) {
                const id = mem.object as *mut JsIdentifier
                if(converter.is_reactive_var(id.value) && mem.property.equals(view("value"))) {
                    converter.str.append_view(id.value);
                    converter.str.append_view(".value");
                } else if(converter.is_component_props_name(id.value)) {
                    converter.append_component_prop_value(node);
                } else {
                    converter.convertJsNode(mem.object);
                    converter.str.append_view(".");
                    converter.str.append_view(mem.property);
                }
            } else if(mem.property.equals(view("value")) && converter.is_component_props_read(mem.object)) {
                converter.convertJsNode(mem.object);
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
                const prop = obj.properties.get(i);
                if(prop.value != null && prop.value.kind == JsNodeKind.Spread) {
                    converter.convertJsNode(prop.value);
                } else {
                    converter.str.append_view(prop.key);
                    converter.str.append_view(": ");
                    converter.convertJsNode(prop.value);
                }
            }
            converter.str.append_view("}");
        }
        JsNodeKind.Spread => {
            const spread = node as *mut JsSpread
            converter.str.append_view("...");
            converter.convertJsNode(spread.argument);
        }
        JsNodeKind.ArrowFunction => {
             var arrow = node as *mut JsArrowFunction
             if(arrow.is_async) converter.str.append_view("async ");
             converter.str.append_view("(");
             for(var i : uint = 0; i < arrow.params.size(); i++) {
                 if(i > 0) converter.str.append_view(", ");
                 var param = arrow.params.get_ptr(i);
                converter.str.append_view(param.name);
                if(param.default_value != null) {
                    converter.str.append_view(" = ");
                    converter.convertJsNode(param.default_value);
                }
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
                 var is_existing_ucs = false;
                 if(decl.value != null && decl.value.kind == JsNodeKind.FunctionCall) {
                     var call = decl.value as *mut JsFunctionCall
                     if(call.callee != null && call.callee.kind == JsNodeKind.Identifier) {
                         var id = call.callee as *mut JsIdentifier
                         if(id.value.equals(view("$_ucs"))) {
                             is_existing_ucs = true;
                         }
                     }
                 }
                 var should_wrap_in_ucs = decl.value != null && !decl.name.empty() && decl.pattern == null &&
                     !is_existing_ucs && converter.expr_references_reactive_var(decl.value);
                 if(should_wrap_in_ucs) {
                     converter.computed_vars.push(decl.name);
                     converter.str.append_view("const ");
                     converter.str.append_view(decl.name);
                     converter.str.append_view(" = $_ucs(() => ");
                     converter.convertJsNode(decl.value);
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
                     if(is_existing_ucs) {
                         converter.computed_vars.push(decl.name);
                     }
                 }
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
             converter.str.append_view("return");
             if(ret.value != null) {
                 converter.str.append(' ');
                 converter.convertJsNode(ret.value);
             }
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
        JsNodeKind.TryCatch => {
             var t = node as *mut JsTryCatch
             converter.str.append_view("try ");
             if(t.tryBlock != null) converter.convertJsNode(t.tryBlock); else converter.str.append_view("{}");
             if(t.catchBlock != null) {
                 converter.str.append_view(" catch");
                 if(!t.catchParam.empty()) {
                     converter.str.append_view("(");
                     converter.str.append_view(t.catchParam);
                     converter.str.append_view(")");
                 }
                 converter.str.append_view(" ");
                 converter.convertJsNode(t.catchBlock);
             }
             if(t.finallyBlock != null) {
                 converter.str.append_view(" finally ");
                 converter.convertJsNode(t.finallyBlock);
             }
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
                     converter.convert_jsx_ssr_expression(container.expression);
                 }
                 return;
             }
             if(container.expression != null) {
                 if(container.expression.kind == JsNodeKind.Identifier) {
                     const id = container.expression as *mut JsIdentifier
                     if(converter.is_reactive_var(id.value)) {
                         converter.str.append_view(id.value);
                         return;
                     }
                 } else if(container.expression.kind == JsNodeKind.MemberAccess) {
                     const mem = container.expression as *mut JsMemberAccess
                     if(mem.object != null && mem.object.kind == JsNodeKind.Identifier && mem.property.equals(view("value"))) {
                         const id = mem.object as *mut JsIdentifier
                         if(converter.is_reactive_var(id.value)) {
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
        JsNodeKind.Paren => {
             var paren = node as *mut JsParen
             converter.str.append('(');
             converter.convertJsNode(paren.expression);
             converter.str.append(')');
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
