public func getNextToken(js : &mut JsLexer, lexer : &mut Lexer) : Token {
    return nextJsToken(js, lexer, true)
}
