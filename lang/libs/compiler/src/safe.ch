// Safe compiler-API helpers. The raw CBI interfaces remain unchanged; these
// wrappers remove ordinary caller-side raw-pointer access for plugin/build code.
public namespace compiler_api_safe {

using std::Option;

/// Current token, or None at end of input.
public func current(parser : &mut Parser) : Option<&Token> {
    var t = parser.getToken()
    if(t == null) { return Option.None<&Token>() }
    return Option.Some<&Token>(unsafe(&*t))
}

/// True when the parser is at end of input.
public func at_end(parser : &mut Parser) : bool {
    return parser.getToken() == null
}

/// Intern a UTF-8 string into the arena.
public func allocate_str(builder : &mut ASTBuilder, text : &str) : std::string_view {
    var view = unsafe(std::string_view(text.data(), text.size()))
    return builder.allocate_view(&view)
}

/// Resolve a symbol through the body-link pass; None when unresolved.
public func resolve(link : &mut SymResLinkBody, name : &str) : Option<&ASTNode> {
    var view = unsafe(std::string_view(name.data(), name.size()))
    var node = link.resolve(&view)
    if(node == null) { return Option.None<&ASTNode>() }
    return Option.Some<&ASTNode>(unsafe(&*node))
}

/// Emit a diagnostic without exposing the raw location encoding.
public func error(diag : &ASTDiagnoser, msg : &str, loc : ubigint) {
    var view = unsafe(std::string_view(msg.data(), msg.size()))
    diag.error(&view, loc)
}



}
