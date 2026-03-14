func append_universal_component_js(
    converter : &mut JsConverter,
    comp : *mut JsComponentDecl
) {
    if(comp == null) return;
    const signature = &mut comp.signature;

    // We generate the JS function definition once per page
    const hash = signature.functionNode.getEncodedLocation() as size_t;

    var out = &mut converter.str;
    out.append_view("function ");
    get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *out);
    out.append_view("(");
    out.append_view(signature.propsName);
    out.append_view(") ");

    if(comp.body != null) {
        converter.convertJsNode(comp.body);
    } else {
        out.append_view("{return document.createTextNode('');}");
    }

    out.append_view("\nwindow.$_ureg('");
    get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *out);
    out.append_view("',");
    get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *out);
    out.append_view(");");
}
