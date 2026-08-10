/**
 * Runtime `universal` package.
 *
 * Allows parsing universal component source (JS + JSX) at runtime, without
 * the compiler:
 *
 *     var out = universal::parse_universal("<div>hello</div>")
 *
 * The same universal_parser package that powers the `#universal` compiler
 * macro is reused here, with runtime implementations of the compiler `Parser`
 * and `BatchAllocator` static interfaces.
 */
using namespace std;

public namespace universal {

public func tokenize_universal(view : std::string_view) : std::vector<Token> {
    var tokenizer = UniversalTokenizer { src : view, pos : 0, line : 0, character : 0 }
    return tokenizer.tokenize()
}

public func parse_universal(view : std::string_view) : std::string {
    var tokens = tokenize_universal(view)

    var parser = RuntimeParser { current : null, tokens : std::vector<Token>(), parent : null }
    parser.setup(tokens)

    var allocator = ASTAllocator.make()
    var builder = ASTBuilder { allocator : (&raw mut allocator) as *mut BatchAllocator, typeBuilder : null }

    const root = parseUniversalRoot((&raw mut parser) as *mut Parser, &raw mut builder)
    if(root == null) {
        allocator.deinit()
        return std::string()
    }

    var out = std::string()
    var converter = UniversalRuntimeConverter { str : &raw mut out }
    convert_universal_root(root, &mut converter)
    allocator.deinit()
    return out
}

public func convert_universal_node_to_string(node : *mut JsNode) : std::string {
    var out = std::string()
    var converter = UniversalRuntimeConverter { str : &raw mut out }
    convert_universal_node(node, &mut converter)
    return out
}

}
