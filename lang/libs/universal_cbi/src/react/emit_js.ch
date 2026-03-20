func append_universal_component_js(
    converter : &mut JsConverter,
    comp : *mut JsComponentDecl
) {
    if(comp == null) return;
    const signature = &mut comp.signature;
    var out = &mut converter.str;

    var scopedName = std::string();
    get_module_scoped_name(signature.functionNode, signature.name, scopedName);
    const scopedNameView = converter.builder.allocate_view(scopedName.to_view());

    out.append_view("function ");
    out.append_view(scopedNameView);
    out.append_view("(");
    out.append_view(signature.propsName);
    out.append_view(") ");
    if(comp.body != null) {
        const prevTarget = converter.target;
        converter.target = BufferType.JavaScript;
        converter.convertJsNode(comp.body);
        converter.target = prevTarget;
    } else {
        out.append_view("{ return null; }");
    }
    out.append_view("\n");
    out.append_view(scopedNameView);
    out.append_view(".__hydrate = function(elem, props) { return window.$__uni_mount(elem, ");
    out.append_view(scopedNameView);
    out.append_view(", props || {}); };\n");
    out.append_view("window.$_u = window.$_u || {};\n");
    out.append_view("window.$_u['");
    out.append_view(scopedNameView);
    out.append_view("'] = ");
    out.append_view(scopedNameView);
    out.append_view(";\n");
    out.append_view("window['");
    out.append_view(scopedNameView);
    out.append_view("'] = ");
    out.append_view(scopedNameView);
    out.append_view(";\n");
}
