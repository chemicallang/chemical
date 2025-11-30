func sym_res_root(
    root : *mut JsRoot,
    resolver : *mut SymbolResolver,
    loc : ubigint
) : bool {

    const page = resolver.find("page");
    if(page == null) {
        resolver.error("couldn't find page variable", loc);
        return false;
    }

    const appendJsCharFn = page.child("append_js_char")
    if(appendJsCharFn == null) {
        resolver.error("'append_js_char' function is required on 'page' for js to work", loc);
        return false;
    }

    const appendJsCharPtrFn = page.child("append_js_char_ptr")
    if(appendJsCharPtrFn == null) {
        resolver.error("'append_js_char_ptr' function is required on 'page' for js to work", loc);
        return false;
    }

    const appendJsFn = page.child("append_js");
    if(appendJsFn == null) {
        resolver.error("'append_js' function is required on 'page' for js to work", loc);
        return false;
    }

    const appendJsIntFn = page.child("append_js_integer");
    if(appendJsIntFn == null) {
        resolver.error("'append_js_integer' function is required on 'page' for js to work", loc);
        return false;
    }

    const appendJsUIntFn = page.child("append_js_uinteger");
    if(appendJsUIntFn == null) {
        resolver.error("'append_js_uinteger' function is required on 'page' for js to work", loc);
        return false;
    }

    const appendJsFloatFn = page.child("append_js_float");
    if(appendJsFloatFn == null) {
        resolver.error("'append_js_float' function is required on 'page' for js to work", loc);
        return false;
    }

    const appendJsDoubleFn = page.child("append_js_double");
    if(appendJsDoubleFn == null) {
        resolver.error("'append_js_double' function is required on 'page' for js to work", loc);
        return false;
    }

    var support = &mut root.support
    support.pageNode = page;
    support.appendJsCharFn = appendJsCharFn
    support.appendJsCharPtrFn = appendJsCharPtrFn
    support.appendJsFn = appendJsFn
    support.appendJsIntFn = appendJsIntFn;
    support.appendJsUIntFn = appendJsUIntFn;
    support.appendJsFloatFn = appendJsFloatFn;
    support.appendJsDoubleFn = appendJsDoubleFn;

    return true;

}
