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
    var nestedBindings = std::vector<UniversalNestedBinding>();

    var converter = JsConverter {
        builder : builder,
        support : tmpSupport,
        vec : comp.signature.functionNode.add_body(),
        parent : comp.signature.functionNode ,
        str : std::string(),
        jsx_parent : view(""),
        t_counter : 0,
        state_vars : std::vector<std::string_view>(),
        target : BufferType.HTML,
        current_func : comp.signature.functionNode
    }

    var nodeCount = 0u;
    if(!render_universal_jsx(builder, returned, view("[]"), states, textBindings, eventBindings, propBindings, nestedBindings, comp.signature.propsName, converter, nodeCount)) {
        return;
    }
    comp.signature.rootNodeCount = nodeCount;

    comp.templateHtml = builder.allocate_view(converter.str.to_view());

    var init = std::string();
    init.append_view("(root,props,offset=0)=>{");
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
        init.append_view(",offset).textContent=v);");
    }
    for(var i : uint = 0; i < propBindings.size(); i++) {
        const b = propBindings.get(i);
        init.append_view("{const v=props.");
        init.append_view(b.propPath);
        init.append_view(";$_ut(root,");
        init.append_view(b.path);
        init.append_view(",offset).textContent=(v==null?\"\":(\"\"+v));}");
    }
    for(var i : uint = 0; i < nestedBindings.size(); i++) {
        const nb = nestedBindings.get(i);
        init.append_view("{const c=(window.$_u&&window.$_u['");
        init.append_view(nb.componentName);
        init.append_view("'])||window['");
        init.append_view(nb.componentName);
        init.append_view("'];if(c&&c.__hydrate){c.__hydrate($_ut(root,");
        init.append_view(nb.path);
        init.append_view(",offset),");
        init.append_view(nb.propsExpr);
        init.append_view(",");
        if(nb.path.equals(view("[]"))) {
            init.append_view("offset+");
        }
        var offsetStr = std::string();
        offsetStr.append_uinteger(nb.offset as u64);
        init.append_view(offsetStr.to_view());
        init.append_view(");}}");
    }
    for(var i : uint = 0; i < eventBindings.size(); i++) {
        const e = eventBindings.get(i);
        init.append_view("$_ut(root,");
        init.append_view(e.path);
        init.append_view(",offset).addEventListener('");
        init.append_view(e.eventName);
        init.append_view("',");
        init.append_view(e.handlerExpr);
        init.append_view(");");
    }
    init.append_view("}");

    comp.hydrationLogic = builder.allocate_view(init.to_view());
}
