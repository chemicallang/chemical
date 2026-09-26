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
    var tokenizer = JsSyntaxTokenizer { src : view, pos : 0, line : 0, character : 0, jsx_enabled : true }
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
    var converter = JsRuntimeConverter { str : &raw mut out }
    convert_universal_root(root, &mut converter)
    allocator.deinit()
    return out
}

public func convert_universal_node_to_string(node : *mut JsNode) : std::string {
    var out = std::string()
    var converter = JsRuntimeConverter { str : &raw mut out }
    convert_universal_node(node, &mut converter)
    return out
}

// Test/debug support (Phase 0 of the universal router): parses a universal body
// and summarizes the router declarations it contains, so parser tests can assert
// the node shape without depending on the emitter.
// Format: `router:<name>[;route:<raw>|id=<id>|url=<0|1>|def=<0|1>|fb=<0|1>|mode=<mode>|title=<title>]...`
public func router_decl_summary(view : std::string_view) : std::string {
    var tokens = tokenize_universal(view)

    var parser = RuntimeParser { current : null, tokens : std::vector<Token>(), parent : null }
    parser.setup(tokens)

    var allocator = ASTAllocator.make()
    var builder = ASTBuilder { allocator : (&raw mut allocator) as *mut BatchAllocator, typeBuilder : null }

    const root = parseUniversalRoot((&raw mut parser) as *mut Parser, &raw mut builder)
    var out = std::string()
    if(root != null) {
        for(var i : uint = 0; i < root.statements.size(); i++) {
            const stmt = root.statements.get(i)
            if(stmt.kind != JsNodeKind.RouterDecl) continue
            var rd = stmt as *mut JsRouterDecl
            out.append_view("router:")
            out.append_view(&rd.name)
            for(var j : uint = 0; j < rd.routes.size(); j++) {
                const routeNode = rd.routes.get(j)
                if(routeNode.kind != JsNodeKind.RouteDecl) continue
                var r = routeNode as *mut JsRouteDecl
                out.append_view(";route:")
                out.append_view(&r.raw)
                out.append_view("|id=")
                out.append_view(&r.id)
                out.append_view("|url=")
                if(r.is_url) { out.append('1') } else { out.append('0') }
                out.append_view("|def=")
                if(r.is_default) { out.append('1') } else { out.append('0') }
                out.append_view("|fb=")
                if(r.is_fallback) { out.append('1') } else { out.append('0') }
                out.append_view("|mode=")
                out.append_view(&r.mode)
                out.append_view("|title=")
                out.append_view(&r.title)
                out.append_view("|hooks=")
                if(r.hooks.size() > 0) { out.append('1') } else { out.append('0') }
            }
        }
    }
    allocator.deinit()
    return out
}

}
