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
 * Allocate arena storage for a `T` that is not initialized yet.
 *
 * The returned reference is to a `MaybeUninit<T>`, so safe code can never read
 * a `T` before it has been written. Initialize the slot with
 * `MaybeUninit::write`, then call `unsafe(assume_init)` when the value is
 * ready. The storage lives as long as the builder's arena.
 */
public func <T> allocate_uninit(builder : &mut ASTBuilder) : &mut std::MaybeUninit<T> {
    const raw = builder.allocate_size(sizeof(std::MaybeUninit<T>), alignof(std::MaybeUninit<T>))
    const ptr = raw as *mut std::MaybeUninit<T>
    unsafe {
        memset(ptr as *mut void, 0, sizeof(std::MaybeUninit<T>))
    }
    return unsafe(&mut *ptr)
}

/*
 * `allocate_default<T: Default>()` cannot be expressed until a generic
 * function returning `&mut T` under a `T: Default` bound links (the checker
 * currently rejects the call with an identical-type lifetime mismatch). Use
 * `allocate_uninit<T>` plus `MaybeUninit::write`, or `allocate_value` with a
 * caller-built default.
 */

/**
 * Move `value` into the builder's arena and return a stable reference to it.
 *
 * The slot is explicitly initialized with `MaybeUninit::write` before the
 * reference is created, so the returned `&mut T` never points at
 * uninitialized storage. Allocation failure (arena growth) follows the
 * builder's normal behavior; the wrapper itself neither dereferences caller
 * memory nor leaks.
 */
public func <T> allocate_value(builder : &mut ASTBuilder, value : T) : &mut T {
    const slot : &mut std::MaybeUninit<T> = allocate_uninit<T>(builder)
    slot.write(value)
    return unsafe(&mut *slot.as_ptr())
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

/// Advance past the current token (safe spelling of `Parser::increment`).
public func increment(parser : &mut Parser) {
    parser.increment()
}

/// True when the parser has no current token.
public func at_end(parser : &mut Parser) : bool {
    return current(parser) is std::Option.None
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

/// The next character of `provider` without advancing.
public func peek(provider : &SourceProvider) : char {
    return provider.peek()
}

/// Consume and return the next character of `provider`.
public func read_char(provider : &SourceProvider) : char {
    return provider.readCharacter()
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

/// The parsing position, or `None` at end of input.
public func (parser : &mut Parser) current() : std::Option<&Token> {
    return compiler::api::current(parser)
}

/// Safe `SourceProvider::peek` spelling.
public func (provider : &SourceProvider) peek_char() : char {
    return compiler::api::peek(provider)
}

/// Safe `SourceProvider::readCharacter` spelling.
public func (provider : &SourceProvider) read_char() : char {
    return compiler::api::read_char(provider)
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

/**
 * Raw compiler-CBI operations.
 *
 * Everything here returns/accepts compiler-owned pointers or uninitialized
 * arena storage. Normal macro/plugin code should use `compiler::api`; reach for
 * `compiler::ffi` only when implementing the parser/arena plumbing itself.
 */
public namespace ffi {

    /// Raw arena allocation of `T` (uninitialized bytes).
    public func <T> allocate(builder : &mut ASTBuilder) : *mut T {
        return builder.allocate<T>()
    }

    /// Raw arena allocation of `obj_size` bytes.
    public func allocate_size(builder : &mut ASTBuilder, obj_size : size_t, alignment : size_t) : *mut char {
        return builder.allocate_size(obj_size, alignment)
    }

    /// Raw arena allocation of a NUL-terminated string buffer.
    public func allocate_str_size(builder : &mut ASTBuilder, size : size_t) : *mut char {
        return builder.allocate_str_size(size)
    }

    /// Raw arena copy of a C string.
    public func allocate_str(builder : &mut ASTBuilder, data : *char, size : size_t) : *mut char {
        return builder.allocate_str(data, size)
    }

    /// Raw arena copy of a string view.
    public func allocate_view(builder : &mut ASTBuilder, view : &string_view) : 'static string_view {
        return builder.allocate_view(view)
    }

    /// Raw current-token pointer (null at end of input).
    public func get_token(parser : &mut Parser) : *mut Token {
        return parser.getToken()
    }

    /// Raw token-slot assignment.
    public func set_token(parser : &mut Parser, token : *mut Token) {
        parser.setToken(token)
    }

    /// Raw token advance.
    public func increment(parser : &mut Parser) {
        parser.increment()
    }

    /// Raw parent-node pointer.
    public func get_parent_node(parser : &mut Parser) : *mut ASTNode {
        return parser.getParentNode()
    }

    /// Raw parent-node assignment.
    public func set_parent_node(parser : &mut Parser, node : *mut ASTNode) {
        parser.setParentNode(node)
    }

    /// Raw `SourceProvider` character read.
    public func read_character(provider : &SourceProvider) : char {
        return provider.readCharacter()
    }

} // namespace ffi

} // namespace compiler
