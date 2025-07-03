
func parseElementChild(parser : *mut Parser, builder : *mut ASTBuilder) : *mut HtmlChild {

    const current = parser.getToken();

    printf("parsing element child at %d, %d\n", current.position.line, current.position.character)
    fflush(null)

    if(current.type == TokenType.LessThan) {

        parser.increment();
        const next = parser.getToken();
        if(next.type == TokenType.TagName) {
            parser.setToken(current);
            return parseElement(parser, builder);
        } else {
            parser.error("unknown symbol, expected text or element");
            return null;
        }

    } else if(current.type == TokenType.TagEnd) {

        return null;

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
    } else if(current.type == TokenType.CommentStart) {

        parser.increment()
        // here we have a comment
        // currently we do not allow chemical values inside comments
        // though this can change

        var start_token = parser.getToken()
        var tok = parser.getToken()

        // lets first calculate total size of the comment text
        var size : uint = 0;
        while(tok.type == TokenType.CommentText) {
            size += tok.value.size()
            tok++
        }

        // lets allocate this string on allocator
        const str_ptr = builder.allocate_str_size(size)

        // reset back the token, we'll traverse again
        tok = start_token;

        // lets copy the comment text into the pointer
        var curr_str = str_ptr;
        while(tok.type == TokenType.CommentText) {
            memcpy(curr_str, tok.value.data(), tok.value.size())
            curr_str += tok.value.size()
            tok++
        }

        // we have space for the last terminator (allocate_str_size)
        *curr_str = '\0'

        // lets set to current token
        // so parser can handle the next token instead of comment tokens
        parser.setToken(tok)

        // allocating final comment ast
        var comment : *mut HtmlComment = builder.allocate<HtmlComment>();
        new (comment) HtmlComment {
            HtmlChild : HtmlChild {
                kind : HtmlChildKind.Comment
            },
            value : std::string_view(str_ptr, size)
        }

        return comment;

    } else if(current.type == TokenType.DeclarationStart) {
     // TODO handle this case
     return null;
    } else {
        parser.error("unknown symbol, expected text or element");
        return null;
    }

}