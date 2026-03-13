func (converter : &mut JsConverter) flush_text(out : &mut std::vector<TemplateToken>) {
    if(!converter.str.empty()) {
        out.push(TemplateToken { kind : TemplateTokenKind.Text, value : converter.builder.allocate_view(converter.str.to_view()) });
        converter.str.clear();
    }
}

func js_node_to_source(
    builder : *mut ASTBuilder,
    node : *mut JsNode,
    states : &std::vector<UniversalStateDecl>,
    support : *mut SymResSupport,
    tokens : *mut std::vector<TemplateToken>
) : std::string_view {
    var tmpConv = JsConverter {
        builder : builder,
        support : support,
        vec : null,
        tokens : tokens,
        parent : null,
        str : std::string(),
        jsx_parent : view(""),
        t_counter : 0,
        state_vars : std::vector<std::string_view>()
    }
    for(var i : uint = 0; i < states.size(); i++) {
        tmpConv.state_vars.push(states.get(i).name);
    }
    tmpConv.convertJsNode(node);
    return builder.allocate_view(tmpConv.str.to_view());
}

func build_child_path(builder : *mut ASTBuilder, parentPath : std::string_view, childIndex : uint) : std::string_view {
    var p = std::string();
    if(parentPath.size() <= 2) {
        p.append_view("[");
        p.append_integer(childIndex as bigint);
        p.append_view("]");
    } else {
        p.append_view(parentPath.subview(0, parentPath.size() - 1));
        p.append_view(",");
        p.append_integer(childIndex as bigint);
        p.append_view("]");
    }
    return builder.allocate_view(p.to_view());
}
