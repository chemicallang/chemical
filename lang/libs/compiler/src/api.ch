// compiler::api — safe facade over the raw CBI compiler bindings.
//
// The top level of the `compiler` module mirrors the compiler's C++ CBI ABI:
// raw pointers, nullable out-params, and body-less `@compiler.interface`
// methods. That surface stays available unchanged for plugins that need it.
//
// This namespace adds the additive safe layer:
//
//   * `std::Option<&T>` instead of nullable pointers
//   * `&mut` receivers instead of out-params
//   * `&str` message/name parameters instead of `*char`/`string_view`
//   * bounded, checked access to compiler pointer vectors
//
// Nothing here is `unsafe func`; the small number of unavoidable conversions
// from a compiler-owned raw pointer to a reference live inside these wrappers.

using std::Option;
using std::span;
using std::string_view;

public namespace compiler {

public namespace api {

// -------------------------------------------------------------------------
// Parser
// -------------------------------------------------------------------------

/**
 * The parser's current token.
 *
 * Returns `None` when the parser has no token slot or the slot is null. The
 * returned reference borrows the compiler's token storage, which outlives the
 * plugin hook that receives the parser.
 */
public func current(parser : &mut Parser) : std::Option<&Token> {
    const slot = parser.getTokenPtr()
    if(slot == null) {
        return std::Option.None<&Token>()
    }
    const token = unsafe(*slot)
    if(token == null) {
        return std::Option.None<&Token>()
    }
    return std::Option.Some<&Token>(unsafe(&*token))
}

/// Advance past the current token (safe replacement for `increment`).
public func advance(parser : &mut Parser) {
    parser.increment()
}

/// Current token type, or `-1` when the parser has no current token.
public func current_type(parser : &mut Parser) : int {
    var token = current(parser)
    if(token is std::Option.None) {
        return -1
    }
    var Some(value) = token else unsafe { unreachable }
    return value.type
}

// -------------------------------------------------------------------------
// ASTBuilder
// -------------------------------------------------------------------------

/**
 * Move `value` into the builder's arena and return a stable reference to it.
 *
 * Allocation failure (arena growth) follows the builder's normal behavior;
 * the wrapper itself neither dereferences caller memory nor leaks.
 */
public func <T> allocate_value(builder : &mut ASTBuilder, value : T) : &mut T {
    const ptr = builder.allocate<T>()
    unsafe { *ptr = value }
    return unsafe(&mut *ptr)
}


/**
 * Copy a UTF-8 string into the builder's arena and return a view of it.
 *
 * The returned view borrows the arena allocation, not the caller's buffer.
 */
public func allocate_string(builder : &mut ASTBuilder, value : &str) : string_view {
    const ptr = builder.allocate_str(value.data(), value.size())
    return unsafe(string_view(ptr, value.size()))
}

// -------------------------------------------------------------------------
// SourceProvider
// -------------------------------------------------------------------------

/**
 * A safe, borrowing reader over a `SourceProvider`.
 *
 * `read_char` advances the provider; `peek` does not. Both operate on the
 * provider's own cursor, so no raw pointer ever escapes to the caller.
 */
public struct SourceReader<'a> {
    internal var provider : &'a mut SourceProvider

    /// The next character without advancing.
    public func peek(&self) : char {
        return self.provider.peek()
    }

    /// Consume and return the next character.
    public func read_char(&mut self) : char {
        return self.provider.readCharacter()
    }

    /// True when the provider has consumed the whole input.
    public func eof(&self) : bool {
        return self.provider.eof()
    }

    /// Zero-based current line number.
    public func line(&self) : uint {
        return self.provider.getLineNumber()
    }

    /// Zero-based current character number on the current line.
    public func column(&self) : uint {
        return self.provider.getLineCharNumber()
    }

    /// Current line/character position.
    public func position(&self) : Position {
        return self.provider.getPosition()
    }
}

/// Borrow a `SourceProvider` as a safe character reader.
public func <'a> source_reader(provider : &'a mut SourceProvider) : SourceReader<'a> {
    return SourceReader<'a> { provider : provider }
}

// -------------------------------------------------------------------------
// Symbol resolution
// -------------------------------------------------------------------------

/**
 * Resolve `name` through the visitor's symbol resolver and current symbol
 * table, returning a reference to the declaration when found.
 */
public func resolve_node(visitor : &mut SymResLinkBody, name : &str) : std::Option<&ASTNode> {
    const found = visitor.resolve(&name.to_view())
    if(found == null) {
        return std::Option.None<&ASTNode>()
    }
    return std::Option.Some<&ASTNode>(unsafe(&*found))
}

/// Report a diagnostic through a `SymResLinkBody`'s diagnoser.
public func visitor_error(visitor : &mut SymResLinkBody, message : &str, location : ubigint) {
    var diag = visitor.getAstDiagnoser()
    unsafe(diag.error(message.to_view(), location))
}

/// Report an error from a `&str` message.
public func report_error(diagnoser : &ASTDiagnoser, message : &str, location : ubigint) {
    diagnoser.error(message.to_view(), location)
}

} // namespace api

/// Safe alias for `compiler::api::current`.
public func (parser : &mut Parser) current_token() : std::Option<&Token> {
    return compiler::api::current(parser)
}

/// Safe alias for `compiler::api::advance`.
public func (parser : &mut Parser) advance_token() {
    compiler::api::advance(parser)
}

/// Safe alias for `compiler::api::current_type`.
public func (parser : &mut Parser) current_token_type() : int {
    return compiler::api::current_type(parser)
}

/// Safe alias for `compiler::api::resolve_node`.
public func (visitor : &mut SymResLinkBody) resolve_node(name : &str) : std::Option<&ASTNode> {
    return compiler::api::resolve_node(visitor, name)
}

/// Safe alias for `compiler::api::visitor_error`.
public func (visitor : &mut SymResLinkBody) error_str(message : &str, location : ubigint) {
    compiler::api::visitor_error(visitor, message, location)
}

/// Safe alias for `compiler::api::report_error`.
public func (diagnoser : &ASTDiagnoser) error_str(message : &str, location : ubigint) {
    compiler::api::report_error(diagnoser, message, location)
}

} // namespace compiler
