public struct CssSymResSupport {

    var pageNode : *mut ASTNode = null

    var appendCssCharFn : *mut ASTNode = null

    var appendCssCharPtrFn : *mut ASTNode = null

    var appendCssFn : *mut ASTNode = null

    var appendCssIntFn : *mut ASTNode = null

    var appendCssUIntFn : *mut ASTNode = null

    var appendCssFloatFn : *mut ASTNode = null

    var appendCssDoubleFn : *mut ASTNode = null

    var requireCssHashFn : *mut ASTNode = null

    var setCssHashFn : *mut ASTNode = null

    var requireRandomCssHashFn : *mut ASTNode = null

    var setRandomCssHashFn : *mut ASTNode = null

    // `page.begin_local_css` / `page.end_local_css`: used to force page-level
    // `#css` (always the page's own CSS, never the shared sink).
    var beginLocalCssFn : *mut ASTNode = null

    var endLocalCssFn : *mut ASTNode = null

}