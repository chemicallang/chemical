func append_universal_component_js(
    converter : &mut JsConverter,
    comp : *mut JsComponentDecl
) {
    if(comp == null) return;
    const signature = &mut comp.signature;

    converter.str.append_view("function ");
    get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, converter.str);
    converter.str.append_view("(");
    converter.str.append_view(signature.propsName);
    converter.str.append_view(")");
    converter.str.append(' ');

    if(!signature.universalTemplate.empty()) {
        converter.str.append_view("{const tpl=document.createElement('template');tpl.innerHTML='");
        for(var i : uint = 0; i < signature.universalTemplate.size(); i++) {
            const tok = signature.universalTemplate.get(i);
            if(tok.kind == TemplateTokenKind.Text) {
                append_escaped_single_quoted(converter.str, tok.value);
            }
        }
        converter.str.append_view("';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);");
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, converter.str);
        converter.str.append_view(".__hydrate(n,");
        converter.str.append_view(signature.propsName);
        converter.str.append_view("||{});return n;}");

        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, converter.str);
        converter.str.append_view(".__hydrate=(root,");
        converter.str.append_view(signature.propsName);
        converter.str.append_view(")=>{");
        converter.str.append_view(signature.universalInit);
        converter.str.append_view("};");
    } else {
        if(comp.body != null) {
            converter.convertJsNode(comp.body);
        } else {
            converter.str.append_view("return document.createTextNode('');}");
        }
    }

    converter.str.append_view("if(window.$_ureg)window.$_ureg('");
    get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, converter.str);
    converter.str.append_view("',");
    get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, converter.str);
    converter.str.append_view(");");
}
