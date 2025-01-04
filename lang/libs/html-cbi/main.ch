import "@compiler/Token.ch"
import "@compiler/Lexer.ch"
import "./TokenType.ch"
import "@compiler/Parser.ch"
import "@compiler/ASTBuilder.ch"
import "@cstd/ctype.ch"
import "@compiler/ChemicalTokenType.ch"
import "./ast/HtmlElement.ch"

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

public func fflush(stream : *mut any) : int

func (str : &std::string) view() : std::string_view {
    return std::string_view(str.data(), str.size());
}

func make_value_chain(builder : *mut ASTBuilder, value : *mut Value, len : size_t) : *mut AccessChain {
    const location = compiler::get_raw_location();
    const chain = builder.make_access_chain(false, location)
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
    chain_values.push(id)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    args.push(value)
    if(len != 0) {
        args.push(builder.make_number_value(len, location));
    }
    const new_chain = builder.make_access_chain(true, location)
    var new_chain_values = new_chain.get_values();
    new_chain_values.push(call);
    return new_chain;
}

func make_expr_chain_of(builder : *mut ASTBuilder, value : *mut Value) : *mut AccessChain {
    return make_value_chain(builder, value, 0);
}

func make_chain_of(builder : *mut ASTBuilder, str : &mut std::string) : *mut AccessChain {
    const location = compiler::get_raw_location();
    const value = builder.make_string_value(builder.allocate_view(str.view()), location)
    const size = str.size()
    str.clear();
    return make_value_chain(builder, value, size);
}

func put_chain_in(builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, str : &mut std::string) {
    const chain = make_chain_of(builder, str);
    const wrapped = builder.make_value_wrapper(chain, parent)
    vec.push(wrapped);
}

func put_value_chain_in(builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, value : *mut Value) {
    const chain = make_expr_chain_of(builder, value);
    const wrapped = builder.make_value_wrapper(chain, parent)
    vec.push(wrapped);
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
                return make_chain_of(builder, str);
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
            const chain = make_expr_chain_of(builder, parser.parseExpression(builder));
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

func convertHtmlAttribute(builder : *mut ASTBuilder, attr : *mut HtmlAttribute, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, str : &mut std::string) {
    str.append(' ')
    str.append_with_len(attr.name.data(), attr.name.size())
    if(attr.value != null) {
        str.append('=')
        switch(attr.value.kind) {
            AttributeValueKind.Text, AttributeValueKind.Number => {
                const value = attr.value as *mut TextAttributeValue
                str.append_with_len(value.text.data(), value.text.size())
            }
            AttributeValueKind.Chemical => {
                if(!str.empty()) {
                    put_chain_in(builder, vec, parent, str);
                }
                const value = attr.value as *mut ChemicalAttributeValue
                put_value_chain_in(builder, vec, parent, value)
            }
        }
    }
}

func convertHtmlChild(builder : *mut ASTBuilder, child : *mut HtmlChild, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, str : &mut std::string) {
    switch(child.kind) {
        HtmlChildKind.Text => {
            var text = child as *mut HtmlText
            str.append_with_len(text.value.data(), text.value.size());
        }
        HtmlChildKind.Element => {
            var element = child as *mut HtmlElement
            str.append('<')
            str.append_with_len(element.name.data(), element.name.size())

            // putting attributes
            var a = 0;
            var attrs = element.attributes.size()
            while(a < attrs) {
                var attr = element.attributes.get(a)
                convertHtmlAttribute(builder, attr, vec, parent, str);
                a++
            }

            str.append('>')

            // doing children
            var i = 0;
            var s = element.children.size();
            while(i < s) {
                var nested_child = element.children.get(i)
                convertHtmlChild(builder, nested_child, vec, parent, str)
                i++;
            }

            str.append('<')
            str.append('/')
            str.append_with_len(element.name.data(), element.name.size())
            str.append('>')
        }
        HtmlChildKind.ChemicalValue => {
            if(!str.empty()) {
                put_chain_in(builder, vec, parent, str);
            }
            const chem_child = child as *mut HtmlChemValueChild
            put_value_chain_in(builder, vec, parent, chem_child.value)
        }
    }
}

func convertHtmlRoot(builder : *mut ASTBuilder, root : *mut HtmlRoot, vec : *mut VecRef<ASTNode>, str : &mut std::string) {
    convertHtmlChild(builder, root.element, vec, root.parent, str);
    if(!str.empty()) {
        put_chain_in(builder, vec, root.parent, str);
    }
}

func parseAttribute(parser : *mut Parser, builder : *mut ASTBuilder) : *mut HtmlAttribute {

    const id = parser.getToken();
    if(id.type != TokenType.Identifier) {
        return null;
    }

    parser.increment();

    var attr = builder.allocate<HtmlAttribute>();
    new (attr) HtmlAttribute {
        name : builder.allocate_view(id.value),
        value : null
    }

    const equal = parser.getToken();
    if(equal.type != TokenType.Equal) {
        return attr;
    }

    parser.increment();

    const next = parser.getToken();

    switch(next.type) {
        TokenType.SingleQuotedValue, TokenType.DoubleQuotedValue, TokenType.Number => {
            parser.increment();
            var value = builder.allocate<TextAttributeValue>()
            new (value) TextAttributeValue {
                AttributeValue : AttributeValue {
                    kind : AttributeValueKind.Text
                },
                text : builder.allocate_view(next.value)
            }
            attr.value = value;
        }
        TokenType.LBrace => {

            parser.increment();

            var expr = parser.parseExpression(builder);
            if(expr == null) {
                parser.error("expected a expression value after '{'");
            }

            var value = builder.allocate<ChemicalAttributeValue>()
            new (value) ChemicalAttributeValue {
                AttributeValue : AttributeValue {
                    kind : AttributeValueKind.Chemical
                },
                value : expr
            }

            const rb = parser.getToken();
            if(rb.type == TokenType.RBrace) {
                parser.increment();
            } else {
                parser.error("expected a '}' after the chemical expression");
            }

            attr.value = value;

        }
        default => {
            parser.error("expected a value after '=' for attribute");
            return null
        }
    }

    return attr;

}

func parseElementChild(parser : *mut Parser, builder : *mut ASTBuilder) : *mut HtmlChild {

    const current = parser.getToken();

    printf("parsing element child at %d, %d\n", current.position.line, current.position.character)
    fflush(null)

    if(current.type == TokenType.LessThan) {

        parser.increment();
        const next = parser.getToken();
        if(next.type == TokenType.Identifier) {
            parser.setToken(current);
            return parseElement(parser, builder);
        } else if(next.type == TokenType.FwdSlash) {
            parser.setToken(current);
            return null;
        } else {
            parser.error("unknown symbol, expected text or element");
            return null;
        }

    } else if(current.type == TokenType.LBrace) {

        printf("parsing chemical value in text\n", current.value.data())
        fflush(null)

        parser.increment();

        var value_child = builder.allocate<HtmlChemValueChild>();
        new (value_child) HtmlChemValueChild {
            HtmlChild : HtmlChild {
                kind : HtmlChildKind.ChemicalValue
            },
            value : null
        }

        const expr = parser.parseExpression(builder)
        if(expr != null) {
            value_child.value = expr;
        } else {
            parser.error("expected a value for html child");
        }

        const next = parser.getToken();
        if(next.type == ChemicalTokenType.RBrace) {
            parser.increment();
        } else {
            printf("boo has error %s\n", parser.getToken().value.data())
            fflush(null)
            parser.error("expected a rbrace after the chemical value")
        }

        return value_child;
    } else if(current.type == TokenType.Text) {

        printf("parsing text with value %s\n", current.value.data())
        fflush(null)

        parser.increment();

        var text = builder.allocate<HtmlText>();
        new (text) HtmlText {
            HtmlChild : HtmlChild {
                kind : HtmlChildKind.Text
            },
            value : builder.allocate_view(current.value)
        }
        return text;
    } else {
        parser.error("unknown symbol, expected text or element");
        return null;
    }

}

func parseElement(parser : *mut Parser, builder : *mut ASTBuilder) : *mut HtmlElement {

    const lt = parser.getToken();

    printf("parsing element at %d, %d\n", lt.position.line, lt.position.character)
    fflush(null)

    if(lt.type == TokenType.LessThan) {

        parser.increment();
        const id = parser.getToken();
        if(id.type != TokenType.Identifier) {
            parser.error("expected an identifier after '<'");
            return null
        }
        parser.increment();

        printf("size of HtmlElement is %d\n", #sizeof(HtmlElement))
        fflush(null)

        var element : *mut HtmlElement = builder.allocate<HtmlElement>();
        new (element) HtmlElement {
            HtmlChild : HtmlChild {
                kind : HtmlChildKind.Element
            },
            name : builder.allocate_view(id.value),
            attributes : std::vector<*HtmlAttribute>(),
            children : std::vector<*HtmlChild>()
        }

        printf("let's checkout size of the children : %d\n", element.children.size())

        printf("parsing attributes for %s\n", id.value.data());
        fflush(null)

        while(true) {
            var attr = parseAttribute(parser, builder);
            if(attr != null) {
                element.attributes.push(attr)
            } else {
                break;
            }
        }

        const gt = parser.getToken();
        if(gt.type != TokenType.GreaterThan) {
            parser.error("expected a greater than sign '>' after the identifier");
        } else {
            parser.increment();
        }

        printf("let's checkout size of the children : %d\n", element.children.size())
        printf("parsing children for %s\n", id.value.data());
        fflush(null)

        printf("let's checkout size of the children : %d\n", element.children.size())
        while(true) {
            var child = parseElementChild(parser, builder);
            printf("checking received element child\n");
            fflush(null)
            if(child != null) {
                printf("adding received element child\n");
                fflush(null)
                printf("let's checkout size of the children : %d\n", element.children.size())
                element.children.push(child)
                printf("added received element child\n");
                fflush(null)
            } else {
                printf("received element child is null\n");
                fflush(null)
                break;
            }
        }

        printf("parsing end of element %s\n", id.value.data())
        fflush(null)

        const last_lt = parser.getToken();
        if(last_lt.type == TokenType.LessThan) {
            parser.increment();
            const fwd = parser.getToken();
            if(fwd.type == TokenType.FwdSlash) {
                parser.increment();
                const last_id = parser.getToken();
                if(last_id.type == TokenType.Identifier) {
                    parser.increment();
                    if(strncmp(last_id.value.data(), id.value.data(), id.value.size()) != 0) {
                        parser.error("expected correct identifier for ending tag");
                    }
                    const last_gt = parser.getToken();
                    if(last_gt.type == TokenType.GreaterThan) {
                        parser.increment();
                    } else {
                        parser.error("expected '>' after the ending tag");
                    }
                } else {
                    parser.error("expected identifier after the '</'");
                }
            } else {
                parser.error("expected a '</' for ending the element");
            }
        } else {
            parser.error("expected a '</' for ending the element");
        }

        return element;

    } else {
        return null;
    }
}

func parseHtmlRoot(parser : *mut Parser, builder : *mut ASTBuilder) : *HtmlRoot {
    var rootElement = parseElement(parser, builder);
    if(rootElement == null) {
        parser.error("expected a root element for #html");
        return null;
    } else {
        var root = builder.allocate<HtmlRoot>()
        new (root) HtmlRoot {
            element : rootElement,
            parent : parser.getParentNode()
        }
        return root;
    }
}

public func parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
    printf("wow create macro node\n");
    const loc = compiler::get_raw_location();
    if(parser.increment_if(TokenType.LBrace)) {
        var scope = builder.make_scope(parser.getParentNode(), loc);
        var scope_nodes = scope.getNodes();
        var str = std::string();
        var root = parseHtmlRoot(parser, builder);
        printf("parsed to html root\n")
        fflush(null)
        convertHtmlRoot(builder, root, scope_nodes, str);
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