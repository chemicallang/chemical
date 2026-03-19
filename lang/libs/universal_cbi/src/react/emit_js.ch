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
    get_module_scoped_name(signature.functionNode , signature.name, *out);
    out.append_view("(");
    out.append_view(signature.propsName);
    out.append_view(") { // TODO: must emit proper hydration function \n }");

}
