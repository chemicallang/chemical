/**
 * Runtime `css` package.
 *
 * Allows parsing and converting CSS at runtime, without the compiler:
 *
 *     var out = css::parse_css("body { color: red }")
 *
 * The same css_parser package that powers the `#css` compiler macro is
 * reused here, with runtime implementations of the compiler `Parser`,
 * `SourceProvider` and `BatchAllocator` interfaces provided by the shared
 * compiler_runtime package.
 */
using namespace std;

public namespace css {

public func tokenize_css(view : std::string_view) : std::vector<Token> {
    return tokenize_css_impl(view)
}

public func parse_css(view : std::string_view) : std::string {
    var tokens = tokenize_css_impl(view)

    var parser = RuntimeParser { current : null, tokens : std::vector<Token>(), parent : null }
    parser.setup(tokens)

    var allocator = ASTAllocator.make()
    var builder = ASTBuilder { allocator : (&raw mut allocator) as *mut BatchAllocator, typeBuilder : null }

    const root = parseCSSOM((&raw mut parser) as *mut Parser, &raw mut builder)
    if(root == null) {
        allocator.deinit()
        return std::string()
    }

    var out = std::string()
    convert_css_root(root as *mut CSSOM, &mut out)
    allocator.deinit()
    return out
}

public func parse_css_root(view : std::string_view, allocator : *mut ASTAllocator) : *mut CSSOM {
    var tokens = tokenize_css_impl(view)

    var parser = RuntimeParser { current : null, tokens : std::vector<Token>(), parent : null }
    parser.setup(tokens)

    var builder = ASTBuilder { allocator : allocator as *mut BatchAllocator, typeBuilder : null }

    return parseCSSOM((&raw mut parser) as *mut Parser, &raw mut builder) as *mut CSSOM
}

public func convert_css_root_to_string(root : *mut CSSOM) : std::string {
    var out = std::string()
    convert_css_root(root, &mut out)
    return out
}

}
