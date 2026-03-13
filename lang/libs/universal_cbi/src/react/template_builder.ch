func find_returned_jsx(block : *mut JsBlock) : *mut JsNode {
    for(var i : uint = 0; i < block.statements.size(); i++) {
        const stmt = block.statements.get(i);
        if(stmt == null || stmt.kind != JsNodeKind.Return) continue;
        const ret = stmt as *mut JsReturn;
        if(ret.value == null) continue;
        if(ret.value.kind == JsNodeKind.JSXElement || ret.value.kind == JsNodeKind.JSXFragment) {
            return ret.value;
        }
        if(ret.value.kind == JsNodeKind.Identifier) {
            const id = ret.value as *mut JsIdentifier;
            for(var j : uint = 0; j < i; j++) {
                const prev = block.statements.get(j);
                if(prev != null && prev.kind == JsNodeKind.VarDecl) {
                    const decl = prev as *mut JsVarDecl;
                    if(decl.name.equals(id.value) && decl.value != null && (decl.value.kind == JsNodeKind.JSXElement || decl.value.kind == JsNodeKind.JSXFragment)) {
                        return decl.value;
                    }
                }
            }
        }
    }
    return null;
}

func collect_states(
    builder : *mut ASTBuilder,
    block : *mut JsBlock,
    outStates : &mut std::vector<UniversalStateDecl>,
    support : *mut SymResSupport
) {
    for(var i : uint = 0; i < block.statements.size(); i++) {
        const stmt = block.statements.get(i);
        if(stmt == null || stmt.kind != JsNodeKind.VarDecl) continue;
        const decl = stmt as *mut JsVarDecl;
        if(!decl.keyword.equals(view("state")) || decl.name.empty()) continue;
        var initExpr = view("undefined");
        var initText = view("");
        if(decl.value != null) {
            initExpr = js_node_to_source(builder, decl.value, outStates, support, null);
            if(decl.value.kind == JsNodeKind.Literal) {
                const lit = decl.value as *mut JsLiteral;
                initText = builder.allocate_view(strip_js_string_quotes(lit.value));
            } else {
                initText = initExpr;
            }
        }
        outStates.push(UniversalStateDecl {
            name : decl.name,
            initExpr : initExpr,
            initText : initText
        });
    }
}

func compute_universal_template(builder : *mut ASTBuilder, comp : *mut JsComponentDecl, tmpSupport : *mut SymResSupport) {
    if(comp == null || comp.body == null || comp.body.kind != JsNodeKind.Block) return;
    const block = comp.body as *mut JsBlock;

    var states = std::vector<UniversalStateDecl>();
    collect_states(builder, block, states, tmpSupport);

    const returned = find_returned_jsx(block);
    if(returned == null) return;

    var textBindings = std::vector<UniversalTextBinding>();
    var eventBindings = std::vector<UniversalEventBinding>();
    var propBindings = std::vector<UniversalPropTextBinding>();
    var attrBindings = std::vector<UniversalAttrBinding>();
    var nestedBindings = std::vector<UniversalNestedBinding>();
    var tokens = std::vector<TemplateToken>();

    var converter = JsConverter {
        builder : builder,
        support : tmpSupport,
        vec : null,
        tokens : &mut tokens,
        parent : null,
        str : std::string(),
        jsx_parent : view(""),
        t_counter : 0,
        state_vars : std::vector<std::string_view>()
    }

    if(!render_universal_jsx(builder, returned, view("[]"), states, textBindings, eventBindings, propBindings, attrBindings, nestedBindings, tokens, comp.signature.propsName, converter)) {
        return;
    }
    converter.flush_text(tokens);

    var init = std::string();
    for(var i : uint = 0; i < states.size(); i++) {
        const st = states.get(i);
        init.append_view("const ");
        init.append_view(st.name);
        init.append_view(" = $_us(");
        init.append_view(st.initExpr);
        init.append_view(");");
    }
    for(var i : uint = 0; i < textBindings.size(); i++) {
        const b = textBindings.get(i);
        init.append_view(b.stateName);
        init.append_view(".subscribe(v=>$_ut(root,");
        init.append_view(b.path);
        init.append_view(").textContent=v);");
    }
    for(var i : uint = 0; i < propBindings.size(); i++) {
        const b = propBindings.get(i);
        init.append_view("{const v=props.");
        init.append_view(b.propPath);
        init.append_view(";$_ut(root,");
        init.append_view(b.path);
        init.append_view(").textContent=(v==null?\"\":(\"\"+v));}");
    }
    for(var i : uint = 0; i < attrBindings.size(); i++) {
        const b = attrBindings.get(i);
        const attr = b.attr;
        if(attr == null) continue;
        const sep = if(attr.name.equals(view("style"))) ';' else ' ';
        init.append_view("{const el=$_ut(root,");
        init.append_view(b.path);
        init.append_view(");let v=\"\";let h=false;const a=(x)=>{if(x==null||x===false)return;const s=\"\"+x;if(s.length===0)return;if(h)v+=\"");
        init.append(sep);
        init.append_view("\";v+=s;h=true;};");
        for(var j : uint = 0; j < attr.segments.size(); j++) {
            const seg = attr.segments.get(j);
            switch(seg.kind) {
                MergedAttrSegmentKind.Text => {
                    if(seg.value.empty()) break;
                    init.append_view("a('");
                    append_escaped_single_quoted(init, seg.value);
                    init.append_view("');");
                }
                MergedAttrSegmentKind.PropAccess => {
                    if(seg.value.empty()) break;
                    init.append_view("a(props.");
                    init.append_view(seg.value);
                    init.append_view(");");
                }
                MergedAttrSegmentKind.SpreadAttr => {
                    if(attr.name.equals(view("class"))) {
                        init.append_view("a(props.className);a(props.class);");
                    } else if(attr.name.equals(view("style"))) {
                        init.append_view("a(props.style);");
                    }
                }
                default => {}
            }
        }
        init.append_view("if(h) el.setAttribute('");
        init.append_view(attr.name);
        init.append_view("', v);}");
    }
    for(var i : uint = 0; i < nestedBindings.size(); i++) {
        const nb = nestedBindings.get(i);
        init.append_view("{const c=(window.$_u&&window.$_u['");
        init.append_view(nb.componentName);
        init.append_view("'])||window['");
        init.append_view(nb.componentName);
        init.append_view("'];if(c&&c.__hydrate){c.__hydrate($_ut(root,");
        init.append_view(nb.path);
        init.append_view("),");
        init.append_view(nb.propsExpr);
        init.append_view(");}}");
    }
    for(var i : uint = 0; i < eventBindings.size(); i++) {
        const e = eventBindings.get(i);
        init.append_view("$_ut(root,");
        init.append_view(e.path);
        init.append_view(").addEventListener('");
        init.append_view(e.eventName);
        init.append_view("',");
        init.append_view(e.handlerExpr);
        init.append_view(");");
    }

    comp.signature.universalTemplate = tokens;
    comp.signature.universalInit = builder.allocate_view(init.to_view());
}
