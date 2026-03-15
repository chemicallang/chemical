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

    // creating the component function
    // func name(page : &mut HtmlPage, attrs : SsrAttributeList) : void
    const voidType = builder.make_void_type(loc);
    const funcDecl = builder.make_function(root.signature.name, voidType, false, true, node.getParent(), loc);
    // the first param for the function, page : &mut HtmlPage
    const linked = builder.make_linked_type(std::string_view("HtmlPage"), root.htmlPageNode, loc);
    const ref = builder.make_reference_type(linked, true, loc);
    const params = funcDecl.get_params();
    const param = builder.make_function_param(std::string_view("page"), ref, 0, null, false, funcDecl, loc);
    params.push(param);

    // resolution of required types and functions for code generation
    root.support.pageNode = param as *mut ASTNode;
    resolve_page_children(resolver, root.support, root.support.pageNode, loc);
    sym_res_support(resolver, root.support, loc);

    // the second param for the function, attrs : SsrAttributeList
    const attrListNodeType = builder.make_linked_type(std::string_view("SsrAttributeList"), root.support.ssrAttributeListNode, loc);
    const attrListPtrType = builder.make_ptr_type(attrListNodeType as *mut BaseType, false, loc);
    const param2 = builder.make_function_param(std::string_view("attrs"), attrListPtrType as *mut BaseType, 1, null, false, funcDecl, loc);
    params.push(param2)
    // add a body
    funcDecl.add_body();

    // set the function to signature, so we can actually access it later
    root.signature.functionNode = funcDecl;

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

}

@no_mangle
public func universal_symResDeclareNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const comp = node.getDataPtr() as *mut JsComponentDecl;
    resolver.declare(comp.signature.name, node);
}
