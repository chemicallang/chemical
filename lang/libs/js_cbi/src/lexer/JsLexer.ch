public struct JsLexer {
    var lb_count : int
    var chemical_mode : bool
}

func (lexer : &mut JsLexer) reset() {
    lexer.lb_count = 0
    lexer.chemical_mode = false
}
