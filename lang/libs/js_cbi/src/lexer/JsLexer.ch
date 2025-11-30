public struct JsLexer {
    var lb_count : int
}

func (lexer : &mut JsLexer) reset() {
    lexer.lb_count = 0
}
