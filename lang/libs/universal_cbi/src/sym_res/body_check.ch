// Component body check (Phase 2/3): the client/JS job resolves the component's
// `props` field reads and browser host names against the per-job index built
// from marked provider modules.
//
// Rules implemented here:
//   * WEB-BODY-008 — a closed host name (`document`, `window`, ...) has no
//     indexed provider. This is the Phase 2 rule and is unchanged.
//   * WEB-BODY-001 — `props.X` where `X` is not a declared field of the
//     component's props struct.
//   * WEB-BODY-002 — a *direct* member of an ambient root (`document.X`,
//     `window.X`, ...) that the indexed type does not declare.
//   * WEB-BODY-006 — a hook called conditionally or inside a handler/effect
//     lambda, so the call order can change between renders.
//   * WEB-BODY-005 — counted, never an error in this phase: a member access
//     through a deeper chain or an unknown local/expression. `[body]
//     component=... untyped=N` is printed under CHEM_DEBUG_INDEX/CHEM_DEBUG_BODY.
//
// Only a direct member of an ambient root is an error, which keeps common
// corpus code (`document.body.classList`, `document.querySelector(...).x`)
// from being rejected before the Element surface exists. The server job
// generates SSR and must not run this check, so the entry point returns
// immediately when `universal_job_begin` recorded a non-client job.

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

// The ambient roots whose direct members are checked. `fetch` is in the closed
// host list but takes parameters, so it has no member surface.
func bc_is_ambient_root(name : &std::string_view) : bool {
        unsafe {
        if(name.equals(std::string_view("document"))) return true
        if(name.equals(std::string_view("window"))) return true
        if(name.equals(std::string_view("localStorage"))) return true
        if(name.equals(std::string_view("navigator"))) return true
        return false

        }
}

// The runtime's hooks. A hook call that is not unconditional is WEB-BODY-006.
func bc_is_hook_name(name : &std::string_view) : bool {
        unsafe {
        if(name.equals(std::string_view("useEffect"))) return true
        if(name.equals(std::string_view("useLayoutEffect"))) return true
        if(name.equals(std::string_view("useMemo"))) return true
        if(name.equals(std::string_view("useCallback"))) return true
        if(name.equals(std::string_view("useRef"))) return true
        if(name.equals(std::string_view("useState"))) return true
        if(name.equals(std::string_view("useReducer"))) return true
        if(name.equals(std::string_view("useContext"))) return true
        if(name.equals(std::string_view("useUnmount"))) return true
        if(name.equals(std::string_view("useErrorBoundary"))) return true
        return false

        }
}

// Framework internals are never part of the public Window surface; a member
// access on them is a counted edge, not an error.
func bc_is_framework_member(name : &std::string_view) : bool {
        unsafe {
        if(name.equals(std::string_view("__reportError"))) return true
        const is_framework = name.size() > 6 && name.at(0) == '$' && name.at(1) == '_' && name.at(2) == '_' && name.at(3) == 'u' && name.at(4) == 'n' && name.at(5) == 'i' && name.at(6) == '_'
        if(is_framework) {
            return true
        }
        return false

        }
}

func bc_debug_enabled() : bool {
        unsafe {
        if(getenv("CHEM_DEBUG_INDEX\0" as *char) != null) return true
        if(getenv("CHEM_DEBUG_BODY\0" as *char) != null) return true
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

func bc_ptr_list_has(list : *mut std::vector<std::string_view>, name : &std::string_view) : bool {
        unsafe {
        for(var i : uint = 0; i < list.size(); i++) {
            if(list.at(i).equals(name)) { return true }
        }
        return false

        }
}

func bc_ptr_list_push(list : *mut std::vector<std::string_view>, name : &std::string_view) {
        unsafe {
        if(name.size() == 0) { return }
        if(!bc_ptr_list_has(list, name)) {
            list.push(*name)
        }

        }
}

func bc_key_seen(list : *mut std::vector<std::string>, key : &std::string) : bool {
        unsafe {
        for(var i : uint = 0; i < list.size(); i++) {
            const existing = list.get_ptr(i)
            if(existing.equals(key)) { return true }
        }
        return false

        }
}

// ── member lookup ──────────────────────────────────────────────────────────

// A struct field declared without an explicit specifier is `private` in the
// parser while still being initializable from a call site (`props.title`), so
// only `internal` is hidden. This is what keeps `Document.handle` out of the
// browser surface.
func bc_is_exposed(node : *mut ASTNode) : bool {
        unsafe {
        const spec = node.getAccessSpecifier()
        return spec != AccessSpecifier.Internal

        }
}

func bc_find_field(container : *mut StructDefinition, name : &std::string_view,
                   outType : *mut *mut BaseType) : bool {
        unsafe {
        if(container == null) return false
        const members = container.getMembers()
        if(members == null) return false
        for(var i : uint = 0; i < members.size(); i++) {
            const member = members.get(i)
            if(member == null) continue
            if(!bc_is_exposed(member as *mut ASTNode)) continue
            if(member.getName().equals(name)) {
                *outType = member.getType()
                return true
            }
        }
        return false

        }
}

// A member exists when it is a field or a method. Generated `init`/`drop`
// functions and non-public members are not part of the surface (`Document`'s
// `internal var handle`, for example).
func bc_find_member(container : *mut StructDefinition, name : &std::string_view,
                    outType : *mut *mut BaseType) : bool {
        unsafe {
        if(bc_find_field(container, name, outType)) return true
        if(container == null) return false
        const functions = container.getFunctions()
        if(functions == null) return false
        for(var i : uint = 0; i < functions.size(); i++) {
            const node = functions.get(i)
            if(node == null || node.getKind() != ASTNodeKind.FunctionDecl) continue
            if(!bc_is_exposed(node)) continue
            const fn = node as *mut FunctionDeclaration
            const attrs = fn.getAttributes()
            if(attrs.is_generated) continue
            const fnName = fn.getName()
            if(fnName.equals(std::string_view("init")) || fnName.equals(std::string_view("drop"))) continue
            if(fnName.equals(name)) {
                *outType = fn.getReturnType()
                return true
            }
        }
        return false

        }
}

// A named return type is a LinkedType whose linked node is the StructDecl. A
// type that is not linked to a struct is unknown: the caller counts the edge
// and never reports a missing member for it.
func bc_struct_of_type(type : *mut BaseType) : *mut StructDefinition {
        unsafe {
        if(type == null) return null
        if(type.getKind() != BaseTypeKind.Linked) return null
        const linked = type as *mut LinkedType
        const node = linked.getLinkedNode()
        if(node == null) return null
        if(node.getKind() != ASTNodeKind.StructDecl) return null
        return node as *mut StructDefinition

        }
}

// One edit (substitution, insertion, deletion or adjacent transposition) —
// enough for `titel` -> `title` and the common typo class.
func bc_one_deleted(longer : &std::string_view, shorter : &std::string_view) : bool {
        unsafe {
        if(longer.size() != shorter.size() + 1) return false
        var i : uint = 0
        var j : uint = 0
        var skipped : bool = false
        while(i < longer.size() && j < shorter.size()) {
            if(longer.at(i) == shorter.at(j)) {
                i = i + 1
                j = j + 1
                continue
            }
            if(skipped) return false
            skipped = true
            i = i + 1
        }
        return true

        }
}

func bc_is_near_name(a : &std::string_view, b : &std::string_view) : bool {
        unsafe {
        if(a.equals(b)) return true
        if(a.size() == b.size()) {
            var first : int = -1
            var second : int = -1
            var diffs : uint = 0
            for(var i : uint = 0; i < a.size(); i++) {
                if(a.at(i) != b.at(i)) {
                    if(diffs == 0) { first = i as int }
                    else if(diffs == 1) { second = i as int }
                    diffs = diffs + 1
                }
            }
            if(diffs == 1) return true
            if(diffs == 2 && second == first + 1) {
                const fi = first as uint
                const si = second as uint
                if(a.at(fi) == b.at(si) && a.at(si) == b.at(fi)) return true
            }
            return false
        }
        if(a.size() + 1 == b.size()) return bc_one_deleted(b, a)
        if(b.size() + 1 == a.size()) return bc_one_deleted(a, b)
        return false

        }
}

func bc_suggest_member(container : *mut StructDefinition, name : &std::string_view) : std::string_view {
        unsafe {
        if(container == null) return std::string_view()
        const members = container.getMembers()
        if(members != null) {
            for(var i : uint = 0; i < members.size(); i++) {
                const member = members.get(i)
                if(member == null || !bc_is_exposed(member as *mut ASTNode)) continue
                const candidate = member.getName()
                if(bc_is_near_name(name, &candidate)) return candidate
            }
        }
        const functions = container.getFunctions()
        if(functions != null) {
            for(var i : uint = 0; i < functions.size(); i++) {
                const node = functions.get(i)
                if(node == null || node.getKind() != ASTNodeKind.FunctionDecl) continue
                if(!bc_is_exposed(node)) continue
                const fn = node as *mut FunctionDeclaration
                const attrs = fn.getAttributes()
                if(attrs.is_generated) continue
                const candidate = fn.getName()
                if(candidate.equals(std::string_view("init")) || candidate.equals(std::string_view("drop"))) continue
                if(bc_is_near_name(name, &candidate)) return candidate
            }
        }
        return std::string_view()

        }
}

func bc_append_members(msg : &mut std::string, container : *mut StructDefinition, limit : uint) {
        unsafe {
        if(container == null) return
        var shown : uint = 0
        const members = container.getMembers()
        if(members != null) {
            for(var i : uint = 0; i < members.size() && shown < limit; i++) {
                const member = members.get(i)
                if(member == null || !bc_is_exposed(member as *mut ASTNode)) continue
                if(shown > 0) { msg.append_view(", ") }
                const memberName = member.getName()
                msg.append_view(&memberName)
                shown = shown + 1
            }
        }
        const functions = container.getFunctions()
        if(functions != null) {
            for(var i : uint = 0; i < functions.size() && shown < limit; i++) {
                const node = functions.get(i)
                if(node == null || node.getKind() != ASTNodeKind.FunctionDecl) continue
                if(!bc_is_exposed(node)) continue
                const fn = node as *mut FunctionDeclaration
                const attrs = fn.getAttributes()
                if(attrs.is_generated) continue
                const fnName = fn.getName()
                if(fnName.equals(std::string_view("init")) || fnName.equals(std::string_view("drop"))) continue
                if(shown > 0) { msg.append_view(", ") }
                msg.append_view(&fnName)
                shown = shown + 1
            }
        }

        }
}

// ── diagnostics ────────────────────────────────────────────────────────────

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

struct BcCtx {
    var comp : std::string_view
    var propsName : std::string_view
    var propsStruct : *mut StructDefinition
    var locals : *mut std::vector<std::string_view>
    var diag : *mut ASTDiagnoser
    var loc : ubigint
    var reportedProps : *mut std::vector<std::string_view>
    var reportedMembers : *mut std::vector<std::string>
    var reportedHooks : *mut std::vector<std::string_view>
    var untyped : uint
}

func bc_report_unknown_prop(ctx : &mut BcCtx, prop : &std::string_view) {
        unsafe {
        if(bc_ptr_list_has(ctx.reportedProps, prop)) return
        bc_ptr_list_push(ctx.reportedProps, prop)

        var msg = std::string("WEB-BODY-001: component '")
        msg.append_view(&ctx.comp)
        msg.append_view("' reads '")
        msg.append_view(&ctx.propsName)
        msg.append_view(".")
        msg.append_view(prop)
        msg.append_view("', but '")
        msg.append_view(prop)
        msg.append_view("' is not a declared prop")
        if(ctx.propsStruct != null) {
            const members = ctx.propsStruct.getMembers()
            if(members != null && members.size() > 0) {
                msg.append_view("; declared props: ")
                var shown : uint = 0
                for(var i : uint = 0; i < members.size(); i++) {
                    const member = members.get(i)
                    if(member == null || !bc_is_exposed(member as *mut ASTNode)) continue
                    if(shown > 0) { msg.append_view(", ") }
                    const memberName = member.getName()
                    msg.append_view(&memberName)
                    shown = shown + 1
                }
            } else {
                msg.append_view("; declared props: (none)")
            }
        } else {
            msg.append_view("; the component has no typed props struct")
        }
        ctx.diag.error(msg.to_view(), ctx.loc)

        }
}

func bc_report_unknown_member(ctx : &mut BcCtx, rootName : &std::string_view,
                              memberName : &std::string_view, container : *mut StructDefinition) {
        unsafe {
        var key = std::string()
        key.append_view(rootName)
        key.append_view(".")
        key.append_view(memberName)
        if(bc_key_seen(ctx.reportedMembers, &key)) return
        ctx.reportedMembers.push(key)

        var msg = std::string("WEB-BODY-002: component '")
        msg.append_view(&ctx.comp)
        msg.append_view("' reads '")
        msg.append_view(rootName)
        msg.append_view(".")
        msg.append_view(memberName)
        msg.append_view("', but '")
        if(container != null) {
            const typeName = container.getName()
            msg.append_view(&typeName)
        } else {
            msg.append_view(rootName)
        }
        msg.append_view("' has no member '")
        msg.append_view(memberName)
        msg.append_view("'")

        const suggestion = bc_suggest_member(container, memberName)
        if(suggestion.size() > 0) {
            msg.append_view("; did you mean '")
            msg.append_view(&suggestion)
            msg.append_view("'?")
        } else {
            msg.append_view("; declared members: ")
            bc_append_members(&mut msg, container, 8)
        }
        ctx.diag.error(msg.to_view(), ctx.loc)

        }
}

func bc_report_hook(ctx : &mut BcCtx, hook : &std::string_view) {
        unsafe {
        if(bc_ptr_list_has(ctx.reportedHooks, hook)) return
        bc_ptr_list_push(ctx.reportedHooks, hook)

        var msg = std::string("WEB-BODY-006: component '")
        msg.append_view(&ctx.comp)
        msg.append_view("' calls hook '")
        msg.append_view(hook)
        msg.append_view("' conditionally or inside a handler/effect lambda; hooks must run unconditionally at the top level of the component")
        ctx.diag.error(msg.to_view(), ctx.loc)

        }
}

// ── local collection (shadowing + the Phase 2 host-name pass) ──────────────

func bc_collect_pattern(node : *mut JsNode, locals : &mut std::vector<std::string_view>) {
        unsafe {
        if(node == null) return
        if(node.kind == JsNodeKind.Identifier) {
            bc_list_push(locals, &(node as *mut JsIdentifier).value)
        } else if(node.kind == JsNodeKind.ArrayDestructuring || node.kind == JsNodeKind.ArrayLiteral) {
            const arr = node as *mut JsArrayLiteral
            for(var i : uint = 0; i < arr.elements.size(); i++) {
                bc_collect_pattern(arr.elements.at(i), locals)
            }
        }

        }
}

// Collects names in local position (var/function/parameter/catch bindings) so a
// host spelling or `props` shadowed by a local is never reported, and collects
// host-name uses so a single pass can report them afterwards.
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
                if(v.name.size() > 0) {
                    bc_list_push(locals, &v.name)
                }
                bc_collect_pattern(v.pattern, locals)
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
                bc_list_push(locals, &c.name)
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
            JsNodeKind.ArrayDestructuring => {
                bc_collect_pattern(node, locals)
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

// ── member / hook walk ─────────────────────────────────────────────────────

func bc_examine(ctx : &mut BcCtx, node : *mut JsNode, inLambda : bool, conditional : bool) {
        unsafe {
        if(node == null) return
        switch(node.kind) {
            JsNodeKind.Identifier => {
                const id = node as *mut JsIdentifier
                // A bare `props` read (spread, destructuring value) has no
                // modelled field set; count it, never error.
                if(id.value.equals(&ctx.propsName) && !bc_ptr_list_has(ctx.locals, &id.value)) {
                    ctx.untyped = ctx.untyped + 1
                }
            }
            JsNodeKind.MemberAccess => {
                const m = node as *mut JsMemberAccess
                bc_examine_member(ctx, m)
                // Walk into a deeper base expression, but not into the plain
                // identifier we already analysed as this access's base.
                if(m.object != null && m.object.kind != JsNodeKind.Identifier) {
                    bc_examine(ctx, m.object, inLambda, conditional)
                }
            }
            JsNodeKind.FunctionCall => {
                const c = node as *mut JsFunctionCall
                if(c.callee != null && c.callee.kind == JsNodeKind.Identifier) {
                    const calleeName = (c.callee as *mut JsIdentifier).value
                    if(bc_is_hook_name(&calleeName)) {
                        if(inLambda || conditional) {
                            bc_report_hook(ctx, &calleeName)
                        }
                    } else if(!bc_is_host_name(&calleeName) && !calleeName.equals(&ctx.propsName)) {
                        // A call to a local/unknown function: a typed edge we do not
                        // model in this phase, so it is counted (never an error).
                        ctx.untyped = ctx.untyped + 1
                    }
                }
                if(c.callee != null && c.callee.kind != JsNodeKind.Identifier) {
                    bc_examine(ctx, c.callee, inLambda, conditional)
                }
                for(var i : uint = 0; i < c.args.size(); i++) {
                    bc_examine(ctx, c.args.at(i), inLambda, conditional)
                }
            }
            JsNodeKind.ExpressionStatement => { bc_examine(ctx, (node as *mut JsExpressionStatement).expression, inLambda, conditional) }
            JsNodeKind.Block => {
                const b = node as *mut JsBlock
                for(var i : uint = 0; i < b.statements.size(); i++) {
                    bc_examine(ctx, b.statements.at(i), inLambda, conditional)
                }
            }
            JsNodeKind.If => {
                const s = node as *mut JsIf
                bc_examine(ctx, s.condition, inLambda, conditional)
                bc_examine(ctx, s.thenBlock, inLambda, true)
                bc_examine(ctx, s.elseBlock, inLambda, true)
            }
            JsNodeKind.Return => { bc_examine(ctx, (node as *mut JsReturn).value, inLambda, conditional) }
            JsNodeKind.VarDecl => {
                const v = node as *mut JsVarDecl
                bc_examine(ctx, v.value, inLambda, conditional)
                bc_examine(ctx, v.next as *mut JsNode, inLambda, conditional)
            }
            JsNodeKind.ArrowFunction => {
                const f = node as *mut JsArrowFunction
                for(var i : uint = 0; i < f.params.size(); i++) {
                    bc_examine(ctx, f.params.at(i).default_value, inLambda, conditional)
                }
                bc_examine(ctx, f.body, true, conditional)
            }
            JsNodeKind.FunctionDecl => {
                const f = node as *mut JsFunctionDecl
                for(var i : uint = 0; i < f.params.size(); i++) {
                    bc_examine(ctx, f.params.at(i).default_value, inLambda, conditional)
                }
                bc_examine(ctx, f.body, true, conditional)
            }
            JsNodeKind.ClassDecl => {
                const c = node as *mut JsClassDecl
                for(var i : uint = 0; i < c.methods.size(); i++) {
                    const m = c.methods.get_ptr(i)
                    bc_examine(ctx, m.body, true, conditional)
                }
            }
            JsNodeKind.BinaryOp => {
                const s = node as *mut JsBinaryOp
                bc_examine(ctx, s.left, inLambda, conditional)
                bc_examine(ctx, s.right, inLambda, conditional)
            }
            JsNodeKind.Ternary => {
                const s = node as *mut JsTernary
                bc_examine(ctx, s.condition, inLambda, conditional)
                bc_examine(ctx, s.consequent, inLambda, true)
                bc_examine(ctx, s.alternate, inLambda, true)
            }
            JsNodeKind.UnaryOp => { bc_examine(ctx, (node as *mut JsUnaryOp).operand, inLambda, conditional) }
            JsNodeKind.Paren => { bc_examine(ctx, (node as *mut JsParen).expression, inLambda, conditional) }
            JsNodeKind.Spread => { bc_examine(ctx, (node as *mut JsSpread).argument, inLambda, conditional) }
            JsNodeKind.ArrayLiteral => {
                const a = node as *mut JsArrayLiteral
                for(var i : uint = 0; i < a.elements.size(); i++) {
                    bc_examine(ctx, a.elements.at(i), inLambda, conditional)
                }
            }
            JsNodeKind.ArrayDestructuring => {
                const a = node as *mut JsArrayLiteral
                for(var i : uint = 0; i < a.elements.size(); i++) {
                    bc_examine(ctx, a.elements.at(i), inLambda, conditional)
                }
            }
            JsNodeKind.ObjectLiteral => {
                const o = node as *mut JsObjectLiteral
                for(var i : uint = 0; i < o.properties.size(); i++) {
                    bc_examine(ctx, o.properties.at(i).value, inLambda, conditional)
                }
            }
            JsNodeKind.IndexAccess => {
                const s = node as *mut JsIndexAccess
                bc_examine(ctx, s.object, inLambda, conditional)
                bc_examine(ctx, s.index, inLambda, conditional)
            }
            JsNodeKind.JSXElement => {
                const el = node as *mut JsJSXElement
                for(var i : uint = 0; i < el.opening.attributes.size(); i++) {
                    bc_examine(ctx, el.opening.attributes.at(i), inLambda, conditional)
                }
                for(var i : uint = 0; i < el.children.size(); i++) {
                    bc_examine(ctx, el.children.at(i), inLambda, conditional)
                }
            }
            JsNodeKind.JSXExpressionContainer => { bc_examine(ctx, (node as *mut JsJSXExpressionContainer).expression, inLambda, conditional) }
            JsNodeKind.JSXAttribute => { bc_examine(ctx, (node as *mut JsJSXAttribute).value, inLambda, conditional) }
            JsNodeKind.JSXSpreadAttribute => { bc_examine(ctx, (node as *mut JsJSXSpreadAttribute).argument, inLambda, conditional) }
            JsNodeKind.JSXFragment => {
                const f = node as *mut JsJSXFragment
                for(var i : uint = 0; i < f.children.size(); i++) {
                    bc_examine(ctx, f.children.at(i), inLambda, conditional)
                }
            }
            JsNodeKind.For => {
                const s = node as *mut JsFor
                bc_examine(ctx, s.init, inLambda, true)
                bc_examine(ctx, s.condition, inLambda, true)
                bc_examine(ctx, s.update, inLambda, true)
                bc_examine(ctx, s.body, inLambda, true)
            }
            JsNodeKind.ForIn => {
                const s = node as *mut JsForIn
                bc_examine(ctx, s.left, inLambda, true)
                bc_examine(ctx, s.right, inLambda, true)
                bc_examine(ctx, s.body, inLambda, true)
            }
            JsNodeKind.ForOf => {
                const s = node as *mut JsForOf
                bc_examine(ctx, s.left, inLambda, true)
                bc_examine(ctx, s.right, inLambda, true)
                bc_examine(ctx, s.body, inLambda, true)
            }
            JsNodeKind.While => {
                const s = node as *mut JsWhile
                bc_examine(ctx, s.condition, inLambda, true)
                bc_examine(ctx, s.body, inLambda, true)
            }
            JsNodeKind.DoWhile => {
                const s = node as *mut JsDoWhile
                bc_examine(ctx, s.body, inLambda, true)
                bc_examine(ctx, s.condition, inLambda, true)
            }
            JsNodeKind.Switch => {
                const s = node as *mut JsSwitch
                bc_examine(ctx, s.discriminant, inLambda, conditional)
                for(var i : uint = 0; i < s.cases.size(); i++) {
                    const c = s.cases.get_ptr(i)
                    bc_examine(ctx, c.test, inLambda, true)
                    for(var j : uint = 0; j < c.body.size(); j++) {
                        bc_examine(ctx, c.body.at(j), inLambda, true)
                    }
                }
            }
            JsNodeKind.TryCatch => {
                const s = node as *mut JsTryCatch
                bc_examine(ctx, s.tryBlock, inLambda, conditional)
                bc_examine(ctx, s.catchBlock, inLambda, conditional)
                bc_examine(ctx, s.finallyBlock, inLambda, conditional)
            }
            JsNodeKind.Throw => { bc_examine(ctx, (node as *mut JsThrow).argument, inLambda, conditional) }
            JsNodeKind.Yield => { bc_examine(ctx, (node as *mut JsYield).argument, inLambda, conditional) }
            JsNodeKind.Export => { bc_examine(ctx, (node as *mut JsExport).declaration, inLambda, conditional) }
            default => { }
        }

        }
}

func bc_examine_member(ctx : &mut BcCtx, m : *mut JsMemberAccess) {
        unsafe {
        if(m == null) return
        if(m.object == null) {
            ctx.untyped = ctx.untyped + 1
            return
        }
        if(m.object.kind != JsNodeKind.Identifier) {
            // `document.body.classList`, `document.querySelector(...).textContent`:
            // a deeper chain is measured, never rejected in this phase.
            ctx.untyped = ctx.untyped + 1
            return
        }

        const id = m.object as *mut JsIdentifier

        // Component props. A local with the props name shadows the parameter,
        // so there is nothing to check against the props struct.
        if(id.value.equals(&ctx.propsName)) {
            if(bc_ptr_list_has(ctx.locals, &id.value)) {
                ctx.untyped = ctx.untyped + 1
                return
            }
            if(ctx.propsStruct == null) {
                ctx.untyped = ctx.untyped + 1
                return
            }
            if(is_implicit_jsx_prop_name(&m.property)) { return }
            var fieldType : *mut BaseType = null
            if(bc_find_field(ctx.propsStruct, &m.property, &raw mut fieldType)) { return }
            bc_report_unknown_prop(ctx, &m.property)
            return
        }

        // Browser roots. Only the direct member is checked; the provider entry
        // may have been indexed without a return type (parameterized function),
        // in which case the access is counted and not errors.
        if(bc_is_ambient_root(&id.value)) {
            if(bc_ptr_list_has(ctx.locals, &id.value)) {
                ctx.untyped = ctx.untyped + 1
                return
            }
            if(bc_is_framework_member(&m.property)) {
                ctx.untyped = ctx.untyped + 1
                return
            }
            const entry = universal_index_lookup(&id.value)
            if(entry == null) {
                // 008 is reported by the host-name pass; do not double-report.
                return
            }
            if(entry.type == null) {
                ctx.untyped = ctx.untyped + 1
                return
            }
            const container = bc_struct_of_type(entry.type)
            if(container == null) {
                ctx.untyped = ctx.untyped + 1
                return
            }
            var memberType : *mut BaseType = null
            if(bc_find_member(container, &m.property, &raw mut memberType)) { return }
            bc_report_unknown_member(ctx, &id.value, &m.property, container)
            return
        }

        // An unknown local, built-in (`Math`, `console`) or expression base.
        ctx.untyped = ctx.untyped + 1

        }
}

// Entry point. The check runs on the client/JS job, and on any other job whose
// per-job index has at least one ambient provider (a shared component compiled
// by the server pass with `import dom` on both sides). A server job with no
// provider has nothing to resolve host names against, so it skips instead of
// producing a false 008. Reports each missing host name (008), prop (001),
// direct root member (002) and misplaced hook (006) once per component. Untyped
// edges (005) are only counted.
public func body_check(body : *mut void, compName : &std::string_view, propsName : &std::string_view,
                       propsStruct : *mut StructDefinition, diag : *mut ASTDiagnoser, loc : ubigint) {
        unsafe {
        if(!universal_is_client_job && universal_index.ambient.size() == 0) return
        if(body == null || diag == null) return

        var locals = std::vector<std::string_view>()
        var used = std::vector<std::string_view>()
        bc_collect(body as *mut JsNode, &mut locals, &mut used)

        // Phase 2: every closed host name must have an indexed provider. A name
        // shadowed by a local is not a global use.
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

        var reportedProps = std::vector<std::string_view>()
        var reportedMembers = std::vector<std::string>()
        var reportedHooks = std::vector<std::string_view>()
        var ctx = BcCtx {
            comp : *compName,
            propsName : *propsName,
            propsStruct : propsStruct,
            locals : &raw mut locals,
            diag : diag,
            loc : loc,
            reportedProps : &raw mut reportedProps,
            reportedMembers : &raw mut reportedMembers,
            reportedHooks : &raw mut reportedHooks,
            untyped : 0
        }

        bc_examine(&mut ctx, body as *mut JsNode, false, false)

        if(bc_debug_enabled()) {
            printf("[body] component=%.*s untyped=%d\n",
                compName.size() as int, compName.data(), ctx.untyped as int)
        }

        }
}
