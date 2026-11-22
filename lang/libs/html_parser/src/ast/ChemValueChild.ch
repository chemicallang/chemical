
// A `{expr}` (or `@(expr)`) in a template. `raw` says which: `{expr}` is text and the
// code generator escapes it, `@(expr)` is spliced verbatim for a value that already
// is markup.
public struct HtmlChemValueChild : HtmlChild {

    var value : *mut Value

    var raw : bool

}
