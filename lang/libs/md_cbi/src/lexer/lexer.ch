public struct MdLexer {
    var in_fenced_code : bool
    var fence_char : char
    var fence_count : int
    var chemical_mode : bool
    var lb_count : int
}

func (lexer : &mut MdLexer) reset() {
    lexer.in_fenced_code = false;
    lexer.fence_char = '\0';
    lexer.fence_count = 0;
    lexer.chemical_mode = false;
    lexer.lb_count = 0;
}
