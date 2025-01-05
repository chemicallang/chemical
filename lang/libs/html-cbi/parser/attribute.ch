import "@compiler/Parser.ch"
import "@compiler/ASTBuilder.ch"
import "../ast/HtmlAttribute.ch"

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