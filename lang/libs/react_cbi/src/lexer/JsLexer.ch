public struct JsLexer {
    var lb_count : int = 0
    var chemical_mode : bool = false
    var jsx_depth : int = 0
    var in_jsx_tag : int = 0 // 0: false, 1: true
    var jsx_brace_count : int = 0
    var tag_mode_stack : ubigint = 0
    var jsx_brace_stack : ubigint = 0
}

func (lexer : &mut JsLexer) reset() {
    lexer.lb_count = 0
    lexer.chemical_mode = false
    lexer.jsx_depth = 0
    lexer.in_jsx_tag = 0
    lexer.jsx_brace_count = 0
    lexer.jsx_brace_stack = 0
}
