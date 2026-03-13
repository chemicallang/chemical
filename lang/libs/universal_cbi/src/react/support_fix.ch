func fix_support_page_node(
    support : &mut SymResSupport,
    page : *mut ASTNode,
    loc : ubigint
) : bool {

    const appendHeadJsCharFn = page.child("append_js_char")
    if(appendHeadJsCharFn == null) {
        return false;
    }

    const appendHeadJsCharPtrFn = page.child("append_js_char_ptr")
    if(appendHeadJsCharPtrFn == null) {
        return false;
    }

    const appendHeadJsFn = page.child("append_js");
    if(appendHeadJsFn == null) {
        return false;
    }

    const appendHeadJsIntFn = page.child("append_js_integer");
    if(appendHeadJsIntFn == null) {
        return false;
    }

    const appendHeadJsUIntFn = page.child("append_js_uinteger");
    if(appendHeadJsUIntFn == null) {
        return false;
    }

    const appendHeadJsFloatFn = page.child("append_js_float");
    if(appendHeadJsFloatFn == null) {
        return false;
    }

    const appendHeadJsDoubleFn = page.child("append_js_double");
    if(appendHeadJsDoubleFn == null) {
        return false;
    }

    const appendHtmlFn = page.child("append_html");
    if(appendHtmlFn == null) return false;
    const appendHtmlCharFn = page.child("append_html_char");
    if(appendHtmlCharFn == null) return false;
    const appendHtmlCharPtrFn = page.child("append_html_char_ptr");
    if(appendHtmlCharPtrFn == null) return false;
    const appendHtmlIntFn = page.child("append_html_integer");
    if(appendHtmlIntFn == null) return false;
    const appendHtmlUIntFn = page.child("append_html_uinteger");
    if(appendHtmlUIntFn == null) return false;
    const appendHtmlFloatFn = page.child("append_html_float");
    if(appendHtmlFloatFn == null) return false;
    const appendHtmlDoubleFn = page.child("append_html_double");
    if(appendHtmlDoubleFn == null) return false;
    // Optional: append_html_attributes_spread may not exist yet
    const appendHtmlAttributesSpreadFn = page.child("append_html_attributes_spread");

    const requireComponentFn = page.child("require_component");
    if(requireComponentFn == null) {
        return false;
    }

    const setComponentHashFn = page.child("set_component_hash");
    if(setComponentHashFn == null) {
        return false;
    }

    support.requireComponentFn = requireComponentFn;
    support.setComponentHashFn = setComponentHashFn;

    support.appendHeadJsFn = appendHeadJsFn
    support.appendHeadJsCharFn = appendHeadJsCharFn
    support.appendHeadJsCharPtrFn = appendHeadJsCharPtrFn
    support.appendHeadJsIntFn = appendHeadJsIntFn;
    support.appendHeadJsUIntFn = appendHeadJsUIntFn;
    support.appendHeadJsFloatFn = appendHeadJsFloatFn;
    support.appendHeadJsDoubleFn = appendHeadJsDoubleFn;

    support.appendHtmlFn = appendHtmlFn;
    support.appendHtmlCharFn = appendHtmlCharFn;
    support.appendHtmlCharPtrFn = appendHtmlCharPtrFn;
    support.appendHtmlIntFn = appendHtmlIntFn;
    support.appendHtmlUIntFn = appendHtmlUIntFn;
    support.appendHtmlFloatFn = appendHtmlFloatFn;
    support.appendHtmlDoubleFn = appendHtmlDoubleFn;
    support.appendHtmlAttributesSpreadFn = appendHtmlAttributesSpreadFn;

    return true
}
