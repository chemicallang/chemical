
func (htmlParser : &mut HtmlParser) parseElementChild(parser : *mut Parser, builder : *mut ASTBuilder) : *mut HtmlChild {

    const current = parser.getToken();

    if(current.type == TokenType.LessThan) {

        parser.increment();
        const next = parser.getToken();
        if(next.type == TokenType.TagName) {
            parser.setToken(current);
            return htmlParser.parseElement(parser, builder);
        } else {
            parser.error("unknown symbol, expected text or element");
            return null;
        }

    } else if(current.type == TokenType.TagEnd) {

        return null;

    } else if(current.type == TokenType.LBrace) {

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
            htmlParser.dyn_values.push(expr)
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
    } else if(current.type == TokenType.If) {

        return htmlParser.parseIfStatement(parser, builder);

    } else if(current.type == TokenType.Text) {

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
        return null;
    }

}

func (htmlParser : &mut HtmlParser) parseIfStatement(parser : *mut Parser, builder : *mut ASTBuilder) : *mut HtmlChild {
    const loc = intrinsics::get_raw_location();
    parser.increment(); // skip 'if'

    var ifstmt = builder.allocate<HtmlIfStatement>();
    new (ifstmt) HtmlIfStatement {
        HtmlChild : HtmlChild {
            kind : HtmlChildKind.IfStatement
        },
        condition : null,
        body : std::vector<*mut HtmlChild>(),
        else_ifs : std::vector<*mut HtmlElseIf>(),
        else_body : std::vector<*mut HtmlChild>()
    }

    if(!parser.increment_if(ChemicalTokenType.LParen as int)) {
        parser.error("expected '(' after 'if'");
    }

    const expr = parser.parseExpression(builder);
    if(expr == null) {
        parser.error("expected a chemical expression");
    } else {
        ifstmt.condition = expr;
        htmlParser.dyn_values.push(expr)
    }

    if(!parser.increment_if(ChemicalTokenType.RParen as int)) {
        parser.error("expected ')' after if condition");
    }

    if(!parser.increment_if(TokenType.LBrace as int)) {
        parser.error("expected '{' after if condition");
    }

    while(true) {
        var child = htmlParser.parseElementChild(parser, builder);
        if(child != null) {
            ifstmt.body.push(child);
        } else {
            break;
        }
    }

    if(!parser.increment_if(TokenType.RBrace as int)) {
        parser.error("expected '}' after if body");
    }

    while(parser.getToken().type == TokenType.Else) {
        parser.increment(); // skip 'else'
        if(parser.getToken().type == TokenType.If) {
            parser.increment(); // skip 'if'
            var elseif = builder.allocate<HtmlElseIf>();
            new (elseif) HtmlElseIf {
                condition : null,
                body : std::vector<*mut HtmlChild>()
            }

            if(!parser.increment_if(ChemicalTokenType.LParen as int)) {
                parser.error("expected '(' after 'else if'");
            }

            const expr = parser.parseExpression(builder);
            if(expr == null) {
                parser.error("expected a chemical expression");
            } else {
                elseif.condition = expr;
                htmlParser.dyn_values.push(expr)
            }

            if(!parser.increment_if(ChemicalTokenType.RParen as int)) {
                parser.error("expected ')' after else if condition");
            }

            if(!parser.increment_if(TokenType.LBrace as int)) {
                parser.error("expected '{' after if condition");
            }

            while(true) {
                var child = htmlParser.parseElementChild(parser, builder);
                if(child != null) {
                    elseif.body.push(child);
                } else {
                    break;
                }
            }

            if(!parser.increment_if(TokenType.RBrace as int)) {
                parser.error("expected '}' after else if body");
            }

            ifstmt.else_ifs.push(elseif);
        } else {
            if(!parser.increment_if(TokenType.LBrace as int)) {
                parser.error("expected '{' after 'else'");
            }

            while(true) {
                var child = htmlParser.parseElementChild(parser, builder);
                if(child != null) {
                    ifstmt.else_body.push(child);
                } else {
                    break;
                }
            }

            if(!parser.increment_if(TokenType.RBrace as int)) {
                parser.error("expected '}' after else body");
            }
            break; // else is the last block
        }
    }

    return ifstmt;
}
