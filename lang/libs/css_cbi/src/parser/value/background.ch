func (cssParser : &mut CSSParser) parseUrlValue(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut UrlData) {
    const next = parser.getToken()
    if(next.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' after 'url'");
    }
    const str = parser.getToken()
    if(str.type == TokenType.DoubleQuotedValue || str.type == TokenType.SingleQuotedValue) {
        parser.increment()
        data.value = builder.allocate_view(str.value)
    } else {
        parser.error("expected a url string inside 'url'");
    }
    const next2 = parser.getToken()
    if(next2.type == TokenType.RParen) {
        parser.increment()
    } else {
        parser.error("expected a ')' after 'url'");
    }
}

func getSideOrCornerKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("left") => { return CSSKeywordKind.Left }
        comptime_fnv1_hash("right") => { return CSSKeywordKind.Right }
        comptime_fnv1_hash("top") => { return CSSKeywordKind.Top }
        comptime_fnv1_hash("bottom") => { return CSSKeywordKind.Bottom }
        default => { return CSSKeywordKind.Unknown }
    }
}

func (cssParser : &mut CSSParser) parseLinearColorStop(parser : *mut Parser, builder : *mut ASTBuilder, stop : &mut LinearColorStop) : bool {
    if(!cssParser.parseCSSColor(parser, builder, stop.color)) {
        return false;
    }
    cssParser.parseLength(parser, builder, stop.length)
    return true;
}

func (cssParser : &mut CSSParser) parseLinearGradient(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut GradientData) {

    const next = parser.getToken()
    if(next.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' after 'linear-gradient'");
    }

    const lin_data = builder.allocate<LinearGradientData>()
    new (lin_data) LinearGradientData()

    data.kind = CSSGradientKind.Linear
    data.data = lin_data;

    const token = parser.getToken()
    if(token.type == TokenType.Number) {
        if(!parser.parseLengthInto(builder, lin_data.angle)) {
            parser.error("expected length for angle");
        }
    } else if(token.type == TokenType.Identifier) {
        if(token.value.equals("to")) {
            parser.increment()
            const sidCorner = parser.getToken()
            const kind = getSideOrCornerKeywordKind(sidCorner.fnv1())
            if(kind != CSSKeywordKind.Unknown) {
                lin_data.to.kind = kind
                lin_data.to.value = builder.allocate_view(sidCorner.value)
            } else {
                parser.error("expected a side or corner from 'left', 'right', 'top', 'bottom'");
            }
        } else {

            lin_data.color_stop_list.push(LinearColorStopWHint())
            const last = lin_data.color_stop_list.last_ptr()
            cssParser.parseLinearColorStop(parser, builder, last.stop)

            const t2 = parser.getToken()
            if(t2.type == TokenType.Comma) {
                parser.increment()
            }

            while(true) {
                lin_data.color_stop_list.push(LinearColorStopWHint())
                const stop = lin_data.color_stop_list.last_ptr()

                // optional hint
                cssParser.parseLength(parser, builder, stop.hint)

                if(!cssParser.parseLinearColorStop(parser, builder, stop.stop)) {
                    break;
                }

                const t = parser.getToken()
                if(t.type == TokenType.Comma) {
                    parser.increment()
                } else {
                    break;
                }

            }
        }
    }


    const next2 = parser.getToken()
    if(next2.type == TokenType.RParen) {
        parser.increment()
    } else {
        parser.error("expected a ')' after 'linear-gradient'");
    }

}

func (cssParser : &mut CSSParser) parseRadialGradient(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut GradientData) {
    const next = parser.getToken()
    if(next.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' after 'radial-gradient'");
    }

    const next3 = parser.getToken()
    parser.error("TODO: Not yet implemented");

    const next2 = parser.getToken()
    if(next2.type == TokenType.RParen) {
        parser.increment()
    } else {
        parser.error("expected a ')' after 'radial-gradient'");
    }

}

func (cssParser : &mut CSSParser) parseConicGradient(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut GradientData) {
    const next = parser.getToken()
    if(next.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' after 'conic-gradient'");
    }

    const next3 = parser.getToken()
    parser.error("TODO: Not yet implemented");

    const next2 = parser.getToken()
    if(next2.type == TokenType.RParen) {
        parser.increment()
    } else {
        parser.error("expected a ')' after 'conic-gradient'");
    }

}

func (cssParser : &mut CSSParser) parseBackgroundImageInto(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    image : &mut BackgroundImageData
) : bool {

    const token = parser.getToken()
    if(token.type == TokenType.Identifier) {
        const hash = token.fnv1()
        switch(hash) {
            comptime_fnv1_hash("url") => {
                parser.increment()
                cssParser.parseUrlValue(parser, builder, image.url)
            }
            comptime_fnv1_hash("linear-gradient") => {
                parser.increment()
                image.is_url = false;
                cssParser.parseLinearGradient(parser, builder, image.gradient)
            }
            comptime_fnv1_hash("radial-gradient") => {
                parser.increment()
                image.is_url = false;
                cssParser.parseRadialGradient(parser, builder, image.gradient)
            }
            comptime_fnv1_hash("conic-gradient") => {
                parser.increment()
                image.is_url = false;
                cssParser.parseConicGradient(parser, builder, image.gradient)
            }
        }
    }

}

func (cssParser : &mut CSSParser) parseBackgroundImage(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {

    const data = builder.allocate<MultipleBackgroundImageData>()
    new (data) MultipleBackgroundImageData()

    value.kind = CSSValueKind.BackgroundImage
    value.data = data

    while(true) {
        data.images.push(BackgroundImageData())
        const ptr = data.images.last_ptr()
        const parsed = cssParser.parseBackgroundImageInto(
            parser, builder, *ptr
        )
        if(parsed) {
            const t = parser.getToken()
            if(t.type == TokenType.Comma) {
                parser.increment()
            } else {
                break;
            }
        } else {
            break
        }
    }

}


func (cssParser : &mut CSSParser) parseBackground(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {



}