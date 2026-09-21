/**
 * the lexer state is represented by this struct, which is created in initializeLexer function
 * this lexer must be able to encode itself into a 16 bit (short) integer
 */
public struct HtmlLexer {

    /**
     * has a less than symbol '<', which means we are lexing a identifier inside '<' identifier '>'
     */
    var has_lt : bool

    /**
     *  if this is true, lexed identifier is an attribute name
     */
    var lexed_tag_name : bool

    /**
     * is inside a comment (it has a comment start)
     */
    var is_comment : bool

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

    var paren_count : uchar

    var in_paren_expr : bool

    var last_token_was_if : bool

    var chem_start_lb : uchar

    var expecting_html_block : bool

    var after_chem_expr : bool

    /**
     * nesting depth of <pre> elements. Inside <pre>, whitespace-only text runs
     * between elements are significant and must be preserved as Text tokens.
     */
    var pre_depth : uchar

    /**
     * we just lexed the '</' of a closing tag, the next TagName token closes it
     */
    var in_end_tag : bool

    /**
     * the last opening tag name was "pre" (used to handle self-closing <pre/>)
     */
    var last_tag_pre : bool

    /**
     * when true, whitespace-only text runs between elements are significant
     * everywhere (not just inside <pre>) and are preserved as Text tokens.
     * The runtime html parser sets this for roundtrip fidelity; the #html
     * compile-time macro keeps it false (JSX-like insignificant whitespace).
     * Set once at lexer construction; reset() intentionally leaves it alone.
     */
    var preserve_whitespace : bool

    /**
     * the current opening tag was a <script>; set when the tag name is lexed,
     * and consumed when the matching '>' is lexed
     */
    var pending_script : bool

    /**
     * we are inside a <script> element; its content is lexed as a single raw
     * text token up to the matching </script> (JS is never lexed as HTML)
     */
    var in_script : bool

}

func (lexer : &mut HtmlLexer) reset() {
    lexer.has_lt = false;
    lexer.other_mode = false;
    lexer.chemical_mode = false;
    lexer.lb_count = 0;
    lexer.paren_count = 0;
    lexer.in_paren_expr = false;
    lexer.last_token_was_if = false;
    lexer.chem_start_lb = 0;
    lexer.expecting_html_block = false;
    lexer.after_chem_expr = false;
    lexer.in_end_tag = false;
    lexer.last_tag_pre = false;
    lexer.pre_depth = 0;
    lexer.pending_script = false;
    lexer.in_script = false;
}