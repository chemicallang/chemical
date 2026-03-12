enum BufferType {
    JavaScript,
    HTML
}

struct JsConverter {
    var builder : *mut ASTBuilder
    var support : *mut SymResSupport
    var vec : *mut VecRef<ASTNode>
    var tokens : *mut std::vector<TemplateToken> = null
    var parent : *mut ASTNode
    var str : std::string
    var jsx_parent : std::string_view
    var t_counter : int = 0
    var id_counter : int = 0
    var state_vars : std::vector<std::string_view>
    var target : BufferType = BufferType.JavaScript
}

func (converter : &mut JsConverter) flush_template_text() {
    if(converter.tokens == null || converter.str.empty()) return;
    converter.tokens.push(TemplateToken { kind : TemplateTokenKind.Text, value : converter.builder.allocate_view(converter.str.to_view()) });
    converter.str.clear();
}

func (converter : &mut JsConverter) append_hex(val : uint) {
    const hex = "0123456789ABCDEF"
    if (val == 0) {
        converter.str.append('0');
        return;
    }
    var buf : [16]char;
    var bi = 0;
    while(val > 0) {
        buf[bi++] = hex[val & 0xF]
        val >>= 4;
    }
    while(bi > 0) {
        converter.str.append(buf[--bi])
    }
}

func (converter : &mut JsConverter) escapeJs(text : std::string_view) {
    var i = 0u;
    var str = &mut converter.str
    while(i < text.size()) {
        const c1 = (text.data()[i] as uint) & 0xFF;
        if (c1 < 0x80) {
            str.append(c1 as char);
            i++;
        } else if ((c1 & 0xE0) == 0xC0) {
            if (i + 1 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const codepoint = ((c1 & 0x1F as uint) << 6u) | (c2 & 0x3F as uint);
                str.append_view("\\u{");
                converter.append_hex(codepoint);
                str.append('}');
                i += 2;
            } else { i++; }
        } else if ((c1 & 0xF0) == 0xE0) {
            if (i + 2 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const c3 = (text.data()[i+2] as uint) & 0xFF;
                const codepoint = ((c1 & 0x0F as uint) << 12u) | ((c2 & 0x3F as uint) << 6u) | (c3 & 0x3F as uint);
                str.append_view("\\u{");
                converter.append_hex(codepoint);
                str.append('}');
                i += 3;
            } else { i++; }
        } else if ((c1 & 0xF8) == 0xF0) {
            if (i + 3 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const c3 = (text.data()[i+2] as uint) & 0xFF;
                const c4 = (text.data()[i+3] as uint) & 0xFF;
                const codepoint = ((c1 & 0x07 as uint) << 18u) | ((c2 & 0x3F as uint) << 12u) | ((c3 & 0x3F as uint) << 6u) | (c4 & 0x3F as uint);
                str.append_view("\\u{");
                converter.append_hex(codepoint);
                str.append('}');
                i += 4;
            } else { i++; }
        } else {
            i++;
        }
    }
}

func (converter : &mut JsConverter) next_t() : std::string {
    converter.t_counter++
    var res = std::string()
    res.append_view("$c_t")
    res.append_integer(converter.t_counter as bigint)
    return res
}

func (converter : &mut JsConverter) is_state_var(name : std::string_view) : bool {
    for(var i : uint = 0; i < converter.state_vars.size(); i++) {
        if(converter.state_vars.get(i).equals(name)) {
            return true;
        }
    }
    return false;
}

func (converter : &mut JsConverter) make_require_component_call(hash : size_t) : *mut FunctionCall {
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

func (converter : &mut JsConverter) make_set_component_hash_call(hash : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var value = builder.make_ubigint_value(hash, location)
    return converter.make_value_call_with(value, std::string_view("set_component_hash"), converter.support.setComponentHashFn, converter.support.setComponentHashFn)
}


func (converter : &mut JsConverter) make_value_call_with(value : *mut Value, fn_name : std::string_view, jsFnPtr : *mut ASTNode, htmlFnPtr : *mut ASTNode) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    
    var name : std::string_view
    var fnPtr : *mut ASTNode
    if(converter.target == BufferType.JavaScript) {
        name = fn_name
        fnPtr = jsFnPtr
    } else {
        // Map append_js_... to append_html_...
        if(fn_name.equals(view("append_js_char_ptr"))) name = view("append_html_char_ptr");
        else if(fn_name.equals(view("append_js_char"))) name = view("append_html_char");
        else if(fn_name.equals(view("append_js_integer"))) name = view("append_html_integer");
        else if(fn_name.equals(view("append_js_uinteger"))) name = view("append_html_uinteger");
        else if(fn_name.equals(view("append_js_float"))) name = view("append_html_float");
        else if(fn_name.equals(view("append_js_double"))) name = view("append_html_double");
        else name = fn_name; // fallback
        
        fnPtr = htmlFnPtr
    }

    var id = builder.make_identifier(name, fnPtr, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func (converter : &mut JsConverter) make_char_ptr_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_char_ptr"), converter.support.appendHeadJsCharPtrFn, converter.support.appendHtmlCharPtrFn)
}

func (converter : &mut JsConverter) make_char_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_char"), converter.support.appendHeadJsCharFn, converter.support.appendHtmlCharFn)
}

func (converter : &mut JsConverter) make_integer_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_integer"), converter.support.appendHeadJsIntFn, converter.support.appendHtmlIntFn)
}

func (converter : &mut JsConverter) make_uinteger_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_uinteger"), converter.support.appendHeadJsUIntFn, converter.support.appendHtmlUIntFn)
}

func (converter : &mut JsConverter) make_float_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_float"), converter.support.appendHeadJsFloatFn, converter.support.appendHtmlFloatFn)
}

func (converter : &mut JsConverter) make_double_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_double"), converter.support.appendHeadJsDoubleFn, converter.support.appendHtmlDoubleFn)
}

func (converter : &mut JsConverter) make_value_call(value : *mut Value, len : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    
    var name : std::string_view
    var fnPtr : *mut ASTNode
    if(converter.target == BufferType.JavaScript) {
        name = std::string_view("append_js")
        fnPtr = converter.support.appendHeadJsFn
    } else {
        name = std::string_view("append_html")
        fnPtr = converter.support.appendHtmlFn
    }
    
    var id = builder.make_identifier(name, fnPtr, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    
    // len arg
    const len_val = builder.make_ubigint_value(len as ubigint, location);
    args.push(len_val)
    
    return call;
}

func (converter : &mut JsConverter) put_chain_in() {
    if(converter.str.empty()) return;
    
    const location = intrinsics::get_raw_location();
    const str_view = converter.builder.allocate_view(converter.str.to_view());
    const val = converter.builder.make_string_value(str_view, location);
    const call = converter.make_value_call(val, converter.str.size());
    converter.vec.push(call as *mut ASTNode);
    converter.str.clear();
}

func (converter : &mut JsConverter) put_char_chain(value : char) {
    const location = intrinsics::get_raw_location();
    var base = converter.builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    
    var name : std::string_view
    var fnPtr : *mut ASTNode
    if(converter.target == BufferType.JavaScript) {
        name = std::string_view("append_js_char")
        fnPtr = converter.support.appendHeadJsCharFn
    } else {
        name = std::string_view("append_html_char")
        fnPtr = converter.support.appendHtmlCharFn
    }

    var id = converter.builder.make_identifier(name, fnPtr, false, location);
    const chain = converter.builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = converter.builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    const char_val = converter.builder.make_char_value(value, location);
    args.push(char_val)
    converter.vec.push(call as *mut ASTNode)
}


func (converter : &mut JsConverter) put_wrapping(value : *mut Value) {
    const wrapped = converter.builder.make_value_wrapper(value, converter.parent)
    converter.vec.push(wrapped as *mut ASTNode);
}

func (converter : &mut JsConverter) put_wrapped_chemical_value_in(value : *mut Value) {
    const chain = converter.make_char_ptr_value_call(value)
    converter.vec.push(chain as *mut ASTNode)
}

func (converter : &mut JsConverter) put_wrapped_chemical_char_value_in(value : *mut Value) {
    var chain = converter.make_char_value_call(value);
    converter.vec.push(chain as *mut ASTNode)
}

func (converter : &mut JsConverter) put_wrapped_chemical_integer_value_in(value : *mut Value) {
    var chain = converter.make_integer_value_call(value);
    converter.vec.push(chain as *mut ASTNode)
}

func (converter : &mut JsConverter) put_wrapped_chemical_uinteger_value_in(value : *mut Value) {
    var chain = converter.make_uinteger_value_call(value);
    converter.vec.push(chain as *mut ASTNode)
}

func (converter : &mut JsConverter) put_wrapped_chemical_float_value_in(value : *mut Value) {
    var chain = converter.make_float_value_call(value);
    converter.vec.push(chain as *mut ASTNode)
}

func (converter : &mut JsConverter) put_wrapped_chemical_double_value_in(value : *mut Value) {
    var chain = converter.make_double_value_call(value);
    converter.vec.push(chain as *mut ASTNode)
}

func (converter : &mut JsConverter) put_by_type(type : *mut BaseType, value : *mut Value) {
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
            }
        }
        default => {
            converter.put_wrapped_chemical_value_in(value);
        }
    }
}

func (converter : &mut JsConverter) put_chemical_value_in(value : *mut Value) {
    converter.put_by_type(value.getType(), value)
}

func (converter : &mut JsConverter) convertChemicalValue(chem : *mut JsChemicalValue) {
    if(converter.tokens != null) {
        converter.flush_template_text();
        converter.tokens.push(TemplateToken { kind : TemplateTokenKind.ChemicalValue, chemicalValue : chem.value });
        return;
    }
    converter.put_chain_in()
    converter.put_chemical_value_in(chem.value)
}

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
                 if(container.expression != null && container.expression.kind == JsNodeKind.ChemicalValue) {
                     converter.convertChemicalValue(container.expression as *mut JsChemicalValue);
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
             }
             converter.convertJsNode(container.expression);
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

func (converter : &mut JsConverter) convertAttributeValue(attr : *mut JsJSXAttribute) {
    if(attr.value != null) {
         if(attr.value.kind == JsNodeKind.JSXExpressionContainer) {
             const container = attr.value as *mut JsJSXExpressionContainer
             if(container.expression != null) {
                 if(container.expression.kind == JsNodeKind.ChemicalValue) {
                     const cv = container.expression as *mut JsChemicalValue;
                     const cvType = cv.value.getType();
                     const isStr = cvType != null && (cvType.getKind() == BaseTypeKind.String ||
                         (cvType.getKind() == BaseTypeKind.Pointer) ||
                         (cvType.getKind() == BaseTypeKind.ExpressiveString));
                     if(isStr) {
                         converter.str.append('"');
                         converter.put_chain_in();
                         converter.put_chemical_value_in(cv.value);
                         converter.str.append('"');
                         return;
                     }
                 }
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
             }
         }
         converter.convertJsNode(attr.value);
    } else {
         converter.str.append_view("true");
    }
}

func (converter : &mut JsConverter) convertJSXComponent(element : *mut JsJSXElement, tagName : std::string_view, tagNameNode : *mut JsNode) {
    if(converter.target == BufferType.HTML) {
        if(element.componentSignature == null) {
             // Fallback to JS if no signature (maybe partial match or dynamic?)
             return;
        }
        
        const signature = element.componentSignature;
        const hash = signature.functionNode.getEncodedLocation();

        // Marker for hydration
        var idStr = std::string();
        idStr.append_view("u");
        idStr.append_uinteger(element.loc);
        
        converter.str.append_view("<div id=\"");
        converter.str.append_view(idStr.to_view());
        converter.str.append_view("\" data-u-comp=\"");
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, converter.str);
        converter.str.append_view("\">");
        converter.put_chain_in();

        // 1. Emit the require/if check and C++ call for SSR
        // This makes it work like html_cbi's components.
        var requireCall = converter.make_require_component_call(hash as size_t)
        var ifStmt = converter.builder.make_if_stmt(requireCall as *mut Value, converter.parent, intrinsics::get_raw_location())
        var body = ifStmt.get_body()
        
        body.push(converter.make_set_component_hash_call(hash as size_t))
        
        var base = converter.builder.make_identifier(signature.name, signature.functionNode as *mut ASTNode, false, intrinsics::get_raw_location())
        var pageId = converter.builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, intrinsics::get_raw_location())
        var call = converter.builder.make_function_call_node(base as *mut ChainValue, converter.parent, intrinsics::get_raw_location())
        call.get_args().push(pageId as *mut Value)
        
        // Push props (this is tricky, we need to convert JSX attributes to C++ struct initializer)
        // For now, only passing page.
        
        body.push(call as *mut ASTNode)
        converter.vec.push(ifStmt as *mut ASTNode)
        
        // 2. Emit hydration entry for JS
        // This should go to pageJs.
        const oldTarget = converter.target;
        converter.target = BufferType.JavaScript;
        converter.str.append_view("window.$_uq.push(['");
        converter.str.append_view(idStr.to_view());
        converter.str.append_view("','");
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, converter.str);
        converter.str.append_view("',{");
        var first = true;
        for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
            const attrNode = element.opening.attributes.get(i);
            if(attrNode.kind == JsNodeKind.JSXAttribute) {
                if(!first) converter.str.append(',');
                const attr = attrNode as *mut JsJSXAttribute;
                converter.str.append_view("\"");
                converter.str.append_view(attr.name);
                converter.str.append_view("\":");
                // Emit attribute value — quote ChemicalValue/ExpressiveString if it's a string type
                if(attr.value != null && attr.value.kind == JsNodeKind.JSXExpressionContainer) {
                    const cont = attr.value as *mut JsJSXExpressionContainer;
                    if(cont.expression != null && cont.expression.kind == JsNodeKind.ChemicalValue) {
                        const cv = cont.expression as *mut JsChemicalValue;
                        const cvType = cv.value.getType();
                        const isStr = cvType != null && (cvType.getKind() == BaseTypeKind.String ||
                            (cvType.getKind() == BaseTypeKind.Pointer) ||
                            (cvType.getKind() == BaseTypeKind.ExpressiveString));
                        if(isStr) {
                            // Emit as: "+chemValue+"
                            converter.str.append_view("\"");
                            converter.put_chain_in();
                            converter.put_chemical_value_in(cv.value);
                            converter.str.append_view("\"");
                        } else {
                            converter.convertAttributeValue(attr);
                        }
                    } else {
                        converter.convertAttributeValue(attr);
                    }
                } else {
                    converter.convertAttributeValue(attr);
                }
                first = false;
            }
        }
        converter.str.append_view("}]);");
        converter.put_chain_in();
        converter.target = oldTarget;

        converter.str.append_view("</div>");
        converter.put_chain_in();
        return;
    }

    converter.str.append_view("$_ur.createElement(");
    
    if(tagNameNode.kind == JsNodeKind.Identifier) {
        if(element.componentSignature != null) {
            get_module_scoped_name(element.componentSignature.functionNode as *mut ASTNode, tagName, converter.str);
        } else {
            converter.str.append_view(tagName);
        }
    } else {
        converter.convertJsNode(tagNameNode);
    }
    
    converter.str.append_view(", {");
    
    var attrCount = 0u;
    for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
        const attrNode = element.opening.attributes.get(i)
        if(attrNode.kind == JsNodeKind.JSXAttribute) {
            if(attrCount > 0) converter.str.append_view(", ");
            const attr = attrNode as *mut JsJSXAttribute
            var name = attr.name;
            if(name.equals("class")) name = std::string_view("className");
            else if(name.equals("for")) name = std::string_view("htmlFor");

            converter.str.append_view("\"");
            converter.str.append_view(name);
            converter.str.append_view("\": ");
            converter.convertAttributeValue(attr);
            attrCount++;
        } else if (attrNode.kind == JsNodeKind.JSXSpreadAttribute) {
            if(attrCount > 0) converter.str.append_view(", ");
            const spread = attrNode as *mut JsJSXSpreadAttribute
            converter.str.append_view("...");
            converter.convertJsNode(spread.argument);
            attrCount++;
        }
    }
    
    converter.str.append_view("}");
    
    if(!element.children.empty()) {
        for(var i : uint = 0; i < element.children.size(); i++) {
             converter.str.append_view(", ");
             converter.convertJsNode(element.children.get(i));
        }
    }
    
    converter.str.append_view(")");
}

func (converter : &mut JsConverter) make_append_attributes_spread_call(spreadExpr : *mut JsNode) {
    const builder = converter.builder;
    const location = intrinsics::get_raw_location();
    const support = converter.support;
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("append_html_attributes_spread"), support.appendHtmlAttributesSpreadFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    // The spread argument is a C++ value that comes from JsChemicalValue
    // We need to wrap the JSX spread argument expression.
    // It should be an identifier or member expression that the C++ side can call with.
    // We emit it as a void call wrapper.
    // For now we will do nothing — proper impl requires value from Chemical land.
    // TODO: pass the actual spread chemical value once the JSX spread argument
    //       resolves to a C++ struct with an append_html_attributes method.
    converter.vec.push(call as *mut ASTNode)
}

func (converter : &mut JsConverter) convertJSXNativeElement(element : *mut JsJSXElement, tagName : std::string_view) {
    if(converter.target == BufferType.HTML) {
        converter.str.append('<');
        converter.str.append_view(tagName);
        
        var classValueNodes = std::vector<*mut JsNode>();
        var otherAttrs = std::vector<*mut JsJSXAttribute>();
        var hasSpreads = false;
        
        for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
            const attrNode = element.opening.attributes.get(i);
            if(attrNode.kind == JsNodeKind.JSXAttribute) {
                const attr = attrNode as *mut JsJSXAttribute;
                if(attr.name.equals("class") || attr.name.equals("className")) {
                    if(attr.value != null) classValueNodes.push(attr.value);
                } else {
                    otherAttrs.push(attr);
                }
            } else if(attrNode.kind == JsNodeKind.JSXSpreadAttribute) {
                hasSpreads = true;
            }
        }
        
        // Emit class attribute if any
        if(!classValueNodes.empty()) {
            converter.str.append_view(" class=\"");
            for(var i : uint = 0; i < classValueNodes.size(); i++) {
                if(i > 0) converter.str.append(' ');
                const valNode = classValueNodes.get(i);
                if(valNode.kind == JsNodeKind.Literal) {
                    const lit = valNode as *mut JsLiteral;
                    var s = lit.value;
                    if(s.size() >= 2 && (s.get(0) == '"' || s.get(0) == '\'')) s = s.subview(1, s.size() - 2);
                    converter.escapeHtml(s);
                } else if(valNode.kind == JsNodeKind.JSXExpressionContainer) {
                    const cont = valNode as *mut JsJSXExpressionContainer;
                    converter.put_chain_in();
                    converter.convertJsNode(cont.expression);
                    converter.put_chain_in();
                }
            }
            converter.str.append('\"');
        }
        
        // Emit other named attributes
        for(var i : uint = 0; i < otherAttrs.size(); i++) {
            const attr = otherAttrs.get(i);
            converter.str.append(' ');
            converter.str.append_view(attr.name);
            if(attr.value != null) {
                converter.str.append_view("=\"");
                if(attr.value.kind == JsNodeKind.Literal) {
                    const lit = attr.value as *mut JsLiteral;
                    var s = lit.value;
                    if(s.size() >= 2 && (s.get(0) == '"' || s.get(0) == '\'')) s = s.subview(1, s.size() - 2);
                    converter.escapeHtml(s);
                } else if(attr.value.kind == JsNodeKind.JSXExpressionContainer) {
                    const cont = attr.value as *mut JsJSXExpressionContainer;
                    converter.put_chain_in();
                    converter.convertJsNode(cont.expression);
                    converter.put_chain_in();
                }
                converter.str.append('\"');
            }
        }
        
        // Finish tag opening (no self-closing yet, need spreads first)
        if(element.opening.selfClosing && !hasSpreads) {
            converter.str.append_view(" />");
            converter.put_chain_in();
            return;
        }
        
        // Flush name + known attrs so far, then handle spreads
        if(hasSpreads) {
            converter.put_chain_in();
            // Emit each spread as page.append_html_attributes_spread(expr)
            for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
                const attrNode = element.opening.attributes.get(i);
                if(attrNode.kind == JsNodeKind.JSXSpreadAttribute) {
                    const spread = attrNode as *mut JsJSXSpreadAttribute;
                    const builder = converter.builder;
                    const location = intrinsics::get_raw_location();
                    const support = converter.support;
                    // Only emit spread call if the page API supports it
                    if(support.appendHtmlAttributesSpreadFn != null &&
                       spread.argument != null && spread.argument.kind == JsNodeKind.ChemicalValue) {
                        const cv = spread.argument as *mut JsChemicalValue;
                        var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
                        var fnId = builder.make_identifier(std::string_view("append_html_attributes_spread"), support.appendHtmlAttributesSpreadFn, false, location);
                        const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, fnId ]), location);
                        var call = builder.make_function_call_node(chain, converter.parent, location);
                        call.get_args().push(cv.value);
                        converter.vec.push(call as *mut ASTNode);
                    }
                    // else: fallback — spread not supported yet or non-Chemical value
                }
            }
        }
        
        if(element.opening.selfClosing) {
            converter.str.append_view(" />");
        } else {
            converter.str.append('>');
            for(var i : uint = 0; i < element.children.size(); i++) {
                converter.convertJsNode(element.children.get(i));
            }
            converter.str.append_view("</");
            converter.str.append_view(tagName);
            converter.str.append('>');
        }
        converter.put_chain_in();
        return;
    }

    converter.str.append_view("$_ur.createElement(\"");
    converter.str.append_view(tagName);
    converter.str.append_view("\", ");
    
     var hasSpread = false;
     var attrMap = std::vector<*mut JsJSXAttribute>();
     for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
         const attrNode = element.opening.attributes.get(i);
         if(attrNode.kind == JsNodeKind.JSXAttribute) {
             attrMap.push(attrNode as *mut JsJSXAttribute);
         } else if(attrNode.kind == JsNodeKind.JSXSpreadAttribute) {
             hasSpread = true;
         }
     }

     if(hasSpread) {
         // Use merge helper $_um
         converter.str.append_view("$_um(");
         var first = true;
         for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
             const attrNode = element.opening.attributes.get(i);
             if(attrNode.kind == JsNodeKind.JSXSpreadAttribute) {
                 if(!first) converter.str.append_view(", ");
                 const spread = attrNode as *mut JsJSXSpreadAttribute;
                 converter.convertJsNode(spread.argument);
                 first = false;
             }
         }
         // Group static/named attributes into an object for merging
         if(!attrMap.empty()) {
             if(!first) converter.str.append_view(", ");
             converter.str.append_view("{");
             for(var i : uint = 0; i < attrMap.size(); i++) {
                 if(i > 0) converter.str.append_view(", ");
                 const attr = attrMap.get(i);
                 converter.str.append_view("\"");
                 converter.str.append_view(attr.name);
                 converter.str.append_view("\": ");
                 converter.convertAttributeValue(attr);
             }
             converter.str.append_view("}");
         }
         converter.str.append_view(")");
     } else {
         converter.str.append_view("{");
         var first = true;
         for(var i : uint = 0; i < attrMap.size(); i++) {
             const attr = attrMap.get(i);
             if(!first) converter.str.append_view(", ");
             converter.str.append_view("\"");
             converter.str.append_view(attr.name);
             converter.str.append_view("\": ");
             converter.convertAttributeValue(attr);
             first = false;
         }
         converter.str.append_view("}");
     }
    
    if(!element.children.empty()) {
        for(var i : uint = 0; i < element.children.size(); i++) {
             converter.str.append_view(", ");
             converter.convertJsNode(element.children.get(i));
        }
    }
    
    converter.str.append_view(")");
}

func (converter : &mut JsConverter) convertJSXElement(element : *mut JsJSXElement) {
    const tagNameNode = element.opening.tagName as *mut JsNode
    var isComponent = false
    var tagName = std::string_view()

    if(tagNameNode.kind == JsNodeKind.Identifier) {
        const jsId = tagNameNode as *mut JsIdentifier
        tagName = jsId.value
        if(element.componentSignature != null || (tagName.size() > 0 && tagName.get(0) >= 'A' && tagName.get(0) <= 'Z')) {
            isComponent = true
        }
    } else if(tagNameNode.kind == JsNodeKind.JSXExpressionContainer) {
        isComponent = true
    }

    if(isComponent) {
        converter.convertJSXComponent(element, tagName, tagNameNode)
    } else {
        converter.convertJSXNativeElement(element, tagName)
    }
}

func (converter : &mut JsConverter) convertJSXFragment(fragment : *mut JsJSXFragment) {
    if(converter.target == BufferType.HTML) {
        for(var i : uint = 0; i < fragment.children.size(); i++) {
             converter.convertJsNode(fragment.children.get(i));
        }
        return;
    }

    converter.str.append_view("$_ur.createElement($_ur.Fragment, null");
    
    for(var i : uint = 0; i < fragment.children.size(); i++) {
          converter.str.append_view(", ");
          converter.convertJsNode(fragment.children.get(i));
    }
    
    converter.str.append_view(")");
}

func (converter : &mut JsConverter) is_html_entity(text : std::string_view, index : uint) : bool {
    if (index + 2 >= text.size()) return false;
    if (text.data()[index] != '&') return false;

    var i = index + 1;
    if (text.data()[i] == '#') {
        i++;
        if (i < text.size() && (text.data()[i] == 'x' || text.data()[i] == 'X')) {
            i++;
            var start = i;
            while (i < text.size() && i - start < 8) {
                const c = text.data()[i];
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) i++;
                else break;
            }
            return (i > start && i < text.size() && text.data()[i] == ';');
        } else {
            var start = i;
            while (i < text.size() && i - start < 8) {
                const c = text.data()[i];
                if (c >= '0' && c <= '9') i++;
                else break;
            }
            return (i > start && i < text.size() && text.data()[i] == ';');
        }
    } else {
        var start = i;
        while (i < text.size() && i - start < 32) {
            const c = text.data()[i];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) i++;
            else break;
        }
        return (i > start && i < text.size() && text.data()[i] == ';');
    }
}

func (converter : &mut JsConverter) escapeHtml(text : std::string_view) {
    var i = 0u;
    var str = &mut converter.str
    while(i < text.size()) {
        const c1 = (text.data()[i] as uint) & 0xFF;
        if (c1 < 0x80) {
            const c = c1 as char;
            switch(c) {
                '&' => {
                    if (converter.is_html_entity(text, i)) str.append('&');
                    else str.append_view("&amp;");
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
        } else { i++; }
    }
}
