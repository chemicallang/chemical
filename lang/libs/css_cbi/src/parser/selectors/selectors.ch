func (cssParser : &mut CSSParser) parseCSSSelector(
    parser : *mut Parser,
    builder : *mut ASTBuilder
) : *mut CSSSelector {

    const start = parser.getToken()
    switch(start.type) {
        TokenType.Dot => {
            parser.increment()
            const next = parser.getToken()
            if(next.type == TokenType.Identifier) {
                parser.increment()
                var s = builder.allocate<CSSNamedSelector>()
                new (s) CSSNamedSelector {
                    CSSSelector : CSSSelector { kind : CSSSelectorKind.Class },
                    identifier : builder.allocate_view(start.value)
                }
                return s;
            } else {
                parser.error("expected a identifier after '.' for class")
                return null
            }
        }
        TokenType.Hash => {
            parser.increment()
            const next = parser.getToken()
            if(next.type == TokenType.Identifier) {
                parser.increment()
                var s = builder.allocate<CSSNamedSelector>()
                new (s) CSSNamedSelector {
                    CSSSelector : CSSSelector { kind : CSSSelectorKind.Id },
                    identifier : builder.allocate_view(start.value)
                }
                return s;
            } else {
                parser.error("expected a identifier after '#' for id")
                return null
            }
        }
        TokenType.Identifier => {
            parser.increment()
            var s = builder.allocate<CSSNamedSelector>()
            new (s) CSSNamedSelector {
                CSSSelector : CSSSelector { kind : CSSSelectorKind.TagName },
                identifier : builder.allocate_view(start.value)
            }
            return s;
        }
        default => {
            parser.error("unexpected token, expected a css selector");
            return null
        }
    }

}