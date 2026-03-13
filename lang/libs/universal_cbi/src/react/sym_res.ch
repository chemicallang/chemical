@no_mangle
public func universal_symResSigNode(visitor : *mut SymResLinkSignature, node : *mut EmbeddedNode) {
    // no type or value is included in this node which requires resolution at link signature
    // other than HtmlPage type, however we are comfortable resolving that after
}

@no_mangle
public func universal_symResNode(visitor : *mut SymResLinkBody, node : *mut EmbeddedNode) {
    const resolver = visitor.getSymbolResolver();
    const loc = node.getEncodedLocation();
    const root = node.getDataPtr() as *mut JsComponentDecl;

    root.htmlPageNode = resolver.find(std::string_view("HtmlPage"));
    if(root.htmlPageNode == null) {
        resolver.error(std::string_view("could not find HtmlPage"), loc);
    }

    var builder = resolver.getJobBuilder()

    const voidType = builder.make_void_type(loc);
    const funcDecl = builder.make_function(root.signature.name, voidType as *mut BaseType, false, true, node.getParent(), loc);

    // func name(page : &mut HtmlPage) : void
    const linked = builder.make_linked_type(std::string_view("HtmlPage"), root.htmlPageNode, loc);
    const ref = builder.make_reference_type(linked as *mut BaseType, true, loc);
    const param = builder.make_function_param(std::string_view("page"), ref as *mut BaseType, 0, null, false, funcDecl, loc);

    funcDecl.get_params().push(param);
    funcDecl.add_body();

    // Create helper function to emit the universal component JS into pageJs
    var emitName = std::string();
    emitName.append_view(root.signature.name);
    emitName.append_view("__emit_js");
    const emitView = builder.allocate_view(emitName.to_view());
    const emitDecl = builder.make_function(emitView, voidType as *mut BaseType, false, true, node.getParent(), loc);
    const emitParam = builder.make_function_param(std::string_view("page"), ref as *mut BaseType, 0, null, false, emitDecl, loc);
    emitDecl.get_params().push(emitParam);
    emitDecl.add_body();
    root.signature.jsEmitFunctionNode = emitDecl;

    // start a scope to store symbols
    resolver.scope_start();

    // declare the page param
    resolver.declare_or_shadow(std::string_view("page"), param)

    // visit the body
    visitor.visitEmbeddedNode(node)

    // resolve components
    sym_res_components(root.components, resolver)

    // end the scope
    resolver.scope_end();

    // fixing support
    root.support.pageNode = param as *mut ASTNode
    fix_support_page_node(root.support, root.support.pageNode, loc)

    root.signature.functionNode = funcDecl;

    compute_universal_template(&mut builder, root, &mut root.support);
}

@no_mangle
public func universal_symResDeclareNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const comp = node.getDataPtr() as *mut JsComponentDecl;
    resolver.declare(comp.signature.name, node);
}
