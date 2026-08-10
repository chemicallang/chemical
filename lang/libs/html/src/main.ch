/**
 * Runtime `html` package.
 *
 * Allows parsing and converting HTML at runtime, without the compiler:
 *
 *     var out = html::parse_html("<div class=\"test\">Hello</div>")
 *
 * The same html_parser package that powers the `#html` compiler macro is
 * reused here, with runtime implementations of the compiler `Parser`,
 * `SourceProvider` and `BatchAllocator` interfaces provided by the shared
 * compiler_runtime package.
 */
using namespace std;

public namespace html {

public func tokenize_html(view : std::string_view) : std::vector<Token> {
    return tokenize_html_impl(view)
}

public func parse_html(view : std::string_view) : std::string {
    var tokens = tokenize_html_impl(view)

    var parser = RuntimeParser { current : null, tokens : std::vector<Token>(), parent : null }
    parser.setup(tokens)

    var allocator = ASTAllocator.make()
    var builder = ASTBuilder { allocator : (&raw mut allocator) as *mut BatchAllocator, typeBuilder : null }

    const root = parseHtmlRoot((&raw mut parser) as *mut Parser, &raw mut builder)
    if(root == null) {
        allocator.deinit()
        return std::string()
    }

    var out = std::string()
    convert_html_root(root as *mut HtmlRoot, &mut out)
    allocator.deinit()
    return out
}

public func parse_html_root(view : std::string_view, allocator : *mut ASTAllocator) : *mut HtmlRoot {
    var tokens = tokenize_html_impl(view)

    var parser = RuntimeParser { current : null, tokens : std::vector<Token>(), parent : null }
    parser.setup(tokens)

    var builder = ASTBuilder { allocator : allocator as *mut BatchAllocator, typeBuilder : null }

    return parseHtmlRoot((&raw mut parser) as *mut Parser, &raw mut builder) as *mut HtmlRoot
}

public func convert_html_root_to_string(root : *mut HtmlRoot) : std::string {
    var out = std::string()
    convert_html_root(root, &mut out)
    return out
}

}
