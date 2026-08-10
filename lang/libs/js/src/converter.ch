/**
 * Runtime implementation of the shared `js_parser::JsNodeEmitter` interface.
 *
 * The compiler plugin (js_cbi) implements JsNodeEmitter by emitting
 * `page.append_js(...)` calls into the generated chemical AST. This runtime
 * implementation appends the converted JS text into a std::string, so users
 * can parse JS and obtain the resulting JS source as a string at runtime.
 *
 * The output string is owned by the caller and passed as a pointer, so this
 * struct has no destructor and can be freely moved around by the interface.
 */
using namespace std;

public struct JsRuntimeConverter {
    var str : *mut std::string
}

impl JsNodeEmitter for JsRuntimeConverter {

    func emit_text(&mut self, text : &std::string_view) {
        self.str.append_view(text)
    }

    func emit_char(&mut self, c : char) {
        self.str.append(c)
    }

    func emit_integer(&mut self, v : bigint) {
        self.str.append_integer(v)
    }

    func emit_chemical_value(&mut self, value : *mut Value) {
        // no chemical values are embedded at runtime
    }

    func flush(&mut self) {
    }

    func has_jsx_parent(&self) : bool {
        return false
    }

}
