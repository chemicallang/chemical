func sym_res_root(
    om : *mut CSSOM,
    resolver : *mut SymbolResolver,
    loc : ubigint
) : bool {

    const page = resolver.find("page");
    if(page == null) {
        resolver.error("couldn't find page variable", loc);
        return false;
    }

    const appendCssCharFn = page.child("append_css_char")
    if(appendCssCharFn == null) {
        resolver.error("'append_css_char' function is required on 'page' for css to work", loc);
        return false;
    }

    const appendCssCharPtrFn = page.child("append_css_char_ptr")
    if(appendCssCharPtrFn == null) {
        resolver.error("'append_css_char_ptr' function is required on 'page' for css to work", loc);
        return false;
    }

    const appendCssFn = page.child("append_css");
    if(appendCssFn == null) {
        resolver.error("'append_css' function is required on 'page' for css to work", loc);
        return false;
    }

    var support = &mut om.support
    support.pageNode = page;
    support.appendCssCharFn = appendCssCharFn
    support.appendCssCharPtrFn = appendCssCharPtrFn
    support.appendCssFn = appendCssFn

    return true;

}