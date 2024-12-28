import "@compiler/Token.ch"
import "@compiler/Lexer.ch"
import "./TokenType.ch"
import "@compiler/Parser.ch"
import "@compiler/ASTBuilder.ch"
import "@cstd/ctype.ch"
import "@compiler/ChemicalTokenType.ch"

using namespace std;

/**
 * the lexer state is represented by this struct, which is created in initializeLexer function
 * this lexer must be able to encode itself into a 16 bit (short) integer
 */
struct HtmlLexer {

    /**
     * has a less than symbol '<', which means we are lexing a identifier inside '<' identifier '>'
     */
    var has_lt : bool

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

}

func (lexer : &mut HtmlLexer) reset() {
    lexer.has_lt = false;
    lexer.other_mode = false;
    lexer.chemical_mode = false;
    lexer.lb_count = 0;
}

public func parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
    printf("wow create macro value\n");
    const loc = compiler::get_raw_location();
    if(parser.increment_if(TokenType.LBrace)) {
        const value = builder.make_int_value(10, loc);
        if(!parser.increment_if(TokenType.RBrace)) {
            parser.error("expected a rbrace");
        }
        return value;
    } else {
        parser.error("expected a lbrace");
    }
    return null;
}

func (str : &std::string) view() : std::string_view {
    return std::string_view(str.data(), str.size());
}

func make_value_chain(parser : *mut Parser, builder : *mut ASTBuilder, value : *mut Value, len : size_t) : *mut AccessChain {
    const location = compiler::get_raw_location();
    const chain = builder.make_access_chain(true, location)
    var chain_values = chain.get_values()
    var base = builder.make_identifier(std::string_view("html"), false, location);
    chain_values.push(base)
    var name : std::string_view
    if(len == 0) {
        name = std::string_view("append_char_ptr")
    } else {
        name = std::string_view("append_with_len")
    }
    var id = builder.make_identifier(name, false, location);
    chain_values.push(id);
    var call = builder.make_function_call_value(location)
    var args = call.get_args();
    args.push(value)
    if(len != 0) {
        args.push(builder.make_number_value(len, location));
    }
    chain_values.push(call)
    return chain;
}

func make_expr_chain_of(parser : *mut Parser, builder : *mut ASTBuilder, value : *mut Value) : *mut AccessChain {
    return make_value_chain(parser, builder, value, 0);
}

func make_chain_of(parser : *mut Parser, builder : *mut ASTBuilder, str : &mut std::string) : *mut AccessChain {
    const location = compiler::get_raw_location();
    const value = builder.make_string_value(builder.allocate_view(str.view()), location)
    const size = str.size()
    str.clear();
    return make_value_chain(parser, builder, value, size);
}

func rtrim_len(str : *mut char, len : size_t) : size_t {
    // Start from the end of the string
    while (len > 0 && isspace(str[len - 1])) {
        len--;
    }
    return len;
}

func parseTextChain(parser : *mut Parser, builder : *mut ASTBuilder, str : &mut std::string) : *mut AccessChain {
    while(true) {
        const token = parser.getToken();
        switch(token.type) {
            TokenType.Identifier, TokenType.SingleQuotedValue, TokenType.DoubleQuotedValue, TokenType.Number => {
                parser.increment();
                var next = parser.getToken();
                if(next.type == TokenType.RBrace) {
                    // last brace, we need to rtrim it
                    str.append_with_len(token.value.data(), rtrim_len(token.value.data(), token.value.size()));
                } else if(next.type == TokenType.Identifier) {
                    str.append_with_len(token.value.data(), token.value.size());
                    str.append(' ')
                } else {
                    str.append_with_len(token.value.data(), token.value.size());
                }
            }
            TokenType.Text, TokenType.LessThan, TokenType.GreaterThan, TokenType.FwdSlash, TokenType.Equal => {
                parser.increment();
                if(parser.getToken().type == TokenType.RBrace) {
                    // last brace, we need to rtrim it
                    str.append_with_len(token.value.data(), rtrim_len(token.value.data(), token.value.size()));
                } else {
                    str.append_with_len(token.value.data(), token.value.size());
                }
            }
            default => {
                return make_chain_of(parser, builder, str);
            }
        }
    }
}

func parseTextChainOrChemExpr(parser : *mut Parser, builder : *mut ASTBuilder, str : &mut std::string) : *mut AccessChain {
    const token = parser.getToken();
    switch(token.type) {
        TokenType.Identifier, TokenType.Text, TokenType.LessThan, TokenType.GreaterThan, TokenType.FwdSlash, TokenType.Equal, TokenType.SingleQuotedValue, TokenType.DoubleQuotedValue, TokenType.Number => {
            return parseTextChain(parser, builder, str);
        }
        TokenType.LBrace => {
            parser.increment();
            const chain = make_expr_chain_of(parser, builder, parser.parseExpression(builder));
            const next = parser.getToken();
            if(next.type == ChemicalTokenType.RBrace) {
                parser.increment()
            } else {
                parser.error("expected a rbrace after chemical expression");
            }
            return chain;
        }
        TokenType.RBrace, TokenType.EndOfFile, TokenType.Unexpected, default => {
            unsafe {
                return null;
            }
        }
    }
}

public func parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
    printf("wow create macro node\n");
    const loc = compiler::get_raw_location();
    if(parser.increment_if(TokenType.LBrace)) {
        var scope = builder.make_scope(parser.getParentNode(), loc);
        var scope_nodes = scope.getNodes();
        var str = std::string();
        while(true) {
            var chain = parseTextChainOrChemExpr(parser, builder, str);
            if(chain != null) {
                const wrapped = builder.make_value_wrapper(chain, parser.getParentNode())
                scope_nodes.push(wrapped);
            } else {
                break;
            }
        }
        if(!parser.increment_if(TokenType.RBrace)) {
            parser.error("expected a rbrace for ending the html macro");
        }
        return scope;
    } else {
        parser.error("expected a lbrace");
    }
}

func (provider : &SourceProvider) read_tag_name(str : &SerialStrAllocator) : std::string_view {
    while(true) {
        const c = provider.peek();
        if(c != -1 && (isalnum(c as int) || c == '_' || c == '-' || c == ':')) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
    return str.finalize_view();
}

func (provider : &SourceProvider) read_text(str : &SerialStrAllocator) : std::string_view {
    while(true) {
        const c = provider.peek();
        if(c != -1 && c != '<' && c != '{' && c != '}') {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
    return str.finalize_view();
}

func (provider : &SourceProvider) read_single_quoted_value(str : &SerialStrAllocator) : std::string_view {
    while(true) {
        const c = provider.peek();
        if (c == '\'') {
            str.append(provider.readCharacter());
            break;
        } else if(c != -1) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
    return str.finalize_view();
}

func (provider : &SourceProvider) read_double_quoted_value(str : &SerialStrAllocator) : std::string_view {
    while(true) {
        const c = provider.peek();
        if (c == '"') {
            str.append(provider.readCharacter());
            break;
        } else if(c != -1) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
    return str.finalize_view();
}

// read digits into the string
func (provider : &mut SourceProvider) read_digits(str : &mut SerialStrAllocator) {
    while(true) {
        const next = provider.peek();
        if(isdigit(next)) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
}

// assumes that a digit exists at current location
func (provider : &mut SourceProvider) read_floating_digits(str : &mut SerialStrAllocator) : bool {
    provider.read_digits(str);
    const c = provider.peek();
    if(c == '.') {
        str.append(provider.readCharacter());
        provider.read_digits(str);
        return true;
    } else {
        return false;
    }
}

func (provider : &SourceProvider) skip_whitespaces() {
    while(true) {
        const c = provider.peek();
        switch(c) {
            ' ', '\t', '\n', '\r' => {
                provider.readCharacter();
            }
            default => {
                return;
            }
        }
    }
}

@comptime
func view(str : literal<string>) : std::string_view {
    return std::string_view(str);
}

public func getNextToken2(html : &mut HtmlLexer, lexer : &mut Lexer) : Token {
    const provider = &lexer.provider;
    const str = &lexer.str;
    // the position of the current symbol
    const position = provider.getPosition();
    const c = provider.readCharacter();
    printf("reading character : %d\n", c);
    switch(c) {
        '<' => {
            html.has_lt = true;
            return Token {
                type : TokenType.LessThan,
                value : view("<"),
                position : position
            }
        }
        '}' => {
            if(html.lb_count == 1) {
                printf("exiting, didn't expect an rb\n");
                html.reset();
                lexer.user_mode = false;
                lexer.other_mode = false;
            } else {
                html.lb_count--;
                printf("lb_count decreased to %d\n", html.lb_count);
            }
            return Token {
                type : TokenType.RBrace,
                value : view("}"),
                position : position
            }
        }
        '{' => {
            // here always lb_count > 0
            if(html.lb_count == 1) {
                printf("turning on chemical mode\n");
                html.other_mode = true;
                html.chemical_mode = true;
            }
            html.lb_count++;
            printf("lb_count increased to %d\n", html.lb_count);
            return Token {
                type : TokenType.LBrace,
                value : view("{"),
                position : position
            }
        }
        ' ', '\t', '\n', '\r' => {
            provider.skip_whitespaces();
            return getNextToken2(html, lexer);
        }
        '/' => {
            if(html.has_lt) {
                return Token {
                    type : TokenType.FwdSlash,
                    value : view("/"),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.Unexpected,
                    value : view("lt is not open"),
                    position : position
                }
            }
        }
        '>' => {
            if(html.has_lt) {
                html.has_lt = false;
                return Token {
                    type : TokenType.GreaterThan,
                    value : view(">"),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.Unexpected,
                    value : view("lt is not open"),
                    position : position
                }
            }
        }
        default => {
            if(html.has_lt) {
                if(isalpha(c as int)) {
                    str.append(c);
                    const tag_name = provider.read_tag_name(lexer.str);
                    return Token {
                        type : TokenType.Identifier,
                        value : tag_name,
                        position : position
                    }
                } else {
                    switch(c) {
                        '=' => {
                           return Token {
                                type : TokenType.Equal,
                                value : view("="),
                                position : position
                           }
                        }
                        '\'' => {
                            str.append('\'');
                            return Token {
                                type : TokenType.SingleQuotedValue,
                                value : provider.read_single_quoted_value(lexer.str),
                                position : position
                            }
                        }
                        '"' => {
                            str.append('"')
                            return Token {
                                type : TokenType.DoubleQuotedValue,
                                value : provider.read_double_quoted_value(lexer.str),
                                position : position
                            }
                        }
                        default => {
                            if(isdigit(c)) {
                                str.append(c);
                                provider.read_floating_digits(lexer.str);
                                return Token {
                                    type : TokenType.Number,
                                    value : str.finalize_view(),
                                    position : position
                                }
                            } else {
                                return Token {
                                    type : TokenType.Unexpected,
                                    value : view("tag names must start with letters"),
                                    position : position
                                }
                            }
                        }
                    }
                }
            } else {
                printf("starting text with character : %d\n", c);
                str.append(c);
                var text = provider.read_text(lexer.str)
                if(text.size() != 0) {
                    return Token {
                        type : TokenType.Text,
                        value : text,
                        position : position
                    }
                }
            }
        }
    }
}

public func getNextToken(html : &mut HtmlLexer, lexer : &mut Lexer) : Token {
    if(html.other_mode) {
        if(html.chemical_mode) {
            var nested = lexer.getEmbeddedToken();
            if(nested.type == ChemicalTokenType.LBrace) {
                html.lb_count++;
                printf("lb_count increases to %d in chemical mode\n", html.lb_count);
            } else if(nested.type == ChemicalTokenType.RBrace) {
                html.lb_count--;
                printf("lb_count decreased to %d in chemical mode\n", html.lb_count);
                if(html.lb_count == 1) {
                    html.other_mode = false;
                    html.chemical_mode = false;
                    printf("since lb_count decreased to 1, we're switching to html mode\n");
                }
            }
            printf("in chemical mode, created token '%s'\n", nested.value.data());
            return nested;
        }
    }
    const t = getNextToken2(html, lexer);
    printf("I created token : '%s' with type %d\n", t.value.data(), t.type);
    return t;
}

public func initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(#sizeof(HtmlLexer), #alignof(HtmlLexer)) as *mut HtmlLexer;
    const x = new (ptr) HtmlLexer {
        has_lt : false,
        other_mode : false,
        chemical_mode : false,
        lb_count : 0
    }
    lexer.other_mode = true;
    lexer.user_mode = true;
    lexer.user_lexer = UserLexerFn {
        instance : ptr,
        subroutine : getNextToken
    }
}