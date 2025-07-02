func sym_res_element(
    element : *HtmlElement,
    resolver : *mut SymbolResolver,
    loc : ubigint
) {

    var i = 0;
    const total = element.children.size()
    while(i < total) {
        const child = element.children.get(i)
        if(child.kind == HtmlChildKind.Element) {
            const elem = child as *HtmlElement
            sym_res_element(elem, resolver, loc)
        } else if(child.kind == HtmlChildKind.ChemicalValue) {
            const elem = child as *HtmlChemValueChild;
            elem.value.link(&elem.value, null, resolver)
        }
        i++;
    }

}

func sym_res_root(
    root : *mut HtmlRoot,
    resolver : *mut SymbolResolver,
    loc : ubigint
) : bool {

    const page = resolver.find("page");
    if(page == null) {
        resolver.error("couldn't find page variable", loc);
        return false;
    }

    const appendHtmlCharFn = page.child("append_html_char")
    if(appendHtmlCharFn == null) {
        resolver.error("'append_html_char' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlCharPtrFn = page.child("append_html_char_ptr")
    if(appendHtmlCharPtrFn == null) {
        resolver.error("'append_html_char_ptr' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlFn = page.child("append_html");
    if(appendHtmlFn == null) {
        resolver.error("'append_html' function is required on 'page' for html to work", loc);
        return false;
    }

    var support = &root.support
    support.pageNode = page;
    support.appendHtmlCharFn = appendHtmlCharFn
    support.appendHtmlCharPtrFn = appendHtmlCharPtrFn
    support.appendHtmlFn = appendHtmlFn

    sym_res_element(root.element, resolver, loc)

    return true;

}