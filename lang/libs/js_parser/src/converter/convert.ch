/**
 * Shared JS AST -> JS text conversion.
 *
 * This is the common conversion logic that walks a parsed JsRoot (or a single
 * JsNode) and produces JS source text. It is shared between:
 *
 *  - the compiler plugin (js_cbi) which implements JsNodeEmitter by emitting
 *    `page.append_js(...)` calls into the chemical AST, and
 *  - the runtime package (js) which implements JsNodeEmitter by appending
 *    text into a std::string.
 */

public func escape_append_hex(emitter : &mut JsNodeEmitter, val : uint) {
    const hex = "0123456789ABCDEF"
    if (val == 0) {
        emitter.emit_char('0');
        return;
    }
    var buf : [16]char;
    var bi = 0;
    while(val > 0) {
        buf[bi++] = hex[val & 0xF]
        val >>= 4;
    }
    while(bi > 0) {
        emitter.emit_char(buf[--bi])
    }
}

public func escape_js_text(text : std::string_view, emitter : &mut JsNodeEmitter) {
    var i = 0u;
    while(i < text.size()) {
        const c1 = (text.data()[i] as uint) & 0xFF;
        if (c1 < 0x80) {
            if (c1 == '`' as uint) {
                emitter.emit_text("\\`");
            } else {
                emitter.emit_char(c1 as char);
            }
            i++;
        } else if ((c1 & 0xE0) == 0xC0) {
            if (i + 1 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const codepoint = ((c1 & (0x1F as uint)) << 6u) | (c2 & (0x3F as uint));
                emitter.emit_text("\\u{");
                escape_append_hex(emitter, codepoint);
                emitter.emit_char('}');
                i += 2;
            } else { i++; }
        } else if ((c1 & 0xF0) == 0xE0) {
            if (i + 2 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const c3 = (text.data()[i+2] as uint) & 0xFF;
                const codepoint = ((c1 & (0x0F as uint)) << 12u) | ((c2 & (0x3F as uint)) << 6u) | (c3 & (0x3F as uint));
                emitter.emit_text("\\u{");
                escape_append_hex(emitter, codepoint);
                emitter.emit_char('}');
                i += 3;
            } else { i++; }
        } else if ((c1 & 0xF8) == 0xF0) {
            if (i + 3 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const c3 = (text.data()[i+2] as uint) & 0xFF;
                const c4 = (text.data()[i+3] as uint) & 0xFF;
                const codepoint = ((c1 & (0x07 as uint)) << 18u) | ((c2 & (0x3F as uint)) << 12u) | ((c3 & (0x3F as uint)) << 6u) | (c4 & (0x3F as uint));
                emitter.emit_text("\\u{");
                escape_append_hex(emitter, codepoint);
                emitter.emit_char('}');
                i += 4;
            } else { i++; }
        } else {
            i++;
        }
    }
}

public func convert_js_node(node : *mut JsNode, emitter : &mut JsNodeEmitter) {
    switch(node.kind) {
        JsNodeKind.VarDecl => {
            var varDecl = node as *mut JsVarDecl
            if(varDecl.keyword.empty()) {
                emitter.emit_text("var ")
            } else {
                emitter.emit_text(&varDecl.keyword)
                emitter.emit_text(" ")
            }
            if(varDecl.pattern != null) {
                convert_js_node(varDecl.pattern, emitter)
            } else {
                emitter.emit_text(&varDecl.name)
            }
            if(varDecl.value != null) {
                emitter.emit_text(" = ")
                convert_js_node(varDecl.value, emitter)
            }
            emitter.emit_text(";")
        }
        JsNodeKind.Literal => {
            var literal = node as *mut JsLiteral
            const val = literal.value
            if (val.size() >= 2 && (val.get(0) == '\'' || val.get(0) == '\"' || val.get(0) == '`')) {
                emitter.emit_char(val.get(0));
                escape_js_text(std::string_view(val.data() + 1, val.size() - 2), emitter);
                emitter.emit_char(val.get(0));
            } else {
                emitter.emit_text(&val)
            }
        }
        JsNodeKind.Identifier => {
            var id = node as *mut JsIdentifier
            emitter.emit_text(&id.value)
        }
        JsNodeKind.ChemicalValue => {
            var chem = node as *mut JsChemicalValue
            emitter.flush()
            emitter.emit_chemical_value(chem.value)
        }
        JsNodeKind.FunctionCall => {
            var call = node as *mut JsFunctionCall
            convert_js_node(call.callee, emitter)
            emitter.emit_text("(")
            var i = 0u
            while(i < call.args.size()) {
                if(i > 0) emitter.emit_text(", ")
                convert_js_node(call.args.get(i), emitter)
                i++
            }
            emitter.emit_text(")")
        }
        JsNodeKind.Block => {
            var block = node as *mut JsBlock
            emitter.emit_text("{")
            var i = 0u
            while(i < block.statements.size()) {
                convert_js_node(block.statements.get(i), emitter)
                i++
            }
            emitter.emit_text("}")
        }
        JsNodeKind.If => {
            var ifStmt = node as *mut JsIf
            emitter.emit_text("if(")
            convert_js_node(ifStmt.condition, emitter)
            emitter.emit_text(")")
            convert_js_node(ifStmt.thenBlock, emitter)
            if(ifStmt.elseBlock != null) {
                emitter.emit_text(" else ")
                convert_js_node(ifStmt.elseBlock, emitter)
            }
        }
        JsNodeKind.Return => {
            var ret = node as *mut JsReturn
            emitter.emit_text("return")
            if(ret.value != null) {
                emitter.emit_text(" ")
                convert_js_node(ret.value, emitter)
            } else if(emitter.has_jsx_parent()) {
                emitter.emit_text(" $c_root")
            }
            emitter.emit_text(";")
        }
        JsNodeKind.BinaryOp => {
            var binOp = node as *mut JsBinaryOp
            if(binOp.left != null && binOp.left.kind == JsNodeKind.Ternary) {
                emitter.emit_text("(")
                convert_js_node(binOp.left, emitter)
                emitter.emit_text(")")
            } else {
                convert_js_node(binOp.left, emitter)
            }
            emitter.emit_text(" ")
            emitter.emit_text(&binOp.op)
            emitter.emit_text(" ")
            if(binOp.right != null && binOp.right.kind == JsNodeKind.Ternary) {
                emitter.emit_text("(")
                convert_js_node(binOp.right, emitter)
                emitter.emit_text(")")
            } else {
                convert_js_node(binOp.right, emitter)
            }
        }
        JsNodeKind.FunctionDecl => {
            var func_decl = node as *mut JsFunctionDecl
            const is_anon = func_decl.name.empty()
            if(is_anon) emitter.emit_char('(');
            if(func_decl.is_async) emitter.emit_text("async ")
            emitter.emit_text("function")
            if(func_decl.is_generator) emitter.emit_text("*")
            if(!is_anon) {
                emitter.emit_text(" ")
                emitter.emit_text(&func_decl.name)
            }
            emitter.emit_text("(")
            var i = 0u
            while(i < func_decl.params.size()) {
                if(i > 0) emitter.emit_text(", ")
                var param = func_decl.params.get_ptr(i)
                emitter.emit_text(&param.name)
                if(param.default_value != null) {
                    emitter.emit_text(" = ")
                    convert_js_node(param.default_value, emitter)
                }
                i++
            }
            emitter.emit_text(")")
            convert_js_node(func_decl.body, emitter)
            if(is_anon) emitter.emit_char(')');
        }
        JsNodeKind.MemberAccess => {
            var access = node as *mut JsMemberAccess
            convert_js_node(access.object, emitter)
            emitter.emit_text(".")
            emitter.emit_text(&access.property)
        }
        JsNodeKind.ExpressionStatement => {
            var stmt = node as *mut JsExpressionStatement
            convert_js_node(stmt.expression, emitter)
            emitter.emit_text(";")
        }
        JsNodeKind.ArrowFunction => {
            var arrow = node as *mut JsArrowFunction
            if(arrow.is_async) emitter.emit_text("async ")
            emitter.emit_text("(")
            var i = 0u
            while(i < arrow.params.size()) {
                if(i > 0) emitter.emit_text(", ")
                var param = arrow.params.get_ptr(i)
                emitter.emit_text(&param.name)
                if(param.default_value != null) {
                    emitter.emit_text(" = ")
                    convert_js_node(param.default_value, emitter)
                }
                i++
            }
            emitter.emit_text(") => ")
            
            // Check if body is a block or expression
            if(arrow.body != null) {
                var bodyNode = arrow.body as *mut JsNode
                if(bodyNode.kind == JsNodeKind.Block) {
                    convert_js_node(arrow.body, emitter)
                } else {
                    convert_js_node(arrow.body, emitter)
                }
            }
        }
        JsNodeKind.ArrayLiteral => {
            var arr = node as *mut JsArrayLiteral
            emitter.emit_text("[")
            var i = 0u
            while(i < arr.elements.size()) {
                if(i > 0) emitter.emit_text(", ")
                var elem = arr.elements.get(i)
                if(elem != null) {
                    convert_js_node(elem, emitter)
                }
                i++
            }
            emitter.emit_text("]")
        }
        JsNodeKind.ArrayDestructuring => {
            var arr = node as *mut JsArrayLiteral
            emitter.emit_text("[")
            var i = 0u
            while(i < arr.elements.size()) {
                if(i > 0) emitter.emit_text(", ")
                var elem = arr.elements.get(i)
                if(elem != null) {
                    convert_js_node(elem, emitter)
                }
                i++
            }
            emitter.emit_text("]")
        }
        JsNodeKind.IndexAccess => {
            var access = node as *mut JsIndexAccess
            convert_js_node(access.object, emitter)
            emitter.emit_text("[")
            convert_js_node(access.index, emitter)
            emitter.emit_text("]")
        }
        JsNodeKind.ObjectLiteral => {
            var obj = node as *mut JsObjectLiteral
            emitter.emit_text("{ ")
            var i = 0u
            while(i < obj.properties.size()) {
                if(i > 0) emitter.emit_text(", ")
                var prop = obj.properties.get(i)
                if(prop.value != null && prop.value.kind == JsNodeKind.Spread) {
                    convert_js_node(prop.value, emitter)
                } else {
                    emitter.emit_text(&prop.key)
                    emitter.emit_text(": ")
                    convert_js_node(prop.value, emitter)
                }
                i++
            }
            emitter.emit_text(" }")
        }
        JsNodeKind.For => {
            var forStmt = node as *mut JsFor
            emitter.emit_text("for(")
            if(forStmt.init != null) {
                // Init might be VarDecl or ExpressionStatement
                // We need to output it without the trailing ; that statements add
                var initNode = forStmt.init as *mut JsNode
                if(initNode.kind == JsNodeKind.VarDecl) {
                    var decl = forStmt.init as *mut JsVarDecl
                    emitter.emit_text(&decl.keyword)
                    emitter.emit_text(" ")
                    if(decl.pattern != null) {
                        convert_js_node(decl.pattern, emitter)
                    } else {
                        if(decl.pattern != null) { convert_js_node(decl.pattern, emitter) } else { emitter.emit_text(&decl.name) }
                    }
                    if(decl.value != null) {
                        emitter.emit_text(" = ")
                        convert_js_node(decl.value, emitter)
                    }
                } else if(initNode.kind == JsNodeKind.ExpressionStatement) {
                    var stmt = forStmt.init as *mut JsExpressionStatement
                    convert_js_node(stmt.expression, emitter)
                } else {
                    convert_js_node(forStmt.init, emitter)
                }
            }
            emitter.emit_text("; ")
            if(forStmt.condition != null) {
                convert_js_node(forStmt.condition, emitter)
            }
            emitter.emit_text("; ")
            if(forStmt.update != null) {
                convert_js_node(forStmt.update, emitter)
            }
            emitter.emit_text(")")
            convert_js_node(forStmt.body, emitter)
        }
        JsNodeKind.ForIn => {
            var forIn = node as *mut JsForIn
            emitter.emit_text("for(")
            // Special handling for VarDecl in loop header to avoid semicolon
            var initNode = forIn.left
            if(initNode.kind == JsNodeKind.VarDecl) {
                 var decl = initNode as *mut JsVarDecl
                 emitter.emit_text(&decl.keyword)
                 emitter.emit_text(" ")
                 if(decl.pattern != null) { convert_js_node(decl.pattern, emitter) } else { emitter.emit_text(&decl.name) }
            } else if(initNode.kind == JsNodeKind.ExpressionStatement) {
                 var stmt = initNode as *mut JsExpressionStatement
                 convert_js_node(stmt.expression, emitter)
            } else {
                 convert_js_node(initNode, emitter)
            }
            emitter.emit_text(" in ")
            convert_js_node(forIn.right, emitter)
            emitter.emit_text(")")
            convert_js_node(forIn.body, emitter)
        }
        JsNodeKind.ForOf => {
            var forOf = node as *mut JsForOf
            emitter.emit_text("for(")
            var initNode = forOf.left
            if(initNode.kind == JsNodeKind.VarDecl) {
                 var decl = initNode as *mut JsVarDecl
                 emitter.emit_text(&decl.keyword)
                 emitter.emit_text(" ")
                 if(decl.pattern != null) { convert_js_node(decl.pattern, emitter) } else { emitter.emit_text(&decl.name) }
            } else if(initNode.kind == JsNodeKind.ExpressionStatement) {
                 var stmt = initNode as *mut JsExpressionStatement
                 convert_js_node(stmt.expression, emitter)
            } else {
                 convert_js_node(initNode, emitter)
            }
            emitter.emit_text(" of ")
            convert_js_node(forOf.right, emitter)
            emitter.emit_text(")")
            convert_js_node(forOf.body, emitter)
        }
        JsNodeKind.While => {
            var whileStmt = node as *mut JsWhile
            emitter.emit_text("while(")
            convert_js_node(whileStmt.condition, emitter)
            emitter.emit_text(")")
            convert_js_node(whileStmt.body, emitter)
        }
        JsNodeKind.DoWhile => {
            var doWhile = node as *mut JsDoWhile
            emitter.emit_text("do ")
            convert_js_node(doWhile.body, emitter)
            emitter.emit_text(" while(")
            convert_js_node(doWhile.condition, emitter)
            emitter.emit_text(");")
        }
        JsNodeKind.Break => {
            emitter.emit_text("break;")
        }
        JsNodeKind.Continue => {
            emitter.emit_text("continue;")
        }
        JsNodeKind.Switch => {
            var switchStmt = node as *mut JsSwitch
            emitter.emit_text("switch(")
            convert_js_node(switchStmt.discriminant, emitter)
            emitter.emit_text(") {")
            var i = 0u
            while(i < switchStmt.cases.size()) {
                var c = switchStmt.cases.get_ptr(i)
                if(c.test == null) {
                    emitter.emit_text("default:")
                } else {
                    emitter.emit_text("case ")
                    convert_js_node(c.test, emitter)
                    emitter.emit_text(":")
                }
                var j = 0u
                while(j < c.body.size()) {
                    convert_js_node(c.body.get(j), emitter)
                    j++
                }
                i++
            }
            emitter.emit_text("}")
        }
        JsNodeKind.Throw => {
            var throwStmt = node as *mut JsThrow
            emitter.emit_text("throw ")
            convert_js_node(throwStmt.argument, emitter)
            emitter.emit_text(";")
        }
        JsNodeKind.TryCatch => {
            var tryCatch = node as *mut JsTryCatch
            emitter.emit_text("try ")
            convert_js_node(tryCatch.tryBlock, emitter)
            if(tryCatch.catchBlock != null) {
                emitter.emit_text(" catch")
                if(!tryCatch.catchParam.empty()) {
                    emitter.emit_text("(")
                    emitter.emit_text(&tryCatch.catchParam)
                    emitter.emit_text(")")
                }
                emitter.emit_text(" ")
                convert_js_node(tryCatch.catchBlock, emitter)
            }
            if(tryCatch.finallyBlock != null) {
                emitter.emit_text(" finally ")
                convert_js_node(tryCatch.finallyBlock, emitter)
            }
        }
        JsNodeKind.Ternary => {
            var tern = node as *mut JsTernary
            emitter.emit_text("(");
            convert_js_node(tern.condition, emitter);
            emitter.emit_text(" ? ");
            convert_js_node(tern.consequent, emitter);
            emitter.emit_text(" : ");
            convert_js_node(tern.alternate, emitter);
            emitter.emit_text(")");
        }
        JsNodeKind.UnaryOp => {
            var unary = node as *mut JsUnaryOp
            if(unary.prefix) {
                emitter.emit_text(&unary.operator)
                // Check if operator needs a space (if it's a word)
                if(unary.operator.size() > 2 && isalpha(unary.operator.get(0) as int)) {
                     emitter.emit_text(" ")
                }
                convert_js_node(unary.operand, emitter)
            } else {
                convert_js_node(unary.operand, emitter)
                emitter.emit_text(&unary.operator)
            }
        }
        JsNodeKind.Spread => {
            var spread = node as *mut JsSpread
            emitter.emit_text("...")
            convert_js_node(spread.argument, emitter)
        }
        JsNodeKind.ClassDecl => {
            var cls = node as *mut JsClassDecl
            emitter.emit_text("class ")
            if(!cls.name.empty()) {
                emitter.emit_text(&cls.name)
                emitter.emit_text(" ")
            }
            if(!cls.superClass.empty()) {
                emitter.emit_text("extends ")
                emitter.emit_text(&cls.superClass)
                emitter.emit_text(" ")
            }
            emitter.emit_text("{")
            var i = 0u
            while(i < cls.methods.size()) {
                var method = cls.methods.get_ptr(i)
                if(method.is_static) emitter.emit_text("static ")
                emitter.emit_text(&method.name)
                emitter.emit_text("(")
                var j = 0u
                while(j < method.params.size()) {
                    if(j > 0) emitter.emit_text(", ")
                    var param = method.params.get_ptr(j)
                    emitter.emit_text(&param.name)
                    if(param.default_value != null) {
                        emitter.emit_text(" = ")
                        convert_js_node(param.default_value, emitter)
                    }
                    j++
                }
                emitter.emit_text(") ")
                convert_js_node(method.body, emitter)
                i++
            }
            emitter.emit_text("}")
        }
        JsNodeKind.Debugger => {
             emitter.emit_text("debugger;")
        }
        JsNodeKind.Import => {
             var imp = node as *mut JsImport
             emitter.emit_text("import ")
             if(!imp.specifiers.empty()) {
                 if(imp.specifiers.get(0).imported.equals(std::string_view("default"))) {
                     emitter.emit_text(&imp.specifiers.get(0).local)
                     if(imp.specifiers.size() > 1) {
                         emitter.emit_text(", {")
                         var i = 1u
                         var found = false
                         while(i < imp.specifiers.size()) {
                             if(found) emitter.emit_text(", ")
                             var s = imp.specifiers.get_ptr(i)
                             if(!s.imported.equals(std::string_view("*"))) {
                                  emitter.emit_text(&s.imported)
                                  if(!s.imported.equals(&s.local)) {
                                       emitter.emit_text(" as ")
                                       emitter.emit_text(&s.local)
                                  }
                                  found = true
                             }
                             i++
                         }
                         emitter.emit_text("}")
                     }
                 } else if(imp.specifiers.get(0).imported.equals(std::string_view("*"))) {
                     emitter.emit_text("* as ")
                     emitter.emit_text(&imp.specifiers.get(0).local)
                 } else {
                     emitter.emit_text("{")
                     var i = 0u
                     while(i < imp.specifiers.size()) {
                         if(i > 0) emitter.emit_text(", ")
                         var s = imp.specifiers.get_ptr(i)
                         emitter.emit_text(&s.imported)
                         if(!s.imported.equals(&s.local)) {
                              emitter.emit_text(" as ")
                              emitter.emit_text(&s.local)
                         }
                         i++
                     }
                     emitter.emit_text("}")
                 }
                 emitter.emit_text(" from ")
             }
             emitter.emit_text(&imp.source)
             emitter.emit_text(";")
        }
        JsNodeKind.Export => {
             var jsExp = node as *mut JsExport
             emitter.emit_text("export ")
             if(jsExp.is_default) emitter.emit_text("default ")
             if(jsExp.declaration != null) {
                 convert_js_node(jsExp.declaration, emitter)
                 if(jsExp.is_default &&
                    jsExp.declaration.kind != JsNodeKind.FunctionDecl &&
                    jsExp.declaration.kind != JsNodeKind.ClassDecl) {
                     emitter.emit_text(";")
                 }
             }
        }
        JsNodeKind.Yield => {
             var yld = node as *mut JsYield
             emitter.emit_text("yield")
             if(yld.delegate) emitter.emit_text("*")
             if(yld.argument != null) {
                 emitter.emit_text(" ")
                 convert_js_node(yld.argument, emitter)
             }
        }
    }
}

public func convert_js_root(root : *mut JsRoot, emitter : &mut JsNodeEmitter) {
    var i = 0u
    while(i < root.statements.size()) {
        convert_js_node(root.statements.get(i), emitter)
        i++
    }
    emitter.flush()
}
