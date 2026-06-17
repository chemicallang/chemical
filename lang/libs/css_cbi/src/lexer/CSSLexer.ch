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

    /**
     * set to true when `{` enters chemical mode in a value context
     * used by the '{' handler to distinguish:
     *   - color: {expr}             → first { in value, enter chemical mode
     *   - color: {a} solid {b}      → has_chemical_in_value is true, so second { also enters chemical mode
     *   - :root{...}                → first { in value, but tokens_since_colon > 0, so selector block
     *   - div:hover{...}            → first { in value, but tokens_since_colon > 0, so selector block
     */
    var has_chemical_in_value : bool

    /**
     * tracks how many non-whitespace tokens were read since the last ':'
     * used together with has_chemical_in_value to decide whether { opens a
     * selector block (tokens_since_colon > 0 && !has_chemical_in_value)
     * or a chemical expression (tokens_since_colon == 0 || has_chemical_in_value)
     */
    var tokens_since_colon : uchar

    var where_state : CSSLexerWhere

}

func (lexer : &mut CSSLexer) reset() {
    lexer.other_mode = false;
    lexer.chemical_mode = false;
    lexer.lb_count = 0;
    lexer.where_state = CSSLexerWhere.Declaration
    lexer.at_rule = false;
    lexer.tokens_since_colon = 0
    lexer.has_chemical_in_value = false
}