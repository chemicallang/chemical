
func (htmlParser : &mut HtmlParser) parseAttribute(parser : *mut Parser, builder : *mut ASTBuilder) : *mut HtmlAttribute {

    const id = parser.getToken();
    if(id.type != TokenType.AttrName) {
        return null;
    }

    parser.increment();

    var attr = builder.allocate<HtmlAttribute>();
    new (attr) HtmlAttribute {
        name : builder.allocate_view(&id.value),
        value : null,
        loc : parser.getEncodedLocation(id)
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
                text : builder.allocate_view(&next.value)
            }
            attr.value = value;
        }
        TokenType.LBrace => {

            parser.increment();

            var expr = parser.parseExpressionOrArrayOrStruct(builder);
            if(expr == null) {
                parser.error("expected a expression value after '{'");
            } else {
                htmlParser.dyn_values.push(expr)
            }

            const next2 = parser.getToken();

            if(next2.type == ChemicalTokenType.CommaSym) {

                // multiple values

                var value = builder.allocate<ChemicalAttributeValues>()
                new (value) ChemicalAttributeValues {
                    AttributeValue : AttributeValue {
                        kind : AttributeValueKind.ChemicalValues
                    },
                    values : std::vector<*mut Value>()
                }

                value.values.push(expr);

                while(true) {
                    const got = parser.getToken();
                    if(got.type == ChemicalTokenType.CommaSym) {

                        parser.increment();

                        const expr2 = parser.parseExpressionOrArrayOrStruct(builder);
                        if(expr2 != null) {

                            value.values.push(expr2)
                            htmlParser.dyn_values.push(expr2)

                        } else {

                            parser.error("expected a chemical expression value");
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
                    printf("WHAT ::::: type %d with value %s at line %d and char %d\n", next2.type, next2.value.data(), next2.position.line, next2.position.character)
                    fflush(null)
                    parser.error("expected a '}' after the multiple chemical expressions");
                }

                attr.value = value;

            } else {

                // single value

                if(next2.type == ChemicalTokenType.RBrace) {
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
        TokenType.ChemicalValueStart => {

            // "@(expr)" in an attribute value. Same as the "{expr}" form below
            // but closed by ')' instead of '}', and it also accepts a
            // comma-separated list so that "@(a, b)" behaves exactly like
            // "{a, b}".
            parser.increment();

            var first = parser.parseExpressionOrArrayOrStruct(builder);
            if(first == null) {
                parser.error("expected a expression value after '@('");
                return null;
            }
            htmlParser.dyn_values.push(first)

            const sep = parser.getToken();

            if(sep.type == ChemicalTokenType.CommaSym) {

                var many = builder.allocate<ChemicalAttributeValues>()
                new (many) ChemicalAttributeValues {
                    AttributeValue : AttributeValue {
                        kind : AttributeValueKind.ChemicalValues
                    },
                    values : std::vector<*mut Value>()
                }

                many.values.push(first);

                while(true) {
                    const got = parser.getToken();
                    if(got.type != ChemicalTokenType.CommaSym) {
                        break;
                    }
                    parser.increment();
                    const expr = parser.parseExpressionOrArrayOrStruct(builder);
                    if(expr == null) {
                        parser.error("expected a chemical expression value");
                        break;
                    }
                    many.values.push(expr)
                    htmlParser.dyn_values.push(expr)
                }

                const close = parser.getToken();
                if(close.type == ChemicalTokenType.RParen) {
                    parser.increment();
                } else {
                    parser.error("expected a ')' after the multiple chemical expressions");
                }

                attr.value = many;

            } else {

                if(sep.type == ChemicalTokenType.RParen) {
                    parser.increment();
                } else {
                    parser.error("expected a ')' after the chemical expression");
                }

                var single = builder.allocate<ChemicalAttributeValue>()
                new (single) ChemicalAttributeValue {
                    AttributeValue : AttributeValue {
                        kind : AttributeValueKind.Chemical
                    },
                    value : first
                }

                attr.value = single;

            }

        }
        default => {
            parser.error("expected a value after '=' for attribute");
            return null
        }
    }

    return attr;

}