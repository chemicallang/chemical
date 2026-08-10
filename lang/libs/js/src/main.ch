/**
 * Runtime `js` package.
 *
 * Allows parsing and converting JavaScript at runtime, without the compiler:
 *
 *     var out = js::parse_js("var x = 1 + 2;")
 *
 * The same js_parser package that powers the `#js` compiler macro is reused
 * here, with runtime implementations of the compiler `Parser` and
 * `BatchAllocator` static interfaces.
 */
using namespace std;

public namespace js {

public func tokenize_js(view : std::string_view) : std::vector<Token> {
    var tokenizer = JsTokenizer { src : view, pos : 0, line : 0, character : 0 }
    return tokenizer.tokenize()
}

public func parse_js(view : std::string_view) : std::string {
    var tokens = tokenize_js(view)

    var parser = RuntimeParser { current : null, tokens : std::vector<Token>(), parent : null }
    parser.setup(tokens)

    var allocator = ASTAllocator.make()
    var builder = ASTBuilder { allocator : (&raw mut allocator) as *mut BatchAllocator, typeBuilder : null }

    const root = parseJsRoot((&raw mut parser) as *mut Parser, &raw mut builder)
    if(root == null) {
        allocator.deinit()
        return std::string()
    }

    var out = std::string()
    var converter = JsRuntimeConverter { str : &raw mut out }
    convert_js_root(root as *mut JsRoot, &mut converter)
    allocator.deinit()
    return out
}

public func convert_js_node_to_string(node : *mut JsNode) : std::string {
    var out = std::string()
    var converter = JsRuntimeConverter { str : &raw mut out }
    convert_js_node(node, &mut converter)
    return out
}

}
