/**
 * Runtime `md` package.
 *
 * Parses markdown at runtime, without the compiler. The parsing logic itself
 * is shared with the `md_cbi` compiler plugin through the `md_parser` package:
 * the same lexer (`getNextToken2`) and parser (`parseMdRoot`) are driven here
 * with runtime implementations of the compiler `Parser`, `SourceProvider` and
 * `BatchAllocator` interfaces provided by the shared `compiler_runtime`
 * package.
 */
public namespace md {

using std::Result;

public enum MdError {
    FileReadFailed,
}


// ---------------------------------------------------------------------------
// Runtime arena + raw lex/parse entry points (docgen compatibility).
// The arena wraps a compiler_runtime ASTAllocator and implements the compiler
// BatchAllocator interface, so parseMdRoot allocates its AST inside the arena
// and everything is freed when the arena goes out of scope.
// ---------------------------------------------------------------------------

public struct Arena {
    var allocator : ASTAllocator

    @make
    func make() : Arena {
        return Arena { allocator : ASTAllocator.make() }
    }

    @delete
    func delete(&mut self) {
        // The compiler generates member destruction after this body runs, which
        // calls the wrapped ASTAllocator's delete (freeing all parsed nodes).
    }
}

public func lex(text : std::string_view) : std::vector<Token> {
    return tokenize_md_impl(text)
}

public func parse(tokens : *std::vector<Token>, arena : *mut Arena) : *mut MdRoot {
    var tokens_copy = std::vector<Token>()
    var i = 0u
    while(i < tokens.size()) {
        tokens_copy.push(tokens.get(i))
        i++
    }

    var parser = RuntimeParser { current : null, tokens : std::vector<Token>(), parent : null }
    parser.setup(tokens_copy)

    var builder = ASTBuilder { allocator : (&raw mut arena.allocator) as *mut BatchAllocator, typeBuilder : null }

    return parseMdRoot((&raw mut parser) as *mut Parser, &raw mut builder) as *mut MdRoot
}

public func to_html(text : std::string_view, highlighter : (lang : std::string_view, code : std::string_view) => std::string, link_rewriter : (url : std::string_view) => std::string) : std::string {
    var tokens = tokenize_md_impl(text)

    var parser = RuntimeParser { current : null, tokens : std::vector<Token>(), parent : null }
    parser.setup(tokens)

    var allocator = ASTAllocator.make()
    var builder = ASTBuilder { allocator : (&raw mut allocator) as *mut BatchAllocator, typeBuilder : null }

    const root = parseMdRoot((&raw mut parser) as *mut Parser, &raw mut builder)
    if(root == null) {
        allocator.deinit()
        return std::string()
    }

    var out = render_to_html(root, highlighter, link_rewriter)
    allocator.deinit()
    return out
}

public func file_to_html(path : *char, highlighter : (lang : std::string_view, code : std::string_view) => std::string, link_rewriter : (url : std::string_view) => std::string) : Result<std::string, MdError> {
    var r = fs::read_entire_file(path)
    if(r is Result.Err) {
        var Err(value) = r else unreachable;
        var msg = value.message()
        printf("couldn't read the file %s\n", msg.data());
        return Result.Err<std::string, MdError>(MdError.FileReadFailed)
    }
    var Ok(bytes) = r else unreachable
    const view = std::string_view(bytes.data() as *char, bytes.size())
    const html = md::to_html(view, highlighter, link_rewriter)
    return Result.Ok<std::string, MdError>(html)
}

}
