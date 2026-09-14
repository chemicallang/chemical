/**
 * Universal (JSX-aware) runtime printer entry points.
 *
 * Thin wrappers over the single printer `convert_js_node(..., universal_mode)`
 * in convert.ch. The runtime `universal` package uses these with the shared
 * `JsNodeEmitter` implementation from the runtime `js` package
 * (`js_parser::JsRuntimeConverter`), so both runtime front ends share one
 * emitter and one printer.
 */
public func convert_universal_node(node : *mut JsNode, emitter : &mut JsNodeEmitter) {
    convert_js_node(node, emitter, true)
}

public func convert_universal_root(root : *mut JsBlock, emitter : &mut JsNodeEmitter) {
    var i = 0u
    while(i < root.statements.size()) {
        convert_js_node(root.statements.get(i), emitter, true)
        i++
    }
    emitter.flush()
}
