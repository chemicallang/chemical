func (cssParser : &mut CSSParser) parseAnimation(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    var anim = builder.allocate<CSSAnimationValueData>()
    new (anim) CSSAnimationValueData()
    value.kind = CSSValueKind.Animation
    value.data = anim

    var has_duration = false
    var has_delay = false

    while(true) {
        const token = parser.getToken()
        if(token.type == TokenType.Semicolon || token.type == TokenType.RBrace) break

        if(token.type == TokenType.Number) {
            // Check if this number has a time unit (s or ms) - it's a duration or delay
            // Otherwise it's an iteration count (e.g. "2" in "animation: spin 1s 2")
            const nextTok = token + 1
            var hasTimeUnit = false
            if(nextTok.type == TokenType.Identifier) {
                if(nextTok.value.equals("s") || nextTok.value.equals("ms")) {
                    hasTimeUnit = true
                }
            }
            if(hasTimeUnit) {
                if(!has_duration) {
                    cssParser.parseLengthInto(parser, builder, &mut anim.duration)
                    has_duration = true
                } else if(!has_delay) {
                    cssParser.parseLengthInto(parser, builder, &mut anim.delay)
                    has_delay = true
                } else {
                    parser.error("too many time values in animation")
                    break
                }
            } else {
                // Integer iteration count (e.g. "2")
                var numVal = builder.allocate<CSSLengthValueData>()
                new (numVal) CSSLengthValueData {
                    kind : CSSLengthKind.None,
                    value : builder.allocate_view(&token.value)
                }
                anim.iterationCount.kind = CSSValueKind.Length
                anim.iterationCount.data = numVal
                parser.increment()
            }
        } else if(token.type == TokenType.Identifier) {
            const hash = token.fnv1()
            // Check for easing
            const easingKind = getAnimationTimingFunctionKeywordKind(hash)
            if(easingKind != CSSKeywordKind.Unknown) {
                parser.increment()
                anim.easing.kind = easingKind
                if(easingKind == CSSKeywordKind.Linear) {
                    const next = parser.getToken()
                    if(next.type == TokenType.LParen) {
                        anim.easing.data.linear = cssParser.parseLinearEasingPoints(parser, builder)
                    } else {
                        anim.easing.data.linear = null
                    }
                } else {
                    anim.easing.data.keyword = CSSKeywordValueData { kind = easingKind, value = builder.allocate_view(&token.value) }
                }
            } else if(hash == comptime_fnv1_hash("var")) {
                parser.increment()
                var varName = cssParser.parseCSSVariableFunc(parser, builder)
                anim.easing.data.keyword = CSSKeywordValueData { kind : CSSKeywordKind.Var, value : builder.allocate_view(&varName) }
                anim.easing.kind = CSSKeywordKind.Var
            } else if(hash == comptime_fnv1_hash("cubic-bezier")) {
                parser.increment()
                anim.easing.data.bezier = cssParser.parseCubicBezierCall(parser, builder)
                anim.easing.kind = CSSKeywordKind.CubicBezier
            } else if(hash == comptime_fnv1_hash("step") || hash == comptime_fnv1_hash("steps")) {
                parser.increment()
                anim.easing.data.steps = cssParser.parseStepsFnCall(parser, builder)
                anim.easing.kind = CSSKeywordKind.Steps
            } else if(hash == comptime_fnv1_hash("infinite")) {
                parser.increment()
                var kwVal = builder.allocate<CSSKeywordValueData>()
                new (kwVal) CSSKeywordValueData {
                    kind = CSSKeywordKind.Infinite,
                    value = builder.allocate_view(&token.value)
                }
                anim.iterationCount.kind = CSSValueKind.Keyword
                anim.iterationCount.data = kwVal
            } else if(anim.name.empty()) {
                parser.increment()
                anim.name = builder.allocate_view(&token.value)
            } else {
                // Direction
                const dirKind = getAnimationDirectionKeywordKind(hash)
                if(dirKind != CSSKeywordKind.Unknown) {
                    parser.increment()
                    anim.direction.kind = dirKind
                    anim.direction.value = builder.allocate_view(&token.value)
                } else {
                    // Fill mode
                    const fillKind = getAnimationFillModeKeywordKind(hash)
                    if(fillKind != CSSKeywordKind.Unknown) {
                        parser.increment()
                        anim.fillMode.kind = fillKind
                        anim.fillMode.value = builder.allocate_view(&token.value)
                    } else {
                        // Play state
                        const playKind = getAnimationPlayStateKeywordKind(hash)
                        if(playKind != CSSKeywordKind.Unknown) {
                            parser.increment()
                            anim.playState.kind = playKind
                            anim.playState.value = builder.allocate_view(&token.value)
                        } else {
                            parser.increment()
                        }
                    }
                }
            }
        } else if(token.type == TokenType.Comma) {
            parser.increment()
            var next = builder.allocate<CSSAnimationValueData>()
            new (next) CSSAnimationValueData()
            anim.next = next
            anim = next
        } else {
            break
        }
    }
}

func (cssParser : &mut CSSParser) parseAnimationDelay(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    cssParser.parseLength(parser, builder, value)
}
