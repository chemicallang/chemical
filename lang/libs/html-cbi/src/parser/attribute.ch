import "@compiler/Parser.ch"
import "@compiler/ASTBuilder.ch"
import "@compiler/ChemicalTokenType.ch"
import "@compiler/ast/base/Value.ch"
import "@std/std.ch"
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

    printf("parsing attribute name %s\n", attr.name.data());
    fflush(null)

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

            const next = parser.getToken();

            if(next.type == ChemicalTokenType.CommaSym) {

                // multiple values

                var value = builder.allocate<ChemicalAttributeValues>()
                new (value) ChemicalAttributeValues {
                    AttributeValue : AttributeValue {
                        kind : AttributeValueKind.ChemicalValues
                    },
                    values : std::vector<*Value>()
                }

                value.values.push(expr);

                while(true) {
                    const got = parser.getToken();
                    if(got.type == ChemicalTokenType.CommaSym) {

                        parser.increment();

                        const expr2 = parser.parseExpression(builder);
                        if(expr2 != null) {

                            value.values.push(expr2)

                        } else {

                            printf("WHAT ::::: couldn't get expression\n")
                            fflush(null)
                            break;

                        }

                    } else {
                        printf("WHAT ::::: breaking at %d with value %s at line %d and char %d\n", got.type, got.value.data(), got.position.line, got.position.character)
                        fflush(null)
                        break;
                    }
                }

                const rb = parser.getToken();
                if(rb.type == ChemicalTokenType.RBrace) {
                    parser.increment();
                } else {
                    printf("WHAT ::::: type %d with value %s at line %d and char %d\n", next.type, next.value.data(), next.position.line, next.position.character)
                    fflush(null)
                    parser.error("expected a '}' after the multiple chemical expressions");
                }

                attr.value = value;

            } else {

                // single value

                if(next.type == TokenType.RBrace) {
                    parser.increment();
                } else {
                    parser.error("expected a '}' after the chemical expression");
                }

                var value = builder.allocate<ChemicalAttributeValue>()
                new (value) ChemicalAttributeValue {
                    AttributeValue : AttributeValue {
                        kind : AttributeValueKind.Chemical
                    },
                    value : expr
                }

                attr.value = value;

            }

        }
        default => {
            parser.error("expected a value after '=' for attribute");
            return null
        }
    }

    return attr;

}