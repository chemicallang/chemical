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
    out.append_view(") {const tpl=document.createElement('template');tpl.innerHTML='");
    converter.escapeJs(comp.templateHtml);
    out.append_view("';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);");
    get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *out);
    out.append_view(".__hydrate(n,");
    out.append_view(signature.propsName);
    out.append_view("||{});return n;}");

    get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *out);
    out.append_view(".__hydrate=(root,");
    out.append_view(signature.propsName);
    out.append_view(")=>");
    out.append_view(comp.hydrationLogic);
    out.append_view(";if(window.$_ureg)window.$_ureg('");
    get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *out);
    out.append_view("',");
    get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *out);
    out.append_view(");");
}
