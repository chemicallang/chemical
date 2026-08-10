/**
 * JsNodeEmitter is a static interface that abstracts how converted JS output
 * is emitted.
 *
 * The compiler plugin (js_cbi) implements this interface by accumulating text
 * into a buffer and emitting `page.append_js(...)` calls into the generated
 * chemical AST (through flush() and emit_chemical_value()).
 *
 * The runtime package (js) implements this interface by appending text
 * directly into a std::string, so users can parse JS and get the resulting
 * JS source back as a string in their executable.
 *
 * Since this is a static interface, only a single implementation may exist
 * in a program. The js_parser package contains the shared conversion logic
 * (convert_js_node / convert_js_root) which drives this interface.
 */
@static
public interface JsNodeEmitter {

    func emit_text(&mut self, text : &std::string_view)

    func emit_char(&mut self, c : char)

    func emit_integer(&mut self, v : bigint)

    /**
     * called when a chemical value is embedded inside the js code
     * (for example `var x = ${value};`)
     */
    func emit_chemical_value(&mut self, value : *mut Value)

    /**
     * called to flush any pending buffered text
     */
    func flush(&mut self)

    /**
     * whether the emitted node is inside a jsx component (only meaningful
     * for the compiler implementation, runtime always returns false)
     */
    func has_jsx_parent(&self) : bool

}
