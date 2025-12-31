public enum CSSLexerWhere {
    Selector,       ///< reading selector text
    Declaration,    ///< inside `{ … }` after a selector (handling both prop‑names and values)
    Value           ///< after `:` and before `;` or `}` (just for clarity, you could even merge this back)
}

/**
 * the lexer state is represented by this struct, which is created in initializeLexer function
 * this lexer must be able to encode itself into a 16 bit (short) integer
 */
public struct CSSLexer {

    /**
     * when other_mode is active it means, some other mode is active, we are lexing
     * chemical code or some other syntax that is part of html and requiring multiple
     * tokens to represent
     */
    var other_mode : bool

    /**
     * is lexing chemical code inside html using get embedded token
     */
    var chemical_mode : bool

    /**
     * this is an unsigned char, so it can be saved in 8 bits
     */
    var lb_count : uchar

    var at_rule : bool

    /**
     * this is state of the lexer which provides us position of the lexer
     */
    /**
     * lb count at the start of the chemical expression
     */
    var start_chemical_lb_count : uchar

    var where : CSSLexerWhere

}

func (lexer : &mut CSSLexer) reset() {
    lexer.other_mode = false;
    lexer.chemical_mode = false;
    lexer.lb_count = 0;
    lexer.where = CSSLexerWhere.Declaration
    lexer.at_rule = false;
}