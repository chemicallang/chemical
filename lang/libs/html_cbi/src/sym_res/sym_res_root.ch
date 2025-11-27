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

    const appendHeadCharFn = page.child("append_head_char")
    if(appendHeadCharFn == null) {
        resolver.error("'append_head_char' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadCharPtrFn = page.child("append_head_char_ptr")
    if(appendHeadCharPtrFn == null) {
        resolver.error("'append_head_char_ptr' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadFn = page.child("append_head");
    if(appendHeadFn == null) {
        resolver.error("'append_head' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadIntFn = page.child("append_head_integer");
    if(appendHeadIntFn == null) {
        resolver.error("'append_head_integer' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadUIntFn = page.child("append_head_uinteger");
    if(appendHeadUIntFn == null) {
        resolver.error("'append_head_uinteger' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadFloatFn = page.child("append_head_float");
    if(appendHeadFloatFn == null) {
        resolver.error("'append_head_float' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadDoubleFn = page.child("append_head_double");
    if(appendHeadDoubleFn == null) {
        resolver.error("'append_head_double' function is required on 'page' for html to work", loc);
        return false;
    }

    support.appendHeadCharFn = appendHeadCharFn
    support.appendHeadCharPtrFn = appendHeadCharPtrFn
    support.appendHeadFn = appendHeadFn
    support.appendHeadIntFn = appendHeadIntFn;
    support.appendHeadUIntFn = appendHeadUIntFn;
    support.appendHeadFloatFn = appendHeadFloatFn;
    support.appendHeadDoubleFn = appendHeadDoubleFn;

    return true;

}