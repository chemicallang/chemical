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
            if(!has_duration) {
                cssParser.parseLengthInto(parser, builder, anim.duration)
                has_duration = true
            } else if(!has_delay) {
                cssParser.parseLengthInto(parser, builder, anim.delay)
                has_delay = true
            } else {
                parser.error("too many time values in animation")
                break
            }
        } else if(token.type == TokenType.Identifier) {
            const hash = token.fnv1()
            // Check for easing
            const easingKind = getAnimationTimingFunctionKeywordKind(hash)
            if(easingKind != CSSKeywordKind.Unknown) {
                parser.increment()
                anim.easing.kind = easingKind
                anim.easing.data.keyword = CSSKeywordValueData { kind = easingKind, value = builder.allocate_view(token.value) }
            } else if(hash == comptime_fnv1_hash("infinite")) {
                parser.increment()
                var kwVal = builder.allocate<CSSKeywordValueData>()
                new (kwVal) CSSKeywordValueData {
                    kind = CSSKeywordKind.Infinite,
                    value = builder.allocate_view(token.value)
                }
                anim.iterationCount.kind = CSSValueKind.Keyword
                anim.iterationCount.data = kwVal
            } else if(anim.name.empty()) {
                parser.increment()
                anim.name = builder.allocate_view(token.value)
            } else {
                // Direction / Fill mode / Play state / etc.
                parser.increment()
                // Just consume for now as name is the most important
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
