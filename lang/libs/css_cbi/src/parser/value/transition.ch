
func (cssParser : &mut CSSParser) parseLinearEasingPoints(parser : *mut Parser, builder : *mut ASTBuilder) : *CSSLinearEasingPoint {

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

        if(!cssParser.parseNumberOrLengthInto(parser, builder, point.point)) {
            parser.error("expected a number for linear");
        }

        cssParser.parseLengthInto(parser, builder, point.start)

        cssParser.parseLengthInto(parser, builder, point.stop)

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

func (cssParser : &mut CSSParser) parseCubicBezierCall(parser : *mut Parser, builder : *mut ASTBuilder) : *CSSCubicBezierEasingData {

    const lpTok = parser.getToken()
    if(lpTok.type == TokenType.LParen) {
        parser.increment();
    } else {
        parser.error("expected '(' for linear function");
    }

    var bezier = builder.allocate<CSSCubicBezierEasingData>()
    new (bezier) CSSCubicBezierEasingData()

    if(!cssParser.parseNumberInto(parser, builder, bezier.x1)) {
        parser.error("expected a number for x1 in bezier-curve");
    }

    parser.consume(TokenType.Comma)

    if(!cssParser.parseNumberInto(parser, builder, bezier.y1)) {
        parser.error("expected a number for y1 in bezier-curve");
    }

    parser.consume(TokenType.Comma)

    if(!cssParser.parseNumberInto(parser, builder, bezier.x2)) {
        parser.error("expected a number for x2 in bezier-curve");
    }

    parser.consume(TokenType.Comma)

    if(!cssParser.parseNumberInto(parser, builder, bezier.y2)) {
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

func (cssParser : &mut CSSParser) parseStepsFnCall(parser : *mut Parser, builder : *mut ASTBuilder) : *CSSStepsEasingData {

    const lpTok = parser.getToken()
    if(lpTok.type == TokenType.LParen) {
        parser.increment();
    } else {
        parser.error("expected '(' for linear function");
    }

    var steps = builder.allocate<CSSStepsEasingData>()
    new (steps) CSSStepsEasingData()

    if(!cssParser.parseNumberInto(parser, builder, steps.step)) {
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
                        transition.easing.data.linear = cssParser.parseLinearEasingPoints(parser, builder)
                        transition.easing.kind = CSSKeywordKind.Linear
                    } else {
                        transition.easing.data.linear = null
                    }
                } else {
                    transition.easing.data.keyword = CSSKeywordValueData { kind : kind, value : builder.allocate_view(token.value) }
                }
            } else if(hash == comptime_fnv1_hash("cubic-bezier")) {
                parser.increment()
                transition.easing.data.bezier = cssParser.parseCubicBezierCall(parser, builder)
                transition.easing.kind = CSSKeywordKind.CubicBezier
            } else if(hash == comptime_fnv1_hash("step")) {
                parser.increment()
                transition.easing.data.steps = cssParser.parseStepsFnCall(parser, builder)
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
                    cssParser.parseLengthInto(parser, builder, transition.delay)
                } else {
                    parser.error("too many lengths given");
                    break;
                }
            } else {
                cssParser.parseLengthInto(parser, builder, transition.duration)
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

func (cssParser : &mut CSSParser) parseTransitionTimingFunction(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type == TokenType.Identifier) {

        const easing = builder.allocate<CSSEasingFunction>()
        new (easing) CSSEasingFunction()

        value.data = easing
        value.kind = CSSValueKind.TransitionTimingFunction

        const hash = token.fnv1();
        const kind = getAnimationTimingFunctionKeywordKind(hash)
        if(kind != CSSKeywordKind.Unknown) {
            parser.increment()
            easing.kind = kind
            if(kind == CSSKeywordKind.Linear) {
                const next = parser.getToken()
                if(next.type == TokenType.LParen) {
                    // parse linear function
                    easing.data.linear = cssParser.parseLinearEasingPoints(parser, builder)
                    easing.kind = CSSKeywordKind.Linear
                } else {
                    easing.data.linear = null
                }
            } else {
                easing.data.keyword = CSSKeywordValueData { kind : kind, value : builder.allocate_view(token.value) }
            }
        } else if(hash == comptime_fnv1_hash("cubic-bezier")) {
            parser.increment()
            easing.data.bezier = cssParser.parseCubicBezierCall(parser, builder)
            easing.kind = CSSKeywordKind.CubicBezier
        } else if(hash == comptime_fnv1_hash("steps")) {
            parser.increment()
            easing.data.steps = cssParser.parseStepsFnCall(parser, builder)
            easing.kind = CSSKeywordKind.Steps
        } else {
            parser.error("unknown identifier given");
        }
    } else {
        parser.error("expected an identifier for transition-timing-function");
    }
}