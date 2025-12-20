func (cssParser : &mut CSSParser) parseCalcFactor(parser : *mut Parser, builder : *mut ASTBuilder) : CSSCalcExpression {
    const token = parser.getToken();
    if (token.type == TokenType.LParen) {
        parser.increment();
        var expr = cssParser.parseCalcExpression(parser, builder);
        if (parser.getToken().type == TokenType.RParen) {
            parser.increment();
        } else {
            parser.error("expected ')' in calc");
        }
        return expr;
    }

    var value = CSSValue();
    if (cssParser.parseNumberOrLength(parser, builder, value)) {
        var literal = builder.allocate<CSSValue>();
        literal.kind = value.kind;
        literal.data = value.data;
        return CSSCalcExpression {
            kind : CSSCalcExpressionKind.Literal,
            data : literal
        };
    }

    parser.error("unexpected token in calc expression");
    return CSSCalcExpression { kind : CSSCalcExpressionKind.Literal, data : null };
}

func (cssParser : &mut CSSParser) parseCalcTerm(parser : *mut Parser, builder : *mut ASTBuilder) : CSSCalcExpression {
    var left = cssParser.parseCalcFactor(parser, builder);
    while (true) {
        const token = parser.getToken();
        if (token.type == TokenType.Multiply || token.type == TokenType.Divide) {
            const op = if(token.type == TokenType.Multiply) '*' else '/';
            parser.increment();
            var right = cssParser.parseCalcFactor(parser, builder);
            var opData = builder.allocate<CSSCalcOperationData>();
            new (opData) CSSCalcOperationData {
                left : left,
                right : right,
                op : op
            };
            left = CSSCalcExpression {
                kind : CSSCalcExpressionKind.Operation,
                data : opData
            };
        } else {
            break;
        }
    }
    return left;
}

func (cssParser : &mut CSSParser) parseCalcExpression(parser : *mut Parser, builder : *mut ASTBuilder) : CSSCalcExpression {
    var left = cssParser.parseCalcTerm(parser, builder);
    while (true) {
        const token = parser.getToken();
        if (token.type == TokenType.Plus || token.type == TokenType.Minus) {
            const op = if(token.type == TokenType.Plus) '+' else '-';
            parser.increment();
            var right = cssParser.parseCalcTerm(parser, builder);
            var opData = builder.allocate<CSSCalcOperationData>();
            new (opData) CSSCalcOperationData {
                left : left,
                right : right,
                op : op
            };
            left = CSSCalcExpression {
                kind : CSSCalcExpressionKind.Operation,
                data : opData
            };
        } else {
            break;
        }
    }
    return left;
}

func (cssParser : &mut CSSParser) parseCalc(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    if (parser.getToken().type == TokenType.LParen) {
        parser.increment();
    } else {
        parser.error("expected '(' after calc");
    }

    const expr = cssParser.parseCalcExpression(parser, builder);

    if (parser.getToken().type == TokenType.RParen) {
        parser.increment();
    } else {
        parser.error("expected ')' after calc expression");
    }

    var data = builder.allocate<CSSCalcValueData>();
    new (data) CSSCalcValueData {
        expression : expr
    };

    value.kind = CSSValueKind.Calc;
    value.data = data;
}
