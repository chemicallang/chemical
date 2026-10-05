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

    /**
     * we are inside an explicit "@(expr)" value. The expression is read by the
     * chemical lexer and ends at the matching ')' rather than at a '}', so it
     * is tracked with paren_count instead of lb_count and never disturbs the
     * brace depth -- which is what lets "@( ... )" work inside <pre>, where a
     * '}' is a literal character.
     */
    var in_paren_value : bool

    var last_token_was_if : bool

    var chem_start_lb : uchar

    var expecting_html_block : bool

    var after_chem_expr : bool

    /**
     * nesting depth of <pre> elements. Inside <pre>, whitespace-only text runs
     * between elements are significant and must be preserved as Text tokens,
     * and '{' / '}' are ordinary text rather than chemical syntax.
     */
    var pre_depth : uchar

    /**
     * how many '{' that were read as literal text inside <pre> are still
     * unmatched. A '}' in <pre> closes one of these when the counter is above
     * zero (it is a literal brace) and is a real chemical block/expression
     * close when it is zero. This is what lets a <pre> block contain both
     * literal code braces and an @if/@else html block: the literal braces pair
     * with each other and never disturb lb_count.
     */
    var pre_brace_depth : uchar

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
     * the `type` attribute of the current opening tag was seen and its value
     * said the content is NOT script data, so this <script> is a data block
     * (application/json, text/template, ...) whose content takes the ordinary
     * text/interpolation path instead of the raw-text one. Only meaningful
     * while pending_script is set.
     */
    var pending_script_is_data : bool

    /**
     * the attribute name most recently lexed was "type"; the next quoted value
     * token is that attribute's value. Attributes arrive as separate tokens
     * (AttrName, '=', quoted value), so the pairing is tracked here.
     */
    var attr_name_is_type : bool

    /**
     * we are inside a <script> element whose content is lexed as a single raw
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
    lexer.in_paren_value = false;
    lexer.last_token_was_if = false;
    lexer.chem_start_lb = 0;
    lexer.expecting_html_block = false;
    lexer.after_chem_expr = false;
    lexer.in_end_tag = false;
    lexer.last_tag_pre = false;
    lexer.pre_depth = 0;
    lexer.pre_brace_depth = 0;
    lexer.pending_script = false;
    lexer.pending_script_is_data = false;
    lexer.attr_name_is_type = false;
    lexer.in_script = false;
}