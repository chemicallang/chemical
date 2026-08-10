/**
 * Runtime universal converter.
 *
 * Walks a parsed universal component AST (JsBlock of JS/JSX nodes) and
 * re-emits the JSX/JS source text into a std::string. The output string is
 * owned by the caller and passed as a pointer, so this struct has no
 * destructor and can be freely moved around.
 */
using namespace std;

public struct UniversalRuntimeConverter {
    var str : *mut std::string
}

func (c : &mut UniversalRuntimeConverter) text(view : &std::string_view) {
    c.str.append_view(view)
}

func (c : &mut UniversalRuntimeConverter) ch(ch : char) {
    c.str.append(ch)
}

public func convert_universal_node(node : *mut JsNode, c : &mut UniversalRuntimeConverter) {
    if(node == null) return
    switch(node.kind) {
        JsNodeKind.VarDecl => {
            var decl = node as *mut JsVarDecl
            if(decl.keyword.empty()) {
                c.text(std::string_view("var "))
            } else {
                c.text(&decl.keyword)
                c.text(std::string_view(" "))
            }
            if(decl.pattern != null) {
                convert_universal_node(decl.pattern, c)
            } else {
                c.text(&decl.name)
            }
            if(decl.value != null) {
                c.text(std::string_view(" = "))
                convert_universal_node(decl.value, c)
            }
            c.ch(';')
        }
        JsNodeKind.Literal => {
            var lit = node as *mut JsLiteral
            const val = lit.value
            if(val.size() >= 2 && (val.get(0) == '\'' || val.get(0) == '"' || val.get(0) == '`')) {
                c.ch(val.get(0))
                c.text(std::string_view(val.data() + 1, val.size() - 2))
                c.ch(val.get(0))
            } else {
                c.text(&val)
            }
        }
        JsNodeKind.Identifier => {
            var id = node as *mut JsIdentifier
            c.text(&id.value)
        }
        JsNodeKind.MemberAccess => {
            var acc = node as *mut JsMemberAccess
            convert_universal_node(acc.object, c)
            c.ch('.')
            c.text(&acc.property)
        }
        JsNodeKind.FunctionCall => {
            var call = node as *mut JsFunctionCall
            convert_universal_node(call.callee, c)
            c.ch('(')
            var i = 0u
            while(i < call.args.size()) {
                if(i > 0) c.text(std::string_view(", "))
                convert_universal_node(call.args.get(i), c)
                i++
            }
            c.ch(')')
        }
        JsNodeKind.ExpressionStatement => {
            var stmt = node as *mut JsExpressionStatement
            convert_universal_node(stmt.expression, c)
            c.ch(';')
        }
        JsNodeKind.Block => {
            var block = node as *mut JsBlock
            c.ch('{')
            var i = 0u
            while(i < block.statements.size()) {
                convert_universal_node(block.statements.get(i), c)
                i++
            }
            c.ch('}')
        }
        JsNodeKind.If => {
            var ifStmt = node as *mut JsIf
            c.text(std::string_view("if("))
            convert_universal_node(ifStmt.condition, c)
            c.ch(')')
            convert_universal_node(ifStmt.thenBlock, c)
            if(ifStmt.elseBlock != null) {
                c.text(std::string_view(" else "))
                convert_universal_node(ifStmt.elseBlock, c)
            }
        }
        JsNodeKind.Return => {
            var ret = node as *mut JsReturn
            c.text(std::string_view("return"))
            if(ret.value != null) {
                c.ch(' ')
                convert_universal_node(ret.value, c)
            }
            c.ch(';')
        }
        JsNodeKind.BinaryOp => {
            var bin = node as *mut JsBinaryOp
            convert_universal_node(bin.left, c)
            c.ch(' ')
            c.text(&bin.op)
            c.ch(' ')
            convert_universal_node(bin.right, c)
        }
        JsNodeKind.ArrowFunction => {
            var arrow = node as *mut JsArrowFunction
            if(arrow.is_async) c.text(std::string_view("async "))
            c.ch('(')
            var i = 0u
            while(i < arrow.params.size()) {
                if(i > 0) c.text(std::string_view(", "))
                var p = arrow.params.get_ptr(i)
                c.text(&p.name)
                if(p.default_value != null) {
                    c.text(std::string_view(" = "))
                    convert_universal_node(p.default_value, c)
                }
                i++
            }
            c.text(std::string_view(") => "))
            convert_universal_node(arrow.body, c)
        }
        JsNodeKind.ArrayLiteral => {
            var arr = node as *mut JsArrayLiteral
            c.ch('[')
            var i = 0u
            while(i < arr.elements.size()) {
                if(i > 0) c.text(std::string_view(", "))
                var e = arr.elements.get(i)
                if(e != null) convert_universal_node(e, c)
                i++
            }
            c.ch(']')
        }
        JsNodeKind.ArrayDestructuring => {
            var arr = node as *mut JsArrayLiteral
            c.ch('[')
            var i = 0u
            while(i < arr.elements.size()) {
                if(i > 0) c.text(std::string_view(", "))
                var e = arr.elements.get(i)
                if(e != null) convert_universal_node(e, c)
                i++
            }
            c.ch(']')
        }
        JsNodeKind.IndexAccess => {
            var acc = node as *mut JsIndexAccess
            convert_universal_node(acc.object, c)
            c.ch('[')
            convert_universal_node(acc.index, c)
            c.ch(']')
        }
        JsNodeKind.ObjectLiteral => {
            var obj = node as *mut JsObjectLiteral
            c.text(std::string_view("{ "))
            var i = 0u
            while(i < obj.properties.size()) {
                if(i > 0) c.text(std::string_view(", "))
                var prop = obj.properties.get(i)
                if(prop.value != null && prop.value.kind == JsNodeKind.Spread) {
                    convert_universal_node(prop.value, c)
                } else {
                    c.text(&prop.key)
                    c.text(std::string_view(": "))
                    convert_universal_node(prop.value, c)
                }
                i++
            }
            c.text(std::string_view(" }"))
        }
        JsNodeKind.For => {
            var f = node as *mut JsFor
            c.text(std::string_view("for("))
            if(f.init != null) {
                var initNode = f.init as *mut JsNode
                if(initNode.kind == JsNodeKind.VarDecl) {
                    var decl = f.init as *mut JsVarDecl
                    c.text(&decl.keyword)
                    c.ch(' ')
                    if(decl.pattern != null) {
                        convert_universal_node(decl.pattern, c)
                    } else {
                        c.text(&decl.name)
                    }
                    if(decl.value != null) {
                        c.text(std::string_view(" = "))
                        convert_universal_node(decl.value, c)
                    }
                } else if(initNode.kind == JsNodeKind.ExpressionStatement) {
                    var stmt = f.init as *mut JsExpressionStatement
                    convert_universal_node(stmt.expression, c)
                } else {
                    convert_universal_node(f.init, c)
                }
            }
            c.text(std::string_view("; "))
            if(f.condition != null) convert_universal_node(f.condition, c)
            c.text(std::string_view("; "))
            if(f.update != null) convert_universal_node(f.update, c)
            c.ch(')')
            convert_universal_node(f.body, c)
        }
        JsNodeKind.ForIn => {
            var f = node as *mut JsForIn
            c.text(std::string_view("for("))
            var initNode = f.left
            if(initNode.kind == JsNodeKind.VarDecl) {
                var decl = initNode as *mut JsVarDecl
                c.text(&decl.keyword)
                c.ch(' ')
                if(decl.pattern != null) { convert_universal_node(decl.pattern, c) } else { c.text(&decl.name) }
            } else if(initNode.kind == JsNodeKind.ExpressionStatement) {
                var stmt = initNode as *mut JsExpressionStatement
                convert_universal_node(stmt.expression, c)
            } else {
                convert_universal_node(initNode, c)
            }
            c.text(std::string_view(" in "))
            convert_universal_node(f.right, c)
            c.ch(')')
            convert_universal_node(f.body, c)
        }
        JsNodeKind.ForOf => {
            var f = node as *mut JsForOf
            c.text(std::string_view("for("))
            var initNode = f.left
            if(initNode.kind == JsNodeKind.VarDecl) {
                var decl = initNode as *mut JsVarDecl
                c.text(&decl.keyword)
                c.ch(' ')
                if(decl.pattern != null) { convert_universal_node(decl.pattern, c) } else { c.text(&decl.name) }
            } else if(initNode.kind == JsNodeKind.ExpressionStatement) {
                var stmt = initNode as *mut JsExpressionStatement
                convert_universal_node(stmt.expression, c)
            } else {
                convert_universal_node(initNode, c)
            }
            c.text(std::string_view(" of "))
            convert_universal_node(f.right, c)
            c.ch(')')
            convert_universal_node(f.body, c)
        }
        JsNodeKind.While => {
            var w = node as *mut JsWhile
            c.text(std::string_view("while("))
            convert_universal_node(w.condition, c)
            c.ch(')')
            convert_universal_node(w.body, c)
        }
        JsNodeKind.DoWhile => {
            var d = node as *mut JsDoWhile
            c.text(std::string_view("do "))
            convert_universal_node(d.body, c)
            c.text(std::string_view(" while("))
            convert_universal_node(d.condition, c)
            c.text(std::string_view(");"))
        }
        JsNodeKind.Break => {
            c.text(std::string_view("break;"))
        }
        JsNodeKind.Continue => {
            c.text(std::string_view("continue;"))
        }
        JsNodeKind.Switch => {
            var s = node as *mut JsSwitch
            c.text(std::string_view("switch("))
            convert_universal_node(s.discriminant, c)
            c.text(std::string_view(") {"))
            var i = 0u
            while(i < s.cases.size()) {
                var cs = s.cases.get_ptr(i)
                if(cs.test == null) {
                    c.text(std::string_view("default:"))
                } else {
                    c.text(std::string_view("case "))
                    convert_universal_node(cs.test, c)
                    c.ch(':')
                }
                var j = 0u
                while(j < cs.body.size()) {
                    convert_universal_node(cs.body.get(j), c)
                    j++
                }
                i++
            }
            c.ch('}')
        }
        JsNodeKind.Throw => {
            var t = node as *mut JsThrow
            c.text(std::string_view("throw "))
            convert_universal_node(t.argument, c)
            c.ch(';')
        }
        JsNodeKind.TryCatch => {
            var tc = node as *mut JsTryCatch
            c.text(std::string_view("try "))
            convert_universal_node(tc.tryBlock, c)
            if(tc.catchBlock != null) {
                c.text(std::string_view(" catch"))
                if(!tc.catchParam.empty()) {
                    c.ch('(')
                    c.text(&tc.catchParam)
                    c.ch(')')
                }
                c.ch(' ')
                convert_universal_node(tc.catchBlock, c)
            }
            if(tc.finallyBlock != null) {
                c.text(std::string_view(" finally "))
                convert_universal_node(tc.finallyBlock, c)
            }
        }
        JsNodeKind.Ternary => {
            var t = node as *mut JsTernary
            c.ch('(')
            convert_universal_node(t.condition, c)
            c.text(std::string_view(" ? "))
            convert_universal_node(t.consequent, c)
            c.text(std::string_view(" : "))
            convert_universal_node(t.alternate, c)
            c.ch(')')
        }
        JsNodeKind.UnaryOp => {
            var u = node as *mut JsUnaryOp
            if(u.prefix) {
                c.text(&u.operator)
                if(u.operator.size() > 2 && isalpha(u.operator.get(0) as int)) {
                    c.ch(' ')
                }
                convert_universal_node(u.operand, c)
            } else {
                convert_universal_node(u.operand, c)
                c.text(&u.operator)
            }
        }
        JsNodeKind.Spread => {
            var s = node as *mut JsSpread
            c.text(std::string_view("..."))
            convert_universal_node(s.argument, c)
        }
        JsNodeKind.FunctionDecl => {
            var f = node as *mut JsFunctionDecl
            const is_anon = f.name.empty()
            if(is_anon) c.ch('(')
            if(f.is_async) c.text(std::string_view("async "))
            c.text(std::string_view("function"))
            if(f.is_generator) c.ch('*')
            if(!is_anon) {
                c.ch(' ')
                c.text(&f.name)
            }
            c.ch('(')
            var i = 0u
            while(i < f.params.size()) {
                if(i > 0) c.text(std::string_view(", "))
                var p = f.params.get_ptr(i)
                c.text(&p.name)
                if(p.default_value != null) {
                    c.text(std::string_view(" = "))
                    convert_universal_node(p.default_value, c)
                }
                i++
            }
            c.ch(')')
            convert_universal_node(f.body, c)
            if(is_anon) c.ch(')')
        }
        JsNodeKind.ClassDecl => {
            var cls = node as *mut JsClassDecl
            c.text(std::string_view("class "))
            if(!cls.name.empty()) {
                c.text(&cls.name)
                c.ch(' ')
            }
            if(!cls.superClass.empty()) {
                c.text(std::string_view("extends "))
                c.text(&cls.superClass)
                c.ch(' ')
            }
            c.ch('{')
            var i = 0u
            while(i < cls.methods.size()) {
                var m = cls.methods.get_ptr(i)
                if(m.is_static) c.text(std::string_view("static "))
                c.text(&m.name)
                c.ch('(')
                var j = 0u
                while(j < m.params.size()) {
                    if(j > 0) c.text(std::string_view(", "))
                    var p = m.params.get_ptr(j)
                    c.text(&p.name)
                    if(p.default_value != null) {
                        c.text(std::string_view(" = "))
                        convert_universal_node(p.default_value, c)
                    }
                    j++
                }
                c.text(std::string_view(") "))
                convert_universal_node(m.body, c)
                i++
            }
            c.ch('}')
        }
        JsNodeKind.Import => {
            var imp = node as *mut JsImport
            c.text(std::string_view("import "))
            if(imp.specifiers.size() > 0) {
                if(imp.specifiers.get(0).imported.equals(std::string_view("default"))) {
                    c.text(&imp.specifiers.get(0).local)
                    if(imp.specifiers.size() > 1) {
                        c.text(std::string_view(", {"))
                        var i = 1u
                        while(i < imp.specifiers.size()) {
                            if(i > 1) c.text(std::string_view(", "))
                            var s = imp.specifiers.get_ptr(i)
                            if(!s.imported.equals(std::string_view("*"))) {
                                c.text(&s.imported)
                                if(!s.imported.equals(&s.local)) {
                                    c.text(std::string_view(" as "))
                                    c.text(&s.local)
                                }
                            }
                            i++
                        }
                        c.ch('}')
                    }
                } else if(imp.specifiers.get(0).imported.equals(std::string_view("*"))) {
                    c.text(std::string_view("* as "))
                    c.text(&imp.specifiers.get(0).local)
                } else {
                    c.ch('{')
                    var i = 0u
                    while(i < imp.specifiers.size()) {
                        if(i > 0) c.text(std::string_view(", "))
                        var s = imp.specifiers.get_ptr(i)
                        c.text(&s.imported)
                        if(!s.imported.equals(&s.local)) {
                            c.text(std::string_view(" as "))
                            c.text(&s.local)
                        }
                        i++
                    }
                    c.ch('}')
                }
                c.text(std::string_view(" from "))
            }
            c.text(&imp.source)
            c.ch(';')
        }
        JsNodeKind.Export => {
            var e = node as *mut JsExport
            c.text(std::string_view("export "))
            if(e.is_default) c.text(std::string_view("default "))
            if(e.declaration != null) {
                convert_universal_node(e.declaration, c)
            }
        }
        JsNodeKind.Yield => {
            var y = node as *mut JsYield
            c.text(std::string_view("yield"))
            if(y.delegate) c.ch('*')
            if(y.argument != null) {
                c.ch(' ')
                convert_universal_node(y.argument, c)
            }
        }
        JsNodeKind.Debugger => {
            c.text(std::string_view("debugger;"))
        }
        JsNodeKind.JSXText => {
            var text = node as *mut JsJSXText
            c.text(&text.value)
        }
        JsNodeKind.JSXExpressionContainer => {
            var container = node as *mut JsJSXExpressionContainer
            c.ch('{')
            if(container.expression != null) {
                convert_universal_node(container.expression, c)
            }
            c.ch('}')
        }
        JsNodeKind.JSXAttribute => {
            var attr = node as *mut JsJSXAttribute
            c.text(&attr.name)
            if(attr.value != null) {
                c.ch('=')
                convert_universal_node(attr.value, c)
            }
        }
        JsNodeKind.JSXSpreadAttribute => {
            var spread = node as *mut JsJSXSpreadAttribute
            c.text(std::string_view("{..."))
            convert_universal_node(spread.argument, c)
            c.ch('}')
        }
        JsNodeKind.JSXElement => {
            var elem = node as *mut JsJSXElement
            c.ch('<')
            convert_universal_node(elem.opening.tagName, c)
            var i = 0u
            while(i < elem.opening.attributes.size()) {
                c.ch(' ')
                convert_universal_node(elem.opening.attributes.get(i), c)
                i++
            }
            if(elem.opening.selfClosing) {
                c.text(std::string_view(" />"))
                return
            }
            c.ch('>')
            i = 0u
            while(i < elem.children.size()) {
                convert_universal_node(elem.children.get(i), c)
                i++
            }
            c.text(std::string_view("</"))
            convert_universal_node(elem.closing.tagName, c)
            c.ch('>')
        }
        JsNodeKind.JSXFragment => {
            var frag = node as *mut JsJSXFragment
            c.text(std::string_view("<>"))
            var i = 0u
            while(i < frag.children.size()) {
                convert_universal_node(frag.children.get(i), c)
                i++
            }
            c.text(std::string_view("</>"))
        }
        JsNodeKind.Paren => {
            var paren = node as *mut JsParen
            c.ch('(')
            convert_universal_node(paren.expression, c)
            c.ch(')')
        }
        JsNodeKind.ChemicalValue => {
            // embedded chemical values are not re-emitted at runtime
        }
        default => {
        }
    }
}

public func convert_universal_root(root : *mut JsBlock, c : &mut UniversalRuntimeConverter) {
    var i = 0u
    while(i < root.statements.size()) {
        convert_universal_node(root.statements.get(i), c)
        i++
    }
}
