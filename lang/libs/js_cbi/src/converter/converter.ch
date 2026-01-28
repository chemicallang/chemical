
struct JsConverter {
    var builder : *mut ASTBuilder
    var support : *mut SymResSupport
    var vec : *mut VecRef<ASTNode>
    var parent : *mut ASTNode
    var str : std::string
    var jsx_parent : std::string_view
    var t_counter : int = 0
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
                const codepoint = ((c1 & (0x1F as uint)) << 6u) | (c2 & (0x3F as uint));
                str.append_view("\\u{");
                converter.append_hex(codepoint);
                str.append('}');
                i += 2;
            } else { i++; }
        } else if ((c1 & 0xF0) == 0xE0) {
            if (i + 2 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const c3 = (text.data()[i+2] as uint) & 0xFF;
                const codepoint = ((c1 & (0x0F as uint)) << 12u) | ((c2 & (0x3F as uint)) << 6u) | (c3 & (0x3F as uint));
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
                const codepoint = ((c1 & (0x07 as uint)) << 18u) | ((c2 & (0x3F as uint)) << 12u) | ((c3 & (0x3F as uint)) << 6u) | (c4 & (0x3F as uint));
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


func (converter : &mut JsConverter) make_char_chain(value : char) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    
    var name = std::string_view("append_js_char")
    var fnPtr = converter.support.appendJsCharFn

    var id = builder.make_identifier(name, fnPtr, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    const char_val = builder.make_char_value(value, location);
    args.push(char_val)
    return call;
}

func (converter : &mut JsConverter) make_value_call_with(value : *mut Value, fn_name : std::string_view, fnPtr : *mut ASTNode) : *mut FunctionCallNode {
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

func (converter : &mut JsConverter) make_char_ptr_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_char_ptr"), converter.support.appendJsCharPtrFn)
}

func (converter : &mut JsConverter) make_char_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_char"), converter.support.appendJsCharFn)
}

func (converter : &mut JsConverter) make_integer_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_integer"), converter.support.appendJsIntFn)
}

func (converter : &mut JsConverter) make_uinteger_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_uinteger"), converter.support.appendJsUIntFn)
}

func (converter : &mut JsConverter) make_float_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_float"), converter.support.appendJsFloatFn)
}

func (converter : &mut JsConverter) make_double_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_double"), converter.support.appendJsDoubleFn)
}

func (converter : &mut JsConverter) make_value_call(value : *mut Value, len : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var name = std::string_view("append_js")
    
    var id = builder.make_identifier(name, converter.support.appendJsFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    args.push(builder.make_ubigint_value(len, location));
    return call;
}

func (converter : &mut JsConverter) put_chain_in() {
    if(converter.str.empty()) return;
    
    const location = intrinsics::get_raw_location();
    const builder = converter.builder;
    const value = builder.make_string_value(builder.allocate_view(converter.str.to_view()), location)
    const size = converter.str.size()
    converter.str.clear();
    
    const call = converter.make_value_call(value, size);
    converter.vec.push(call);
}

func (converter : &mut JsConverter) put_wrapping(value : *mut Value) {
    const wrapped = converter.builder.make_value_wrapper(value, converter.parent)
    converter.vec.push(wrapped);
}

func (converter : &mut JsConverter) put_wrapped_chemical_value_in(value : *mut Value) {
    const chain = converter.make_char_ptr_value_call(value)
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_wrapped_chemical_char_value_in(value : *mut Value) {
    var chain = converter.make_char_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_wrapped_chemical_integer_value_in(value : *mut Value) {
    var chain = converter.make_integer_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_wrapped_chemical_uinteger_value_in(value : *mut Value) {
    var chain = converter.make_uinteger_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_wrapped_chemical_float_value_in(value : *mut Value) {
    var chain = converter.make_float_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_wrapped_chemical_double_value_in(value : *mut Value) {
    var chain = converter.make_double_value_call(value);
    converter.vec.push(chain)
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
        default => {
            converter.put_wrapped_chemical_value_in(value);
        }
    }
}

func (converter : &mut JsConverter) put_chemical_value_in(value : *mut Value) {
    converter.put_by_type(value.getType(), value)
}

func (converter : &mut JsConverter) convertJsNode(node : *mut JsNode) {
    switch(node.kind) {
        JsNodeKind.VarDecl => {
            var varDecl = node as *mut JsVarDecl
            if(varDecl.keyword.empty()) {
                converter.str.append_view("var ")
            } else {
                converter.str.append_view(varDecl.keyword)
                converter.str.append_view(" ")
            }
            if(varDecl.pattern != null) {
                converter.convertJsNode(varDecl.pattern)
            } else {
                converter.str.append_view(varDecl.name)
            }
            if(varDecl.value != null) {
                converter.str.append_view(" = ")
                converter.convertJsNode(varDecl.value)
            }
            converter.str.append_view(";")
        }
        JsNodeKind.Literal => {
            var literal = node as *mut JsLiteral
            const val = literal.value
            if (val.size() >= 2 && (val.get(0) == '\'' || val.get(0) == '\"' || val.get(0) == '`')) {
                converter.str.append(val.get(0));
                converter.escapeJs(std::string_view(val.data() + 1, val.size() - 2));
                converter.str.append(val.get(0));
            } else {
                converter.str.append_view(val)
            }
        }
        JsNodeKind.Identifier => {
            var id = node as *mut JsIdentifier
            converter.str.append_view(id.value)
        }
        JsNodeKind.ChemicalValue => {
            var chem = node as *mut JsChemicalValue
            converter.put_chain_in()
            converter.put_chemical_value_in(chem.value)
        }
        JsNodeKind.FunctionCall => {
            var call = node as *mut JsFunctionCall
            converter.convertJsNode(call.callee)
            converter.str.append_view("(")
            var i = 0u
            while(i < call.args.size()) {
                if(i > 0) converter.str.append_view(", ")
                converter.convertJsNode(call.args.get(i))
                i++
            }
            converter.str.append_view(")")
        }
        JsNodeKind.Block => {
            var block = node as *mut JsBlock
            converter.str.append_view("{")
            var i = 0u
            while(i < block.statements.size()) {
                converter.convertJsNode(block.statements.get(i))
                i++
            }
            converter.str.append_view("}")
        }
        JsNodeKind.If => {
            var ifStmt = node as *mut JsIf
            converter.str.append_view("if(")
            converter.convertJsNode(ifStmt.condition)
            converter.str.append_view(")")
            converter.convertJsNode(ifStmt.thenBlock)
            if(ifStmt.elseBlock != null) {
                converter.str.append_view(" else ")
                converter.convertJsNode(ifStmt.elseBlock)
            }
        }
        JsNodeKind.Return => {
            var ret = node as *mut JsReturn
            converter.str.append_view("return")
            if(ret.value != null) {
                converter.str.append_view(" ")
                converter.convertJsNode(ret.value)
            } else if(!converter.jsx_parent.empty()) {
                converter.str.append_view(" $c_root")
            }
            converter.str.append_view(";")
        }
        JsNodeKind.BinaryOp => {
            var binOp = node as *mut JsBinaryOp
            converter.convertJsNode(binOp.left)
            converter.str.append_view(" ")
            converter.str.append_view(binOp.op)
            converter.str.append_view(" ")
            converter.convertJsNode(binOp.right)
        }
        JsNodeKind.FunctionDecl => {
            var func_decl = node as *mut JsFunctionDecl
            if(func_decl.is_async) converter.str.append_view("async ")
            converter.str.append_view("function")
            if(func_decl.is_generator) converter.str.append_view("*")
            converter.str.append_view(" ")
            converter.str.append_view(func_decl.name)
            converter.str.append_view("(")
            var i = 0u
            while(i < func_decl.params.size()) {
                if(i > 0) converter.str.append_view(", ")
                converter.str.append_view(func_decl.params.get(i))
                i++
            }
            converter.str.append_view(")")
            converter.convertJsNode(func_decl.body)
        }
        JsNodeKind.MemberAccess => {
            var access = node as *mut JsMemberAccess
            converter.convertJsNode(access.object)
            converter.str.append_view(".")
            converter.str.append_view(access.property)
        }
        JsNodeKind.ExpressionStatement => {
            var stmt = node as *mut JsExpressionStatement
            converter.convertJsNode(stmt.expression)
            converter.str.append_view(";")
        }
        JsNodeKind.ArrowFunction => {
            var arrow = node as *mut JsArrowFunction
            if(arrow.is_async) converter.str.append_view("async ")
            converter.str.append_view("(")
            var i = 0u
            while(i < arrow.params.size()) {
                if(i > 0) converter.str.append_view(", ")
                converter.str.append_view(arrow.params.get(i))
                i++
            }
            converter.str.append_view(") => ")
            
            // Check if body is a block or expression
            if(arrow.body != null) {
                var bodyNode = arrow.body as *mut JsNode
                if(bodyNode.kind == JsNodeKind.Block) {
                    converter.convertJsNode(arrow.body)
                } else {
                    converter.convertJsNode(arrow.body)
                }
            }
        }
        JsNodeKind.ArrayLiteral => {
            var arr = node as *mut JsArrayLiteral
            converter.str.append_view("[")
            var i = 0u
            while(i < arr.elements.size()) {
                if(i > 0) converter.str.append_view(", ")
                var elem = arr.elements.get(i)
                if(elem != null) {
                    converter.convertJsNode(elem)
                }
                i++
            }
            converter.str.append_view("]")
        }
        JsNodeKind.ArrayDestructuring => {
            var arr = node as *mut JsArrayLiteral
            converter.str.append_view("[")
            var i = 0u
            while(i < arr.elements.size()) {
                if(i > 0) converter.str.append_view(", ")
                var elem = arr.elements.get(i)
                if(elem != null) {
                    converter.convertJsNode(elem)
                }
                i++
            }
            converter.str.append_view("]")
        }
        JsNodeKind.IndexAccess => {
            var access = node as *mut JsIndexAccess
            converter.convertJsNode(access.object)
            converter.str.append_view("[")
            converter.convertJsNode(access.index)
            converter.str.append_view("]")
        }
        JsNodeKind.ObjectLiteral => {
            var obj = node as *mut JsObjectLiteral
            converter.str.append_view("{ ")
            var i = 0u
            while(i < obj.properties.size()) {
                if(i > 0) converter.str.append_view(", ")
                var prop = obj.properties.get(i)
                converter.str.append_view(prop.key)
                converter.str.append_view(": ")
                converter.convertJsNode(prop.value)
                i++
            }
            converter.str.append_view(" }")
        }
        JsNodeKind.For => {
            var forStmt = node as *mut JsFor
            converter.str.append_view("for(")
            if(forStmt.init != null) {
                // Init might be VarDecl or ExpressionStatement
                // We need to output it without the trailing ; that statements add
                var initNode = forStmt.init as *mut JsNode
                if(initNode.kind == JsNodeKind.VarDecl) {
                    var decl = forStmt.init as *mut JsVarDecl
                    converter.str.append_view(decl.keyword)
                    converter.str.append_view(" ")
                    if(decl.pattern != null) {
                        converter.convertJsNode(decl.pattern)
                    } else {
                        if(decl.pattern != null) { converter.convertJsNode(decl.pattern) } else { converter.str.append_view(decl.name) }
                    }
                    if(decl.value != null) {
                        converter.str.append_view(" = ")
                        converter.convertJsNode(decl.value)
                    }
                } else if(initNode.kind == JsNodeKind.ExpressionStatement) {
                    var stmt = forStmt.init as *mut JsExpressionStatement
                    converter.convertJsNode(stmt.expression)
                } else {
                    converter.convertJsNode(forStmt.init)
                }
            }
            converter.str.append_view("; ")
            if(forStmt.condition != null) {
                converter.convertJsNode(forStmt.condition)
            }
            converter.str.append_view("; ")
            if(forStmt.update != null) {
                converter.convertJsNode(forStmt.update)
            }
            converter.str.append_view(")")
            converter.convertJsNode(forStmt.body)
        }
        JsNodeKind.ForIn => {
            var forIn = node as *mut JsForIn
            converter.str.append_view("for(")
            // Special handling for VarDecl in loop header to avoid semicolon
            var initNode = forIn.left
            if(initNode.kind == JsNodeKind.VarDecl) {
                 var decl = initNode as *mut JsVarDecl
                 converter.str.append_view(decl.keyword)
                 converter.str.append_view(" ")
                 if(decl.pattern != null) { converter.convertJsNode(decl.pattern) } else { converter.str.append_view(decl.name) }
            } else if(initNode.kind == JsNodeKind.ExpressionStatement) {
                 var stmt = initNode as *mut JsExpressionStatement
                 converter.convertJsNode(stmt.expression)
            } else {
                 converter.convertJsNode(initNode)
            }
            converter.str.append_view(" in ")
            converter.convertJsNode(forIn.right)
            converter.str.append_view(")")
            converter.convertJsNode(forIn.body)
        }
        JsNodeKind.ForOf => {
            var forOf = node as *mut JsForOf
            converter.str.append_view("for(")
            var initNode = forOf.left
            if(initNode.kind == JsNodeKind.VarDecl) {
                 var decl = initNode as *mut JsVarDecl
                 converter.str.append_view(decl.keyword)
                 converter.str.append_view(" ")
                 if(decl.pattern != null) { converter.convertJsNode(decl.pattern) } else { converter.str.append_view(decl.name) }
            } else if(initNode.kind == JsNodeKind.ExpressionStatement) {
                 var stmt = initNode as *mut JsExpressionStatement
                 converter.convertJsNode(stmt.expression)
            } else {
                 converter.convertJsNode(initNode)
            }
            converter.str.append_view(" of ")
            converter.convertJsNode(forOf.right)
            converter.str.append_view(")")
            converter.convertJsNode(forOf.body)
        }
        JsNodeKind.While => {
            var whileStmt = node as *mut JsWhile
            converter.str.append_view("while(")
            converter.convertJsNode(whileStmt.condition)
            converter.str.append_view(")")
            converter.convertJsNode(whileStmt.body)
        }
        JsNodeKind.DoWhile => {
            var doWhile = node as *mut JsDoWhile
            converter.str.append_view("do ")
            converter.convertJsNode(doWhile.body)
            converter.str.append_view(" while(")
            converter.convertJsNode(doWhile.condition)
            converter.str.append_view(");")
        }
        JsNodeKind.Break => {
            converter.str.append_view("break;")
        }
        JsNodeKind.Continue => {
            converter.str.append_view("continue;")
        }
        JsNodeKind.Switch => {
            var switchStmt = node as *mut JsSwitch
            converter.str.append_view("switch(")
            converter.convertJsNode(switchStmt.discriminant)
            converter.str.append_view(") {")
            var i = 0u
            while(i < switchStmt.cases.size()) {
                var c = switchStmt.cases.get_ptr(i)
                if(c.test == null) {
                    converter.str.append_view("default:")
                } else {
                    converter.str.append_view("case ")
                    converter.convertJsNode(c.test)
                    converter.str.append_view(":")
                }
                var j = 0u
                while(j < c.body.size()) {
                    converter.convertJsNode(c.body.get(j))
                    j++
                }
                i++
            }
            converter.str.append_view("}")
        }
        JsNodeKind.Throw => {
            var throwStmt = node as *mut JsThrow
            converter.str.append_view("throw ")
            converter.convertJsNode(throwStmt.argument)
            converter.str.append_view(";")
        }
        JsNodeKind.TryCatch => {
            var tryCatch = node as *mut JsTryCatch
            converter.str.append_view("try ")
            converter.convertJsNode(tryCatch.tryBlock)
            if(tryCatch.catchBlock != null) {
                converter.str.append_view(" catch")
                if(!tryCatch.catchParam.empty()) {
                    converter.str.append_view("(")
                    converter.str.append_view(tryCatch.catchParam)
                    converter.str.append_view(")")
                }
                converter.str.append_view(" ")
                converter.convertJsNode(tryCatch.catchBlock)
            }
            if(tryCatch.finallyBlock != null) {
                converter.str.append_view(" finally ")
                converter.convertJsNode(tryCatch.finallyBlock)
            }
        }
        JsNodeKind.Ternary => {
            var ternary = node as *mut JsTernary
            converter.convertJsNode(ternary.condition)
            converter.str.append_view(" ? ")
            converter.convertJsNode(ternary.consequent)
            converter.str.append_view(" : ")
            converter.convertJsNode(ternary.alternate)
        }
        JsNodeKind.UnaryOp => {
            var unary = node as *mut JsUnaryOp
            if(unary.prefix) {
                converter.str.append_view(unary.operator)
                // Check if operator needs a space (if it's a word)
                if(unary.operator.size() > 2 && isalpha(unary.operator.get(0) as int)) {
                     converter.str.append_view(" ")
                }
                converter.convertJsNode(unary.operand)
            } else {
                converter.convertJsNode(unary.operand)
                converter.str.append_view(unary.operator)
            }
        }
        JsNodeKind.Spread => {
            var spread = node as *mut JsSpread
            converter.str.append_view("...")
            converter.convertJsNode(spread.argument)
        }
        JsNodeKind.ClassDecl => {
            var cls = node as *mut JsClassDecl
            converter.str.append_view("class ")
            if(!cls.name.empty()) {
                converter.str.append_view(cls.name)
                converter.str.append_view(" ")
            }
            if(!cls.superClass.empty()) {
                converter.str.append_view("extends ")
                converter.str.append_view(cls.superClass)
                converter.str.append_view(" ")
            }
            converter.str.append_view("{")
            var i = 0u
            while(i < cls.methods.size()) {
                var method = cls.methods.get_ptr(i)
                if(method.is_static) converter.str.append_view("static ")
                converter.str.append_view(method.name)
                converter.str.append_view("(")
                var j = 0u
                while(j < method.params.size()) {
                    if(j > 0) converter.str.append_view(", ")
                    converter.str.append_view(method.params.get(j))
                    j++
                }
                converter.str.append_view(") ")
                converter.convertJsNode(method.body)
                i++
            }
            converter.str.append_view("}")
        }
        JsNodeKind.Debugger => {
             converter.str.append_view("debugger;")
        }
        JsNodeKind.Import => {
             var imp = node as *mut JsImport
             converter.str.append_view("import ")
             if(!imp.specifiers.empty()) {
                 if(imp.specifiers.get(0).imported.equals(view("default"))) {
                     converter.str.append_view(imp.specifiers.get(0).local)
                     if(imp.specifiers.size() > 1) {
                         converter.str.append_view(", {")
                         var i = 1u
                         var found = false
                         while(i < imp.specifiers.size()) {
                             if(found) converter.str.append_view(", ")
                             var s = imp.specifiers.get_ptr(i)
                             if(!s.imported.equals(view("*"))) {
                                  converter.str.append_view(s.imported)
                                  if(!s.imported.equals(s.local)) {
                                       converter.str.append_view(" as ")
                                       converter.str.append_view(s.local)
                                  }
                                  found = true
                             }
                             i++
                         }
                         converter.str.append_view("}")
                     }
                 } else if(imp.specifiers.get(0).imported.equals(view("*"))) {
                     converter.str.append_view("* as ")
                     converter.str.append_view(imp.specifiers.get(0).local)
                 } else {
                     converter.str.append_view("{")
                     var i = 0u
                     while(i < imp.specifiers.size()) {
                         if(i > 0) converter.str.append_view(", ")
                         var s = imp.specifiers.get_ptr(i)
                         converter.str.append_view(s.imported)
                         if(!s.imported.equals(s.local)) {
                              converter.str.append_view(" as ")
                              converter.str.append_view(s.local)
                         }
                         i++
                     }
                     converter.str.append_view("}")
                 }
                 converter.str.append_view(" from ")
             }
             converter.str.append_view(imp.source)
             converter.str.append_view(";")
        }
        JsNodeKind.Export => {
             var exp = node as *mut JsExport
             converter.str.append_view("export ")
             if(exp.is_default) converter.str.append_view("default ")
             if(exp.declaration != null) {
                 converter.convertJsNode(exp.declaration)
                 if(exp.is_default && 
                    exp.declaration.kind != JsNodeKind.FunctionDecl && 
                    exp.declaration.kind != JsNodeKind.ClassDecl) {
                     converter.str.append_view(";")
                 }
             }
        }
        JsNodeKind.Yield => {
             var yld = node as *mut JsYield
             converter.str.append_view("yield")
             if(yld.delegate) converter.str.append_view("*")
             if(yld.argument != null) {
                 converter.str.append_view(" ")
                 converter.convertJsNode(yld.argument)
             }
        }
    }
}


func (converter : &mut JsConverter) convertJsRoot(root : *mut JsRoot) {
    var i = 0u
    while(i < root.statements.size()) {
        converter.convertJsNode(root.statements.get(i))
        i++
    }
    converter.put_chain_in()
}
