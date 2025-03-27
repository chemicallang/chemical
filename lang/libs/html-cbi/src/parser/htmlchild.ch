import "./element.ch"

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