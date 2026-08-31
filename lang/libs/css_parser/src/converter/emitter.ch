/**
 * CssEmitter is a static interface that abstracts how converted CSS output
 * is emitted.
 *
 * The compiler plugin (css_cbi) implements this interface by accumulating text
 * into a buffer and emitting `page.append_css(...)` calls into the generated
 * chemical AST (through flush() and emit_chemical_value()).
 *
 * The runtime package (css) implements this interface by appending text
 * directly into a std::string, so users can parse CSS and get the resulting
 * CSS source back as a string in their executable.
 *
 * Since this is a static interface, only a single implementation may exist
 * in a program. The css_parser package contains the shared conversion logic
 * (CssConverter) which drives this interface.
 */
@static
public interface CssEmitter {

    /**
     * Append raw CSS text to the output buffer.
     */
    func emit_text(&mut self, text : &std::string_view)

    /**
     * Append a single character to the output buffer.
     */
    func emit_char(&mut self, c : char)

    /**
     * Append an integer value to the output buffer.
     */
    func emit_integer(&mut self, v : bigint)

    /**
     * Append an unsigned integer value to the output buffer.
     */
    func emit_uinteger(&mut self, v : ubigint)

    /**
     * Called to flush any pending buffered text. In the compiler plugin,
     * this emits a `page.append_css(str, len)` call. In the runtime,
     * this is a no-op.
     */
    func flush(&mut self)

    /**
     * Called when a Chemical value (interpolation) is encountered inside
     * a CSS value. In the compiler plugin, this emits the appropriate
     * AST call. In the runtime, this is a no-op.
     */
    func emit_chemical_value(&mut self, value : *mut Value)

}
