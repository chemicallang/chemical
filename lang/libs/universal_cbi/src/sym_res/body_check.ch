// Component body check (Phase 2): the client/JS job resolves browser host names
// against the per-job ambient index built from marked provider modules.
//
// A missing root is `WEB-BODY-008`. Only the closed host-name list is checked;
// `Math`, `console`, hooks, framework internals, import locals and arrow
// parameters are ordinary JS names and stay uncounted in this phase.
//
// The server job generates SSR and must not run the browser-surface check, so
// the entry point returns immediately when `universal_job_begin` recorded a
// non-client job.

func bc_is_host_name(name : &std::string_view) : bool {
        unsafe {
        if(name.equals(std::string_view("document"))) return true
        if(name.equals(std::string_view("window"))) return true
        if(name.equals(std::string_view("localStorage"))) return true
        if(name.equals(std::string_view("sessionStorage"))) return true
        if(name.equals(std::string_view("navigator"))) return true
        if(name.equals(std::string_view("fetch"))) return true
        if(name.equals(std::string_view("location"))) return true
        if(name.equals(std::string_view("history"))) return true
        if(name.equals(std::string_view("WebSocket"))) return true
        return false

        }
}

func bc_list_has(list : &std::vector<std::string_view>, name : &std::string_view) : bool {
        unsafe {
        for(var i : uint = 0; i < list.size(); i++) {
            if(list.at(i).equals(name)) { return true }
        }
        return false

        }
}

func bc_list_push(list : &mut std::vector<std::string_view>, name : &std::string_view) {
        unsafe {
        if(name.size() == 0) { return }
        if(!bc_list_has(list, name)) {
            list.push(*name)
        }

        }
}

// Collects names in local position (var/function/parameter/catch bindings) so a
// host spelling shadowed by a local is never reported as a missing global, and
// collects host-name uses so a single pass can report them afterwards.
func bc_collect(node : *mut JsNode, locals : &mut std::vector<std::string_view>,
                used : &mut std::vector<std::string_view>) {
        unsafe {
        if(node == null) return
        switch(node.kind) {
            JsNodeKind.Identifier => {
                const id = node as *mut JsIdentifier
                if(bc_is_host_name(&id.value)) {
                    bc_list_push(used, &id.value)
                }
            }
            JsNodeKind.MemberAccess => {
                const m = node as *mut JsMemberAccess
                bc_collect(m.object, locals, used)
            }
            JsNodeKind.FunctionCall => {
                const c = node as *mut JsFunctionCall
                bc_collect(c.callee, locals, used)
                for(var i : uint = 0; i < c.args.size(); i++) {
                    bc_collect(c.args.at(i), locals, used)
                }
            }
            JsNodeKind.ExpressionStatement => { bc_collect((node as *mut JsExpressionStatement).expression, locals, used) }
            JsNodeKind.Block => {
                const b = node as *mut JsBlock
                for(var i : uint = 0; i < b.statements.size(); i++) {
                    bc_collect(b.statements.at(i), locals, used)
                }
            }
            JsNodeKind.If => {
                const s = node as *mut JsIf
                bc_collect(s.condition, locals, used)
                bc_collect(s.thenBlock, locals, used)
                bc_collect(s.elseBlock, locals, used)
            }
            JsNodeKind.Return => { bc_collect((node as *mut JsReturn).value, locals, used) }
            JsNodeKind.VarDecl => {
                const v = node as *mut JsVarDecl
                bc_list_push(locals, &v.name)
                bc_collect(v.value, locals, used)
                bc_collect(v.next as *mut JsNode, locals, used)
            }
            // Lambdas are handler/effect position for hydration, but they are
            // still component body source: a browser global used in an event
            // handler must resolve to a provider too.
            JsNodeKind.ArrowFunction => {
                const f = node as *mut JsArrowFunction
                for(var i : uint = 0; i < f.params.size(); i++) {
                    bc_list_push(locals, &f.params.at(i).name)
                }
                bc_collect(f.body, locals, used)
            }
            JsNodeKind.FunctionDecl => {
                const f = node as *mut JsFunctionDecl
                bc_list_push(locals, &f.name)
                for(var i : uint = 0; i < f.params.size(); i++) {
                    bc_list_push(locals, &f.params.at(i).name)
                }
                bc_collect(f.body, locals, used)
            }
            JsNodeKind.ClassDecl => {
                const c = node as *mut JsClassDecl
                for(var i : uint = 0; i < c.methods.size(); i++) {
                    const m = c.methods.get_ptr(i)
                    for(var j : uint = 0; j < m.params.size(); j++) {
                        bc_list_push(locals, &m.params.at(j).name)
                    }
                    bc_collect(m.body, locals, used)
                }
            }
            JsNodeKind.BinaryOp => {
                const s = node as *mut JsBinaryOp
                bc_collect(s.left, locals, used)
                bc_collect(s.right, locals, used)
            }
            JsNodeKind.Ternary => {
                const s = node as *mut JsTernary
                bc_collect(s.condition, locals, used)
                bc_collect(s.consequent, locals, used)
                bc_collect(s.alternate, locals, used)
            }
            JsNodeKind.UnaryOp => { bc_collect((node as *mut JsUnaryOp).operand, locals, used) }
            JsNodeKind.Paren => { bc_collect((node as *mut JsParen).expression, locals, used) }
            JsNodeKind.Spread => { bc_collect((node as *mut JsSpread).argument, locals, used) }
            JsNodeKind.ArrayLiteral => {
                const a = node as *mut JsArrayLiteral
                for(var i : uint = 0; i < a.elements.size(); i++) {
                    bc_collect(a.elements.at(i), locals, used)
                }
            }
            JsNodeKind.ObjectLiteral => {
                const o = node as *mut JsObjectLiteral
                for(var i : uint = 0; i < o.properties.size(); i++) {
                    bc_collect(o.properties.at(i).value, locals, used)
                }
            }
            JsNodeKind.IndexAccess => {
                const s = node as *mut JsIndexAccess
                bc_collect(s.object, locals, used)
                bc_collect(s.index, locals, used)
            }
            JsNodeKind.JSXElement => {
                const el = node as *mut JsJSXElement
                for(var i : uint = 0; i < el.opening.attributes.size(); i++) {
                    bc_collect(el.opening.attributes.at(i), locals, used)
                }
                for(var i : uint = 0; i < el.children.size(); i++) {
                    bc_collect(el.children.at(i), locals, used)
                }
            }
            JsNodeKind.JSXExpressionContainer => { bc_collect((node as *mut JsJSXExpressionContainer).expression, locals, used) }
            JsNodeKind.JSXAttribute => { bc_collect((node as *mut JsJSXAttribute).value, locals, used) }
            JsNodeKind.JSXSpreadAttribute => { bc_collect((node as *mut JsJSXSpreadAttribute).argument, locals, used) }
            JsNodeKind.JSXFragment => {
                const f = node as *mut JsJSXFragment
                for(var i : uint = 0; i < f.children.size(); i++) {
                    bc_collect(f.children.at(i), locals, used)
                }
            }
            JsNodeKind.For => {
                const s = node as *mut JsFor
                bc_collect(s.init, locals, used)
                bc_collect(s.condition, locals, used)
                bc_collect(s.update, locals, used)
                bc_collect(s.body, locals, used)
            }
            JsNodeKind.ForIn => {
                const s = node as *mut JsForIn
                bc_collect(s.left, locals, used)
                bc_collect(s.right, locals, used)
                bc_collect(s.body, locals, used)
            }
            JsNodeKind.ForOf => {
                const s = node as *mut JsForOf
                bc_collect(s.left, locals, used)
                bc_collect(s.right, locals, used)
                bc_collect(s.body, locals, used)
            }
            JsNodeKind.While => {
                const s = node as *mut JsWhile
                bc_collect(s.condition, locals, used)
                bc_collect(s.body, locals, used)
            }
            JsNodeKind.DoWhile => {
                const s = node as *mut JsDoWhile
                bc_collect(s.body, locals, used)
                bc_collect(s.condition, locals, used)
            }
            JsNodeKind.Switch => {
                const s = node as *mut JsSwitch
                bc_collect(s.discriminant, locals, used)
                for(var i : uint = 0; i < s.cases.size(); i++) {
                    const c = s.cases.get_ptr(i)
                    bc_collect(c.test, locals, used)
                    for(var j : uint = 0; j < c.body.size(); j++) {
                        bc_collect(c.body.at(j), locals, used)
                    }
                }
            }
            JsNodeKind.TryCatch => {
                const s = node as *mut JsTryCatch
                bc_list_push(locals, &s.catchParam)
                bc_collect(s.tryBlock, locals, used)
                bc_collect(s.catchBlock, locals, used)
                bc_collect(s.finallyBlock, locals, used)
            }
            JsNodeKind.Throw => { bc_collect((node as *mut JsThrow).argument, locals, used) }
            JsNodeKind.Yield => { bc_collect((node as *mut JsYield).argument, locals, used) }
            JsNodeKind.Export => { bc_collect((node as *mut JsExport).declaration, locals, used) }
            default => { }
        }

        }
}

func bc_report_missing(compName : &std::string_view, name : &std::string_view,
                       diag : *mut ASTDiagnoser, loc : ubigint) {
        unsafe {
        var msg = std::string("WEB-BODY-008: component '")
        msg.append_view(compName)
        msg.append_view("' uses '")
        msg.append_view(name)
        msg.append_view("', but no indexed module provides it; import a provider (e.g. dom)")
        diag.error(msg.to_view(), loc)

        }
}

func bc_report_conflict(compName : &std::string_view, name : &std::string_view,
                        conflict : *mut AmbientConflict, diag : *mut ASTDiagnoser, loc : ubigint) {
        unsafe {
        var msg = std::string("component '")
        msg.append_view(compName)
        msg.append_view("' resolves '")
        msg.append_view(name)
        msg.append_view("', but multiple indexed modules provide it: '")
        const first = conflict.first_provider.to_view()
        const second = conflict.second_provider.to_view()
        msg.append_view(&first)
        msg.append_view("' and '")
        msg.append_view(&second)
        msg.append_view("'; import only one provider or rename the declaration")
        diag.error(msg.to_view(), loc)

        }
}

// Entry point. `propsName`/`propsStruct` are accepted now so the Phase 3 member
// check can use them without changing the call site; Phase 2 does not inspect
// props. The check runs only on the client/JS job and reports each missing host
// name once per component.
public func body_check(body : *mut void, compName : &std::string_view, propsName : &std::string_view,
                       propsStruct : *mut StructDefinition, diag : *mut ASTDiagnoser, loc : ubigint) {
        unsafe {
        if(!universal_is_client_job) return
        if(body == null || diag == null) return

        var locals = std::vector<std::string_view>()
        var used = std::vector<std::string_view>()
        bc_collect(body as *mut JsNode, &mut locals, &mut used)

        for(var i : uint = 0; i < used.size(); i++) {
            const name = used.at(i)
            if(bc_list_has(&locals, &name)) { continue }

            if(universal_index_lookup(&name) == null) {
                bc_report_missing(compName, &name, diag, loc)
                continue
            }

            const conflict = universal_index_conflict(&name)
            if(conflict != null) {
                bc_report_conflict(compName, &name, conflict, diag, loc)
            }
        }

        }
}
