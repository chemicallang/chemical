
struct JsConverter {
    var builder : *mut ASTBuilder
    var support : *mut SymResSupport
    var vec : *mut VecRef<ASTNode>
    var parent : *mut ASTNode
    var str : std::string
    var jsx_parent : std::string_view
    var t_counter : int = 0
}

func (converter : &mut JsConverter) next_t() : std::string {
    converter.t_counter++
    var res = std::string()
    res.append_view("$c_t")
    res.append_integer(converter.t_counter as bigint)
    return res
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
    return converter.make_value_call_with(value, std::string_view("set_component_hash"), converter.support.setComponentHashFn)
}


func (converter : &mut JsConverter) make_char_chain(value : char) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    
    var name = std::string_view("append_head_js_char")
    var fnPtr = converter.support.appendHeadJsCharFn

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
    return converter.make_value_call_with(value, std::string_view("append_head_js_char_ptr"), converter.support.appendHeadJsCharPtrFn)
}

func (converter : &mut JsConverter) make_char_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_head_js_char"), converter.support.appendHeadJsCharFn)
}

func (converter : &mut JsConverter) make_integer_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_head_js_integer"), converter.support.appendHeadJsIntFn)
}

func (converter : &mut JsConverter) make_uinteger_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_head_js_uinteger"), converter.support.appendHeadJsUIntFn)
}

func (converter : &mut JsConverter) make_float_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_head_js_float"), converter.support.appendHeadJsFloatFn)
}

func (converter : &mut JsConverter) make_double_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_head_js_double"), converter.support.appendHeadJsDoubleFn)
}

func (converter : &mut JsConverter) make_value_call(value : *mut Value, len : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var name = std::string_view("append_head_js")
    
    var id = builder.make_identifier(name, converter.support.appendHeadJsFn, false, location);
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
            converter.str.append_view(varDecl.name)
            if(varDecl.value != null) {
                converter.str.append_view(" = ")
                converter.convertJsNode(varDecl.value)
            }
            converter.str.append_view(";")
        }
        JsNodeKind.Literal => {
            var literal = node as *mut JsLiteral
            converter.str.append_view(literal.value)
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
                converter.convertJsNode(arr.elements.get(i))
                i++
            }
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
                    converter.str.append_view(decl.name)
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
                 converter.str.append_view(decl.name)
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
                 converter.str.append_view(decl.name)
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
        JsNodeKind.JSXElement => {
            converter.convertJSXElement(node as *mut JsJSXElement)
        }
        JsNodeKind.JSXText => {
            converter.convertJSXText(node as *mut JsJSXText)
        }
        JsNodeKind.JSXExpressionContainer => {
            converter.convertJSXExpressionContainer(node as *mut JsJSXExpressionContainer)
        }
        JsNodeKind.JSXFragment => {
            converter.convertJSXFragment(node as *mut JsJSXFragment)
        }
    }
}

func (converter : &mut JsConverter) convertAttributeValue(attr : *mut JsJSXAttribute) {
    if(attr.value == null) {
        converter.str.append_view("true")
        return;
    }
    
    const jsNode = attr.value as *mut JsNode
    if(jsNode.kind == JsNodeKind.JSXExpressionContainer) {
        // Unwrap expression container for attributes to avoid generating statements (like const t = ...)
        // inside object literals or argument lists.
        const exprCont = jsNode as *mut JsJSXExpressionContainer
        converter.convertJsNode(exprCont.expression)
    } else {
        converter.convertJsNode(attr.value)
    }
}

func (converter : &mut JsConverter) convertJSXComponent(element : *mut JsJSXElement, tagName : std::string_view, tagNameNode : *mut JsNode) {
    const t = converter.next_t()
    
    // Generate children first
    var children_var_name = std::string("null")
    var has_children = element.children.size() > 0

    if(has_children) {
        if(!converter.str.empty()) converter.put_chain_in()

        if(element.children.size() == 1) {
            // Optimize for single child
            const child = element.children.get(0)
            
            if(child.kind == JsNodeKind.JSXText) {
                var text = child as *mut JsJSXText
                if(!text.value.empty()) {
                        const child_fn = converter.next_t()
                        converter.str.append_view("const ")
                        converter.str.append_view(child_fn.to_view())
                        converter.str.append_view(" = (props) => { return document.createTextNode(`")
                        converter.str.append_view(text.value)
                        converter.str.append_view("`); };")
                        children_var_name = child_fn
                } else {
                        children_var_name = std::string("null")
                }
            } else if(child.kind == JsNodeKind.JSXExpressionContainer) {
                    // Expression: const t = expr; children: typeof t === 'function' ? t : (props) => t
                    const child_expr = converter.next_t()
                    converter.str.append_view("const ")
                    converter.str.append_view(child_expr.to_view())
                    converter.str.append_view(" = ")
                    
                    var exprCont = child as *mut JsJSXExpressionContainer
                    // Temporarily clear jsx_parent so it doesn't try to append
                    const old_parent = converter.jsx_parent
                    converter.jsx_parent = std::string_view()
                    converter.convertJsNode(exprCont.expression) // This puts expression code
                    converter.jsx_parent = old_parent
                    
                    converter.str.append_view(";")
                    
                    // Create wrapper string
                    // We can't easily allocate a new variable for the wrapper here without complex string ops if we want inline, 
                    // but we can just use the expression in the prop value assignment?
                    // Wait, we need `children_var_name` to be the FINAL value passed.
                    
                    // Let's create the final variable
                    const final_var = converter.next_t()
                    converter.str.append_view("const ")
                    converter.str.append_view(final_var.to_view())
                    converter.str.append_view(" = (typeof ")
                    converter.str.append_view(child_expr.to_view())
                    converter.str.append_view(" === 'function') ? ")
                    converter.str.append_view(child_expr.to_view())
                    converter.str.append_view(" : (props) => ")
                    converter.str.append_view(child_expr.to_view())
                    converter.str.append_view(";")
                    
                    children_var_name = final_var
                    
            } else {
                    // Element/Fragment: Wrap in (props) => { ... return t; }
                    const child_fn = converter.next_t()
                    converter.str.append_view("const ")
                    converter.str.append_view(child_fn.to_view())
                    converter.str.append_view(" = (props) => { ")
                    
                    // We need to capture the variable name of the child
                    // We can peek t_counter?
                    // convertJsNode will generate `const t = ...`
                    // We need to know `t`.
                    
                    const old_parent = converter.jsx_parent
                    converter.jsx_parent = std::string_view() // Detached
                    
                    converter.convertJsNode(child)
                    
                    converter.jsx_parent = old_parent
                    
                    converter.str.append_view("return $c_t")
                    converter.str.append_integer(converter.t_counter as bigint)
                    converter.str.append_view("; };")
                    
                    children_var_name = child_fn
            }
            
        } else {
            // Multiple children -> Function returning DocumentFragment
            const child_fn = converter.next_t()
            converter.str.append_view("const ")
            converter.str.append_view(child_fn.to_view())
            converter.str.append_view(" = (props) => { ")
            
            const t_frag = converter.next_t()
            converter.str.append_view("const ")
            converter.str.append_view(t_frag.to_view())
            converter.str.append_view(" = document.createDocumentFragment();")
            
            const old_parent = converter.jsx_parent
            converter.jsx_parent = converter.builder.allocate_view(t_frag.to_view())
            
            for(var j : uint = 0; j < element.children.size(); j++) {
                converter.convertJsNode(element.children.get(j))
            }
            
            converter.jsx_parent = old_parent
            
            converter.str.append_view("return ")
            converter.str.append_view(t_frag.to_view())
            converter.str.append_view("; };")
            
            children_var_name = child_fn
        }
        
        converter.put_chain_in()
    }

    converter.str.append_view("const ")
    converter.str.append_view(t.to_view())
    converter.str.append_view(" = ")
    
    if(tagNameNode.kind == JsNodeKind.Identifier) {
        converter.str.append_view("$c_")
        converter.str.append_view(tagName)
    } else if(tagNameNode.kind == JsNodeKind.JSXExpressionContainer) {
            var exprCont = tagNameNode as *mut JsJSXExpressionContainer
            converter.convertJsNode(exprCont.expression)
    }
    
    converter.str.append_view("({")
    
    for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
        if(i > 0) converter.str.append_view(", ")
        const attrNode = element.opening.attributes.get(i)
        if(attrNode.kind == JsNodeKind.JSXAttribute) {
            const attr = attrNode as *mut JsJSXAttribute
            converter.str.append_view("\"")
            converter.str.append_view(attr.name)
            converter.str.append_view("\": ")
            converter.convertAttributeValue(attr)
        }
    }

    if(has_children) {
        if(!element.opening.attributes.empty()) {
            converter.str.append_view(", ")
        }
        converter.str.append_view("\"children\": ")
        converter.str.append_view(children_var_name.to_view())
    }
    
    converter.str.append_view("});")
    
    if(!converter.jsx_parent.empty()) {
        converter.str.append_view(converter.jsx_parent)
        converter.str.append_view(".appendChild(")
        converter.str.append_view(t.to_view())
        converter.str.append_view(");")
    }
}

func (converter : &mut JsConverter) convertJSXNativeElement(element : *mut JsJSXElement, tagName : std::string_view) {
    const t = converter.next_t()
    converter.str.append_view("const ")
    converter.str.append_view(t.to_view())
    converter.str.append_view(" = document.createElement(\"")
    converter.str.append_view(tagName)
    converter.str.append_view("\");")
    
    for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
        const attrNode = element.opening.attributes.get(i)
        if(attrNode.kind == JsNodeKind.JSXAttribute) {
            const attr = attrNode as *mut JsJSXAttribute
            converter.str.append_view(t.to_view())
            converter.str.append_view(".setAttribute(\"")
            converter.str.append_view(attr.name)
            converter.str.append_view("\", ")
            converter.convertAttributeValue(attr)
            converter.str.append_view(");")
        }
    }
    
    if(!converter.jsx_parent.empty()) {
        converter.str.append_view(converter.jsx_parent)
        converter.str.append_view(".appendChild(")
        converter.str.append_view(t.to_view())
        converter.str.append_view(");")
    }

    const old_parent = converter.jsx_parent
    converter.jsx_parent = converter.builder.allocate_view(t.to_view())
    
    for(var i : uint = 0; i < element.children.size(); i++) {
        converter.convertJsNode(element.children.get(i))
    }
    
    converter.jsx_parent = old_parent

    converter.put_chain_in()
}

func (converter : &mut JsConverter) convertJSXElement(element : *mut JsJSXElement) {
    if(!converter.str.empty()) converter.put_chain_in()
    
    const tagNameNode = element.opening.tagName as *mut JsNode
    var isComponent = false
    var tagName = std::string_view()

    if(tagNameNode.kind == JsNodeKind.Identifier) {
        const jsId = tagNameNode as *mut JsIdentifier
        tagName = jsId.value
        if(element.componentSignature != null || (tagName.size() > 0 && isupper(tagName.get(0) as int))) {
            isComponent = true
        }
    } else if(tagNameNode.kind == JsNodeKind.JSXExpressionContainer) {
        // <{Expression}> -> Expression({})
        isComponent = true
    } else {
        return
    }

    if(isComponent) {
        converter.convertJSXComponent(element, tagName, tagNameNode)
    } else {
        converter.convertJSXNativeElement(element, tagName)
    }
}

func (converter : &mut JsConverter) convertJSXText(text : *mut JsJSXText) {
    if(text.value.empty()) return
    if(!converter.str.empty()) converter.put_chain_in()
    
    converter.str.append_view(converter.jsx_parent)
    converter.str.append_view(".appendChild(document.createTextNode(`")
    converter.str.append_view(text.value)
    converter.str.append_view("`));")
    converter.put_chain_in()
}

func (converter : &mut JsConverter) convertJSXExpressionContainer(expr : *mut JsJSXExpressionContainer) {
    if(!converter.str.empty()) converter.put_chain_in()
    
    // We need to append the result to parent
    // document.createTextNode or directly if it returns a node?
    // React behavior: if it's a string/number -> text node, if it's a DOM element -> append directly.
    // For now assume it might be a string or Node.
    
    const t = converter.next_t()
    converter.str.append_view("const ")
    converter.str.append_view(t.to_view())
    converter.str.append_view(" = ")
    converter.convertJsNode(expr.expression)
    converter.str.append_view(";")
    
    converter.str.append_view("if(")
    converter.str.append_view(t.to_view())
    converter.str.append_view(" instanceof Node) { ")
    
    if(!converter.jsx_parent.empty()) {
        converter.str.append_view(converter.jsx_parent)
        converter.str.append_view(".appendChild(")
        converter.str.append_view(t.to_view())
        converter.str.append_view("); } else { ")
        converter.str.append_view(converter.jsx_parent)
        converter.str.append_view(".appendChild(document.createTextNode(")
        converter.str.append_view(t.to_view())
        converter.str.append_view(")); }")
    } else {
        converter.str.append_view("; } else { ; }")
    }
    
    converter.put_chain_in()
}

func (converter : &mut JsConverter) convertJSXFragment(fragment : *mut JsJSXFragment) {
    if(!converter.str.empty()) converter.put_chain_in()
    
    const t = converter.next_t()
    converter.str.append_view("const ")
    converter.str.append_view(t.to_view())
    converter.str.append_view(" = document.createDocumentFragment();")
    
    if(!converter.jsx_parent.empty()) {
        converter.str.append_view(converter.jsx_parent)
        converter.str.append_view(".appendChild(")
        converter.str.append_view(t.to_view())
        converter.str.append_view(");")
    }
    
    const old_parent = converter.jsx_parent
    converter.jsx_parent = converter.builder.allocate_view(t.to_view())
    
    for(var i : uint = 0; i < fragment.children.size(); i++) {
        converter.convertJsNode(fragment.children.get(i))
    }
    
    converter.jsx_parent = old_parent
    converter.put_chain_in()
}