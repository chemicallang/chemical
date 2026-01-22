public struct JsLexer {
    var lb_count : int
    var chemical_mode : bool
    var jsx_depth : int
    var in_jsx_tag : int // 0: false, 1: true
    var jsx_brace_count : int
    var tag_mode_stack : ubigint
}

func (lexer : &mut JsLexer) reset() {
    lexer.lb_count = 0
    lexer.chemical_mode = false
    lexer.jsx_depth = 0
    lexer.in_jsx_tag = 0
    lexer.jsx_brace_count = 0
}
