
func (parser : &mut Parser) parseLinearEasingPoints(builder : *mut ASTBuilder) : *CSSLinearEasingPoint {

    const lpTok = parser.getToken()
    if(lpTok.type == TokenType.LParen) {
        parser.increment();
    } else {
        parser.error("expected '(' for linear function");
    }

    const root = builder.allocate<CSSLinearEasingPoint>()
    var point = root
    new (point) CSSLinearEasingPoint()

    while(true) {

        if(!parser.parseNumberOrLengthInto(builder, point.point)) {
            parser.error("expected a number for linear");
        }

        parser.parseLengthInto(builder, point.start)

        parser.parseLengthInto(builder, point.stop)

        const token = parser.getToken()
        if(token.type == TokenType.Comma) {

            parser.increment()

            var nextPoint = builder.allocate<CSSLinearEasingPoint>()
            new (nextPoint) CSSLinearEasingPoint()

            point.next = nextPoint
            point = nextPoint

        } else {

            break;

        }

    }

    const rpTok = parser.getToken()
    if(rpTok.type == TokenType.RParen) {
        parser.increment();
    } else {
        parser.error("expected ')' for linear function");
    }

    return root;

}

func (parser : &mut Parser) parseCubicBezierCall(builder : *mut ASTBuilder) : *CSSCubicBezierEasingData {

    const lpTok = parser.getToken()
    if(lpTok.type == TokenType.LParen) {
        parser.increment();
    } else {
        parser.error("expected '(' for linear function");
    }

    var bezier = builder.allocate<CSSCubicBezierEasingData>()
    new (bezier) CSSCubicBezierEasingData()

    if(!parser.parseNumberInto(builder, bezier.x1)) {
        parser.error("expected a number for x1 in bezier-curve");
    }

    parser.consume(TokenType.Comma)

    if(!parser.parseNumberInto(builder, bezier.y1)) {
        parser.error("expected a number for y1 in bezier-curve");
    }

    parser.consume(TokenType.Comma)

    if(!parser.parseNumberInto(builder, bezier.x2)) {
        parser.error("expected a number for x2 in bezier-curve");
    }

    parser.consume(TokenType.Comma)

    if(!parser.parseNumberInto(builder, bezier.y2)) {
        parser.error("expected a number for y2 in bezier-curve");
    }

    const rpTok = parser.getToken()
    if(rpTok.type == TokenType.RParen) {
        parser.increment();
    } else {
        parser.error("expected ')' for linear function");
    }

    return bezier;

}

func getStepPositionKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("jump-start") => { return CSSKeywordKind.JumpStart }
        comptime_fnv1_hash("jump-end") => { return CSSKeywordKind.JumpEnd }
        comptime_fnv1_hash("jump-none") => { return CSSKeywordKind.JumpNone }
        comptime_fnv1_hash("jump-both") => { return CSSKeywordKind.JumpBoth }
        comptime_fnv1_hash("start") => { return CSSKeywordKind.Start }
        comptime_fnv1_hash("end") => { return CSSKeywordKind.End }
        default => { return CSSKeywordKind.Unknown }
    }
}

func (parser : &mut Parser) parseStepsFnCall(builder : *mut ASTBuilder) : *CSSStepsEasingData {

    const lpTok = parser.getToken()
    if(lpTok.type == TokenType.LParen) {
        parser.increment();
    } else {
        parser.error("expected '(' for linear function");
    }

    var steps = builder.allocate<CSSStepsEasingData>()
    new (steps) CSSStepsEasingData()

    if(!parser.parseNumberInto(builder, steps.step)) {
        parser.error("expected a number in steps");
    }

    parser.incrementToken(TokenType.Comma)

    const stepPosition = parser.getToken()
    if(stepPosition.type == TokenType.Identifier) {

        const hash = stepPosition.fnv1()
        const stepPos = getStepPositionKeywordKind(hash)
        if(stepPos != CSSKeywordKind.Unknown) {
            parser.increment()
            steps.position = CSSKeywordValueData { kind : stepPos, value : builder.allocate_view(stepPosition.value) }
        }

    }

    const rpTok = parser.getToken()
    if(rpTok.type == TokenType.RParen) {
        parser.increment();
    } else {
        parser.error("expected ')' for linear function");
    }

    return steps;

}

func (cssParser : &mut CSSParser) parseTransition(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    // Allocate a new box-shadow value.
    var transition = builder.allocate<CSSTransitionValueData>();
    new (transition) CSSTransitionValueData()

    value.kind = CSSValueKind.Transition;
    value.data = transition;

    var has_duration = false;

    while(true) {

        const token = parser.getToken();
        if(token.type == TokenType.Identifier) {
            const hash = token.fnv1();
            const kind = getAnimationTimingFunctionKeywordKind(hash)
            if(kind != CSSKeywordKind.Unknown) {
                parser.increment()
                transition.easing.kind = kind
                if(kind == CSSKeywordKind.Linear) {
                    const next = parser.getToken()
                    if(next.type == TokenType.LParen) {
                        // parse linear function
                        transition.easing.data.linear = parser.parseLinearEasingPoints(builder)
                        transition.easing.kind = CSSKeywordKind.Linear
                    } else {
                        transition.easing.data.linear = null
                    }
                } else {
                    transition.easing.data.keyword = CSSKeywordValueData { kind : kind, value : builder.allocate_view(token.value) }
                }
            } else if(hash == comptime_fnv1_hash("cubic-bezier")) {
                parser.increment()
                transition.easing.data.bezier = parser.parseCubicBezierCall(builder)
                transition.easing.kind = CSSKeywordKind.CubicBezier
            } else if(hash == comptime_fnv1_hash("step")) {
                parser.increment()
                transition.easing.data.steps = parser.parseStepsFnCall(builder)
                transition.easing.kind = CSSKeywordKind.Steps
            } else if(transition.property.empty()) {
                parser.increment()
                transition.property = builder.allocate_view(token.value)
            } else {
                const behaviorKind = getTransitionBehaviorKeywordKind(hash)
                if(behaviorKind != CSSKeywordKind.Unknown) {
                    parser.increment()
                    transition.behavior = CSSKeywordValueData { kind : behaviorKind, value : builder.allocate_view(token.value) }
                } else {
                    parser.error("unknown identifier given");
                    break;
                }
            }
       } else if(token.type == TokenType.Number) {
            if(has_duration) {
                if(transition.delay.kind == CSSLengthKind.Unknown) {
                    parser.parseLengthInto(builder, transition.delay)
                } else {
                    parser.error("too many lengths given");
                    break;
                }
            } else {
                parser.parseLengthInto(builder, transition.duration)
                has_duration = true;
            }
        } else if(token.type == TokenType.Comma) {
            parser.increment()
            var nextTransition = builder.allocate<CSSTransitionValueData>();
            new (nextTransition) CSSTransitionValueData()
            transition.next = nextTransition
            transition = nextTransition
        } else if(token.type == TokenType.Semicolon) {
            break;
        } else {
            parser.error("unknown token encountered");
            break;
        }

    }

}