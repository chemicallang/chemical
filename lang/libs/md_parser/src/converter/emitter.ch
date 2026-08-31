/**
 * MdEmitter abstracts how MD-to-HTML conversion output is emitted.
 *
 * The compiler plugin (md_cbi) implements this by accumulating text
 * and flushing to AST nodes. The runtime (md) implements it by
 * appending to a std::string.
 *
 * Callbacks for code highlighting and link rewriting are passed through
 * to the shared convert_md_node function.
 */
@static
public interface MdEmitter {

    /**
     * Append raw HTML text to the output.
     */
    func emit_text(&mut self, text : &std::string_view)

    /**
     * Append a single character to the output.
     */
    func emit_char(&mut self, c : char)

    /**
     * Append an integer value to the output.
     */
    func emit_integer(&mut self, v : bigint)

    /**
     * Flush any pending buffered text to the output.
     */
    func flush(&mut self)

    /**
     * Handle Chemical interpolation (${...}) — emit the chemical value.
     * In the runtime, this is a no-op.
     */
    func emit_interpolation(&mut self, value : *mut Value)

}
