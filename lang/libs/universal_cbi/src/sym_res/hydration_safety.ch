// Hydration-safety analysis (web-stack-plan.md §3.15b).
//
// A `#universal` component renders twice: once on the server (SSR) and once in
// the browser (hydration). Anything read during *render* therefore has to be
// identical on both sides. Reading the clock, a random number, or browser-only
// state in render position produces markup the client cannot reproduce, and the
// symptom is a hydration mismatch the user sees — historically debugged in a
// browser console, which is exactly the workflow this check exists to end.
//
// Scope rules:
//   * the component body, JSX attribute values, and `state` initializers are
//     render position  -> checked
//   * arrow/function bodies are handler/effect position -> exempt
// Exempting every lambda is deliberate: the framework only passes lambdas to
// event handlers and effects, so this is correct for the shipped surface, and a
// false negative is far cheaper than failing correct components.

func hs_is_browser_global(name : &std::string_view) : bool {
    if(name.equals(std::string_view("document"))) return true
    if(name.equals(std::string_view("window"))) return true
    if(name.equals(std::string_view("localStorage"))) return true
    if(name.equals(std::string_view("sessionStorage"))) return true
    if(name.equals(std::string_view("navigator"))) return true
    if(name.equals(std::string_view("fetch"))) return true
    if(name.equals(std::string_view("location"))) return true
    if(name.equals(std::string_view("history"))) return true
    return false
}

// `Date.now()`, `Math.random()`, `performance.now()` — non-deterministic values.
func hs_is_nondeterministic_call(callee : *mut JsNode) : std::string_view {
        unsafe {
        if(callee == null) return std::string_view()
        if(callee.kind != JsNodeKind.MemberAccess) return std::string_view()
        const m = callee as *mut JsMemberAccess
        if(m.object == null || m.object.kind != JsNodeKind.Identifier) return std::string_view()
        const recv = (m.object as *mut JsIdentifier).value
        const is_clock = m.property.equals(std::string_view("now")) || m.property.equals(std::string_view("random"))
        if(!is_clock) { return std::string_view() }
        const is_date = recv.equals(std::string_view("Date"))
        const is_math = recv.equals(std::string_view("Math"))
        const is_perf = recv.equals(std::string_view("performance"))
        if(is_date || is_math || is_perf) {
            return m.property
        }
        return std::string_view()

        }
}

func hs_push_unique(out : &mut std::vector<std::string_view>, name : std::string_view) {
        unsafe {
        for(var i : uint = 0; i < out.size(); i++) {
            if(out.at(i).equals(&name)) { return }
        }
        out.push(name)

        }
    }

func hs_report(diag : *mut ASTDiagnoser, compName : &std::string_view, what : &std::string_view, loc : ubigint) {
        unsafe {
        var msg = std::string("component '")
        msg.append_view(compName)
        msg.append_view("' reads '")
        msg.append_view(what)
        msg.append_view("' during render; SSR and hydration would disagree — move it into an event handler or an effect")
        diag.error(msg.to_view(), loc)

        }
}

func hs_seen(out : &std::vector<std::string_view>, name : &std::string_view) : bool {
        unsafe {
        for(var i : uint = 0; i < out.size(); i++) {
            if(out.at(i).equals(name)) { return true }
        }
        return false

        }
    }

func hydration_scan(node : *mut JsNode, compName : &std::string_view, diag : *mut ASTDiagnoser, loc : ubigint,
                    reported : &mut std::vector<std::string_view>, in_handler : bool) {
        unsafe {
        if(node == null) return
        switch(node.kind) {
            JsNodeKind.Identifier => {
                const id = node as *mut JsIdentifier
                if(!in_handler && hs_is_browser_global(&id.value)) {
                    if(!hs_seen(reported, &id.value)) {
                        hs_push_unique(reported, id.value)
                        hs_report(diag, compName, &id.value, loc)
                    }
                }
            }
            JsNodeKind.MemberAccess => {
                const m = node as *mut JsMemberAccess
                hydration_scan(m.object, compName, diag, loc, reported, in_handler)
            }
            JsNodeKind.FunctionCall => {
                const c = node as *mut JsFunctionCall
                if(!in_handler) {
                    const bad = hs_is_nondeterministic_call(c.callee)
                    if(bad.size() > 0 && !hs_seen(reported, &bad)) {
                        hs_push_unique(reported, bad)
                        hs_report(diag, compName, &bad, loc)
                    }
                }
                hydration_scan(c.callee, compName, diag, loc, reported, in_handler)
                for(var i : uint = 0; i < c.args.size(); i++) {
                    hydration_scan(c.args.at(i), compName, diag, loc, reported, in_handler)
                }
            }
            JsNodeKind.ExpressionStatement => { hydration_scan((node as *mut JsExpressionStatement).expression, compName, diag, loc, reported, in_handler) }
            JsNodeKind.Block => {
                const b = node as *mut JsBlock
                for(var i : uint = 0; i < b.statements.size(); i++) {
                    hydration_scan(b.statements.at(i), compName, diag, loc, reported, in_handler)
                }
            }
            JsNodeKind.If => {
                const s = node as *mut JsIf
                hydration_scan(s.condition, compName, diag, loc, reported, in_handler)
                hydration_scan(s.thenBlock, compName, diag, loc, reported, in_handler)
                hydration_scan(s.elseBlock, compName, diag, loc, reported, in_handler)
            }
            JsNodeKind.Return => { hydration_scan((node as *mut JsReturn).value, compName, diag, loc, reported, in_handler) }
            JsNodeKind.VarDecl => { hydration_scan((node as *mut JsVarDecl).value, compName, diag, loc, reported, in_handler) }
            // A lambda is handler/effect position: never render position.
            JsNodeKind.ArrowFunction => { hydration_scan((node as *mut JsArrowFunction).body, compName, diag, loc, reported, true) }
            JsNodeKind.BinaryOp => {
                const s = node as *mut JsBinaryOp
                hydration_scan(s.left, compName, diag, loc, reported, in_handler)
                hydration_scan(s.right, compName, diag, loc, reported, in_handler)
            }
            JsNodeKind.Ternary => {
                const s = node as *mut JsTernary
                hydration_scan(s.condition, compName, diag, loc, reported, in_handler)
                hydration_scan(s.consequent, compName, diag, loc, reported, in_handler)
                hydration_scan(s.alternate, compName, diag, loc, reported, in_handler)
            }
            JsNodeKind.UnaryOp => { hydration_scan((node as *mut JsUnaryOp).operand, compName, diag, loc, reported, in_handler) }
            JsNodeKind.Paren => { hydration_scan((node as *mut JsParen).expression, compName, diag, loc, reported, in_handler) }
            JsNodeKind.Spread => { hydration_scan((node as *mut JsSpread).argument, compName, diag, loc, reported, in_handler) }
            JsNodeKind.ArrayLiteral => {
                const a = node as *mut JsArrayLiteral
                for(var i : uint = 0; i < a.elements.size(); i++) {
                    hydration_scan(a.elements.at(i), compName, diag, loc, reported, in_handler)
                }
            }
            JsNodeKind.ObjectLiteral => {
                const o = node as *mut JsObjectLiteral
                for(var i : uint = 0; i < o.properties.size(); i++) {
                    hydration_scan(o.properties.at(i).value, compName, diag, loc, reported, in_handler)
                }
            }
            JsNodeKind.IndexAccess => {
                const s = node as *mut JsIndexAccess
                hydration_scan(s.object, compName, diag, loc, reported, in_handler)
                hydration_scan(s.index, compName, diag, loc, reported, in_handler)
            }
            JsNodeKind.JSXElement => {
                const el = node as *mut JsJSXElement
                for(var i : uint = 0; i < el.opening.attributes.size(); i++) {
                    hydration_scan(el.opening.attributes.at(i), compName, diag, loc, reported, in_handler)
                }
                for(var i : uint = 0; i < el.children.size(); i++) {
                    hydration_scan(el.children.at(i), compName, diag, loc, reported, in_handler)
                }
            }
            JsNodeKind.JSXExpressionContainer => { hydration_scan((node as *mut JsJSXExpressionContainer).expression, compName, diag, loc, reported, in_handler) }
            JsNodeKind.JSXAttribute => { hydration_scan((node as *mut JsJSXAttribute).value, compName, diag, loc, reported, in_handler) }
            JsNodeKind.JSXSpreadAttribute => { hydration_scan((node as *mut JsJSXSpreadAttribute).argument, compName, diag, loc, reported, in_handler) }
            JsNodeKind.JSXFragment => {
                const f = node as *mut JsJSXFragment
                for(var i : uint = 0; i < f.children.size(); i++) {
                    hydration_scan(f.children.at(i), compName, diag, loc, reported, in_handler)
                }
            }
            JsNodeKind.For => {
                const s = node as *mut JsFor
                hydration_scan(s.init, compName, diag, loc, reported, in_handler)
                hydration_scan(s.condition, compName, diag, loc, reported, in_handler)
                hydration_scan(s.update, compName, diag, loc, reported, in_handler)
                hydration_scan(s.body, compName, diag, loc, reported, in_handler)
            }
            JsNodeKind.ForIn => {
                const s = node as *mut JsForIn
                hydration_scan(s.left, compName, diag, loc, reported, in_handler)
                hydration_scan(s.right, compName, diag, loc, reported, in_handler)
                hydration_scan(s.body, compName, diag, loc, reported, in_handler)
            }
            JsNodeKind.ForOf => {
                const s = node as *mut JsForOf
                hydration_scan(s.left, compName, diag, loc, reported, in_handler)
                hydration_scan(s.right, compName, diag, loc, reported, in_handler)
                hydration_scan(s.body, compName, diag, loc, reported, in_handler)
            }
            JsNodeKind.While => {
                const s = node as *mut JsWhile
                hydration_scan(s.condition, compName, diag, loc, reported, in_handler)
                hydration_scan(s.body, compName, diag, loc, reported, in_handler)
            }
            JsNodeKind.DoWhile => {
                const s = node as *mut JsDoWhile
                hydration_scan(s.body, compName, diag, loc, reported, in_handler)
                hydration_scan(s.condition, compName, diag, loc, reported, in_handler)
            }
            JsNodeKind.Switch => {
                const s = node as *mut JsSwitch
                hydration_scan(s.discriminant, compName, diag, loc, reported, in_handler)
                for(var i : uint = 0; i < s.cases.size(); i++) {
                    const c = s.cases.get_ptr(i)
                    hydration_scan(c.test, compName, diag, loc, reported, in_handler)
                    for(var j : uint = 0; j < c.body.size(); j++) {
                        hydration_scan(c.body.at(j), compName, diag, loc, reported, in_handler)
                    }
                }
            }
            JsNodeKind.TryCatch => {
                const s = node as *mut JsTryCatch
                hydration_scan(s.tryBlock, compName, diag, loc, reported, in_handler)
                hydration_scan(s.catchBlock, compName, diag, loc, reported, in_handler)
                hydration_scan(s.finallyBlock, compName, diag, loc, reported, in_handler)
            }
            JsNodeKind.Throw => { hydration_scan((node as *mut JsThrow).argument, compName, diag, loc, reported, in_handler) }
            JsNodeKind.Yield => { hydration_scan((node as *mut JsYield).argument, compName, diag, loc, reported, in_handler) }
            default => { }
        }

        }
}

// Entry point: analyse one component body. Reports the first occurrence of each
// distinct offending construct, at the component's location.
public func hydration_check_body(body : *mut void, compName : &std::string_view, diag : *mut ASTDiagnoser, loc : ubigint) {
        unsafe {
        if(body == null || diag == null) return
        var reported = std::vector<std::string_view>()
        hydration_scan(body as *mut JsNode, compName, diag, loc, &mut reported, false)

        }
}
