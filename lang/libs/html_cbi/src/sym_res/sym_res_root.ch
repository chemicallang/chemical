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

    const appendHtmlIntFn = page.child("append_html_integer");
    if(appendHtmlIntFn == null) {
        resolver.error("'append_html_integer' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlUIntFn = page.child("append_html_uinteger");
    if(appendHtmlUIntFn == null) {
        resolver.error("'append_html_uinteger' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlFloatFn = page.child("append_html_float");
    if(appendHtmlFloatFn == null) {
        resolver.error("'append_html_float' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlDoubleFn = page.child("append_html_double");
    if(appendHtmlDoubleFn == null) {
        resolver.error("'append_html_double' function is required on 'page' for html to work", loc);
        return false;
    }

    var support = &mut root.support
    support.pageNode = page;
    support.appendHtmlCharFn = appendHtmlCharFn
    support.appendHtmlCharPtrFn = appendHtmlCharPtrFn
    support.appendHtmlFn = appendHtmlFn
    support.appendHtmlIntFn = appendHtmlIntFn;
    support.appendHtmlUIntFn = appendHtmlUIntFn;
    support.appendHtmlFloatFn = appendHtmlFloatFn;
    support.appendHtmlDoubleFn = appendHtmlDoubleFn;

    return true;

}