func checkHasJSX(node : *mut JsNode) : bool {
    if(node == null) return false;
    
    switch(node.kind) {
        JsNodeKind.JSXElement, JsNodeKind.JSXFragment => {
            return true;
        }
        JsNodeKind.Block => {
            var block = node as *mut JsBlock
            for(var i : uint = 0; i < block.statements.size(); i++) {
                if(checkHasJSX(block.statements.get(i))) return true;
            }
            return false;
        }
        JsNodeKind.ExpressionStatement => {
            var stmt = node as *mut JsExpressionStatement
            return checkHasJSX(stmt.expression);
        }
        JsNodeKind.If => {
            var ifStmt = node as *mut JsIf
            if(checkHasJSX(ifStmt.condition)) return true;
            if(checkHasJSX(ifStmt.thenBlock)) return true;
            if(checkHasJSX(ifStmt.elseBlock)) return true;
            return false;
        }
        JsNodeKind.Return => {
            var ret = node as *mut JsReturn
            return checkHasJSX(ret.value);
        }
        JsNodeKind.VarDecl => {
            var decl = node as *mut JsVarDecl
            return checkHasJSX(decl.value);
        }
        JsNodeKind.BinaryOp => {
            var bin = node as *mut JsBinaryOp
            return checkHasJSX(bin.left) || checkHasJSX(bin.right);
        }
        JsNodeKind.UnaryOp => {
            var un = node as *mut JsUnaryOp
            return checkHasJSX(un.operand);
        }
        JsNodeKind.FunctionCall => {
            var call = node as *mut JsFunctionCall
            if(checkHasJSX(call.callee)) return true;
            for(var i : uint = 0; i < call.args.size(); i++) {
                if(checkHasJSX(call.args.get(i))) return true;
            }
            return false;
        }
        JsNodeKind.MemberAccess => {
            var acc = node as *mut JsMemberAccess
            return checkHasJSX(acc.object);
        }
        JsNodeKind.ArrowFunction => {
             // Nested arrow function? If it contains JSX, should the parent be marked?
             // If nested arrow has JSX, it manages its own fragment.
             // Parent doesn't necessarily need to be marked unless the nested arrow ITSELF returns JSX elements that parent uses?
             // NOTE: The request is "every lambda that uses jsx like syntax should automatically become a component".
             // This implies local transformation.
             // Recursion should probably continue into children, BUT `checkHasJSX` is used to flag the *current* arrow function being parsed.
             // If a child arrow function has JSX, it will be flagged by its own check.
             // Does the parent need to know?
             // If I have `() => { return () => <div/> }`. Inner is component. Outer returns a component.
             // Outer does not "contain JSX elements" directly in its flow.
             // So we probably don't need to traverse into nested FunctionDecl/ArrowFunction bodies for *this* node's flag.
             return false;
        }
        JsNodeKind.FunctionDecl => {
             return false; // See above
        }
        JsNodeKind.For => {
            var f = node as *mut JsFor
            return checkHasJSX(f.init) || checkHasJSX(f.condition) || checkHasJSX(f.update) || checkHasJSX(f.body);
        }
        JsNodeKind.ForIn => {
            var f = node as *mut JsForIn
            return checkHasJSX(f.left) || checkHasJSX(f.right) || checkHasJSX(f.body);
        }
        JsNodeKind.ForOf => {
            var f = node as *mut JsForOf
            return checkHasJSX(f.left) || checkHasJSX(f.right) || checkHasJSX(f.body);
        }
        JsNodeKind.While => {
            var w = node as *mut JsWhile
            return checkHasJSX(w.condition) || checkHasJSX(w.body);
        }
        JsNodeKind.DoWhile => {
            var d = node as *mut JsDoWhile
            return checkHasJSX(d.condition) || checkHasJSX(d.body);
        }
        JsNodeKind.Switch => {
            var s = node as *mut JsSwitch
            if(checkHasJSX(s.discriminant)) return true;
            for(var i : uint = 0; i < s.cases.size(); i++) {
                var c = s.cases.get_ptr(i);
                if(checkHasJSX(c.test)) return true;
                for(var j : uint = 0; j < c.body.size(); j++) {
                    if(checkHasJSX(c.body.get(j))) return true;
                }
            }
            return false;
        }
        JsNodeKind.Ternary => {
            var t = node as *mut JsTernary
            return checkHasJSX(t.condition) || checkHasJSX(t.consequent) || checkHasJSX(t.alternate);
        }
        JsNodeKind.TryCatch => {
            var t = node as *mut JsTryCatch
            return checkHasJSX(t.tryBlock) || checkHasJSX(t.catchBlock) || checkHasJSX(t.finallyBlock);
        }
        JsNodeKind.Throw => {
            var t = node as *mut JsThrow
            return checkHasJSX(t.argument);
        }
        JsNodeKind.ArrayLiteral => {
            var a = node as *mut JsArrayLiteral
            for(var i : uint = 0; i < a.elements.size(); i++) {
                if(checkHasJSX(a.elements.get(i))) return true;
            }
            return false;
        }
        JsNodeKind.ObjectLiteral => {
            var o = node as *mut JsObjectLiteral
            for(var i : uint = 0; i < o.properties.size(); i++) {
                if(checkHasJSX(o.properties.get(i).value)) return true;
            }
            return false;
        }
        JsNodeKind.Spread => {
            var s = node as *mut JsSpread
            return checkHasJSX(s.argument);
        }
        JsNodeKind.ClassDecl => {
            return false; // Class methods are functions
        }
        default => {
            return false;
        }
    }
    return false;
}
