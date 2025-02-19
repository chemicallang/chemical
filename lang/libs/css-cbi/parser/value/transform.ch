func (cssParser : &mut CSSParser) parseTransform(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    // Allocate a new box-shadow value.
    var transform = builder.allocate<CSSTransformValueData>();
    new (transform) CSSTransformValueData()

    value.kind = CSSValueKind.Transform;
    value.data = transform;

    while(true) {

        const token = parser.getToken()
        if(token.type == TokenType.Identifier) {

            const hash = token.fnv1()
            if(transform == value.data && hash == comptime_fnv1_hash("none")) {
                parser.increment()
                break;
            }
            const kind = getTransformFunctionKeywordKind(hash)

            transform.transformFunction = CSSKeywordValueData { value : builder.allocate_view(token.value), kind : kind }

            if(kind != CSSKeywordKind.Unknown) {
                parser.increment();

                const lpTok = parser.getToken()
                if(lpTok.type == TokenType.LParen) {
                    parser.increment()
                } else {
                    parser.error("expected a '(' after the transform function");
                }

                const node = builder.allocate<CSSTransformLengthNode>();
                new (node) CSSTransformLengthNode()
                transform.node = node;

                while(true) {
                    if(parser.parseNumberOrLengthInto(builder, node.length)) {

                        if(parser.incrementToken(TokenType.Comma)) {

                            const nextNode = builder.allocate<CSSTransformLengthNode>();
                            new (nextNode) CSSTransformLengthNode()
                            node.next = nextNode;
                            node = nextNode

                        } else {
                            break;
                        }

                    } else {
                        break;
                    }
                }

                const rpTok = parser.getToken()
                if(rpTok.type == TokenType.RParen) {
                    parser.increment()
                } else {
                    parser.error("expected a ')' after the transform function");
                }

                const nextTok = parser.getToken()
                if(nextTok.type == TokenType.Identifier) {
                    var nextTransform = builder.allocate<CSSTransformValueData>();
                    new (nextTransform) CSSTransformValueData()
                    transform.next = nextTransform
                    transform = nextTransform
                }

            } else {
                parser.error("unknown transform function");
                break;
            }

        } else if(token.type == TokenType.Semicolon) {
            break;
        } else {
            parser.error("expected a transform function name");
            break;
        }

    }

}