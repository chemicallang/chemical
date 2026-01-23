public func sym_res_root(root : *mut MdRoot, resolver : *mut SymbolResolver, loc : ubigint) {
    const pageNode = resolver.find(std::string_view("page"))
    if(pageNode == null) {
        // Not erroring out here because it might be used as a simple string macro?
        // But if it's meant to write html, we probably need it.
        // For now, let's try to find it.
        return;
    }
    
    root.support.pageNode = pageNode;


    // TODO: these are not NULL
    root.support.appendHtmlCharFn = pageNode.child(std::string_view("append_html_char"))
    root.support.appendHtmlCharPtrFn = pageNode.child(std::string_view("append_html_char_ptr"))
    root.support.appendHtmlFn = pageNode.child(std::string_view("append_html"))
    root.support.appendHtmlIntFn = pageNode.child(std::string_view("append_html_integer"))
    root.support.appendHtmlUIntFn = pageNode.child(std::string_view("append_html_uinteger"))
    root.support.appendHtmlFloatFn = pageNode.child(std::string_view("append_html_float"))
    root.support.appendHtmlDoubleFn = pageNode.child(std::string_view("append_html_double"))
}
