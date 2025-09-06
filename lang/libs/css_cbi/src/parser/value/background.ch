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

func (cssParser : &mut CSSParser) parseLinearGradient(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut UrlData) {
    const next = parser.getToken()

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
                parser.error("TODO: linear-gradient");
            }
            comptime_fnv1_hash("radial-gradient") => {
                parser.error("TODO: radial-gradient");
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