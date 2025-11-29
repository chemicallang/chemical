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

func getRadialShapeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("circle") => { return CSSKeywordKind.Circle }
        comptime_fnv1_hash("ellipse") => { return CSSKeywordKind.Ellipse }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getRadialSizeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("closest-side") => { return CSSKeywordKind.ClosestSide }
        comptime_fnv1_hash("closest-corner") => { return CSSKeywordKind.ClosestCorner }
        comptime_fnv1_hash("farthest-side") => { return CSSKeywordKind.FarthestSide }
        comptime_fnv1_hash("farthest-corner") => { return CSSKeywordKind.FarthestCorner }
        default => { return CSSKeywordKind.Unknown }
    }
}

func (cssParser : &mut CSSParser) parseLinearColorStop(parser : *mut Parser, builder : *mut ASTBuilder, stop : &mut LinearColorStop) : bool {
    if(!cssParser.parseCSSColor(parser, builder, stop.color)) {
        return false;
    }
    if(cssParser.parseLength(parser, builder, stop.length)) {
        cssParser.parseLength(parser, builder, stop.optSecLength)
    }
    return true;
}

func (cssParser : &mut CSSParser) parseColorStopList(parser : *mut Parser, builder : *mut ASTBuilder, list : &mut std::vector<LinearColorStopWHint>) {
    while(true) {
        list.push(LinearColorStopWHint())
        const stop = list.last_ptr()

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

func (cssParser : &mut CSSParser) parseLinearGradient(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut GradientData, repeating : bool) {

    const next = parser.getToken()
    if(next.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' after 'linear-gradient'");
    }

    const lin_data = builder.allocate<LinearGradientData>()
    new (lin_data) LinearGradientData()

    data.kind = if(repeating) CSSGradientKind.RepeatingLinear else CSSGradientKind.Linear
    data.data = lin_data;

    const token = parser.getToken()
    if(token.type == TokenType.Number) {

        if(!cssParser.parseLengthInto(parser, builder, lin_data.angle)) {
            parser.error("expected length for angle");
        }

        const t2 = parser.getToken()
        if(t2.type == TokenType.Comma) {
            parser.increment()
        }

        cssParser.parseColorStopList(parser, builder, lin_data.color_stop_list)

    } else if(token.type == TokenType.Identifier) {
        if(token.value.equals("to")) {
            parser.increment()

            const sidCorner = parser.getToken()
            const kind = getSideOrCornerKeywordKind(sidCorner.fnv1())
            if(kind != CSSKeywordKind.Unknown) {
                parser.increment()
                lin_data.to1.kind = kind
                lin_data.to1.value = builder.allocate_view(sidCorner.value)
            } else {
                parser.error("expected a side or corner from 'left', 'right', 'top', 'bottom'");
            }

            const nSid = parser.getToken()
            if(nSid.type != TokenType.Comma) {
                const kind = getSideOrCornerKeywordKind(nSid.fnv1())
                if(kind != CSSKeywordKind.Unknown) {
                    parser.increment()
                    lin_data.to2.kind = kind
                    lin_data.to2.value = builder.allocate_view(nSid.value)
                }
            }

            const t2 = parser.getToken()
            if(t2.type == TokenType.Comma) {
                parser.increment()
            }

            cssParser.parseColorStopList(parser, builder, lin_data.color_stop_list)

        } else {

            lin_data.color_stop_list.push(LinearColorStopWHint())
            const last = lin_data.color_stop_list.last_ptr()
            cssParser.parseLinearColorStop(parser, builder, last.stop)

            const t2 = parser.getToken()
            if(t2.type == TokenType.Comma) {
                parser.increment()
            }

            cssParser.parseColorStopList(parser, builder, lin_data.color_stop_list)

        }
    }


    const next2 = parser.getToken()
    if(next2.type == TokenType.RParen) {
        parser.increment()
    } else {
        parser.error("expected a ')' after 'linear-gradient'");
    }

}

func (cssParser : &mut CSSParser) parseRadialGradient(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut GradientData, repeating : bool) {
    const next = parser.getToken()
    if(next.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' after 'radial-gradient'");
    }

    const rad_data = builder.allocate<RadialGradientData>()
    new (rad_data) RadialGradientData()

    data.kind = if(repeating) CSSGradientKind.RepeatingRadial else CSSGradientKind.Radial
    data.data = rad_data;

    // Parse shape, size, position
    // Syntax: [ <ending-shape> || <size> ]? [ at <position> ]? , <color-stop-list>
    
    var has_shape_or_size = false
    
    // Try parsing shape or size keywords
    var token = parser.getToken()
    if(token.type == TokenType.Identifier) {
        var hash = token.fnv1()
        
        // Check for shape
        var shapeKind = getRadialShapeKeywordKind(hash)
        if(shapeKind != CSSKeywordKind.Unknown || hash == comptime_fnv1_hash("ellipse")) {
            parser.increment()
            alloc_keyword_data(builder, rad_data.shape, shapeKind, token.value)
            has_shape_or_size = true
            
            // Check for size after shape
            token = parser.getToken()
            if(token.type == TokenType.Identifier) {
                hash = token.fnv1()
                const sizeKind = getRadialSizeKeywordKind(hash)
                if(sizeKind != CSSKeywordKind.Unknown) {
                    parser.increment()
                    alloc_keyword_data(builder, rad_data.size.extent, sizeKind, token.value)
                }
            } else if(token.type == TokenType.Number) {
                 // Length size
                 cssParser.parseLength(parser, builder, rad_data.size.length)
                 // If ellipse, can have second length
                 if(rad_data.shape.kind == CSSKeywordKind.Unknown && rad_data.shape.value.equals("ellipse")) { // Ellipse is Unknown kind with value "ellipse"
                     // Try second length
                     var second = CSSValue()
                     if(cssParser.parseLength(parser, builder, second)) {
                         const pair = builder.allocate<CSSValuePair>()
                         pair.first = rad_data.size.length
                         pair.second = second
                         rad_data.size.length.kind = CSSValueKind.Pair
                         rad_data.size.length.data = pair
                     }
                 }
            }
        } else {
            // Check for size keyword first
            const sizeKind = getRadialSizeKeywordKind(hash)
            if(sizeKind != CSSKeywordKind.Unknown) {
                parser.increment()
                alloc_keyword_data(builder, rad_data.size.extent, sizeKind, token.value)
                has_shape_or_size = true
                
                // Check for shape after size
                token = parser.getToken()
                if(token.type == TokenType.Identifier) {
                    hash = token.fnv1()
                    shapeKind = getRadialShapeKeywordKind(hash)
                    if(shapeKind != CSSKeywordKind.Unknown) {
                        parser.increment()
                        alloc_keyword_data(builder, rad_data.shape, shapeKind, token.value)
                    }
                }
            }
        }
    }
    
    // If we didn't find keywords, maybe we have lengths (size)
    if(!has_shape_or_size) {
        if(cssParser.parseLength(parser, builder, rad_data.size.length)) {
            has_shape_or_size = true
            // Try second length
             var second = CSSValue()
             if(cssParser.parseLength(parser, builder, second)) {
                 const pair = builder.allocate<CSSValuePair>()
                 pair.first = rad_data.size.length
                 pair.second = second
                 rad_data.size.length.kind = CSSValueKind.Pair
                 rad_data.size.length.data = pair
             }
             
             // Check for shape after length size
             token = parser.getToken()
             if(token.type == TokenType.Identifier) {
                const hash = token.fnv1()
                const shapeKind = getRadialShapeKeywordKind(hash)
                if(shapeKind != CSSKeywordKind.Unknown) {
                    parser.increment()
                    alloc_keyword_data(builder, rad_data.shape, shapeKind, token.value)
                }
             }
        }
    }

    // Parse 'at <position>'
    token = parser.getToken()
    if(token.type == TokenType.Identifier && token.value.equals("at")) {
        parser.increment()
        
        var posX = CSSValue()
        var posY = CSSValue()
        if(cssParser.parsePositionValue(parser, builder, posX, posY)) {
            if(!posY.isUnknown()) {
                const pair = builder.allocate<CSSValuePair>()
                pair.first = posX
                pair.second = posY
                rad_data.position.kind = CSSValueKind.Pair
                rad_data.position.data = pair
            } else {
                rad_data.position = posX
            }
        } else {
            parser.error("expected position after 'at'")
        }
    }
    
    // Comma before color stops if we parsed anything before
    if(has_shape_or_size || !rad_data.position.isUnknown()) {
        const t = parser.getToken()
        if(t.type == TokenType.Comma) {
            parser.increment()
        }
    }

    cssParser.parseColorStopList(parser, builder, rad_data.color_stop_list)

    const next2 = parser.getToken()
    if(next2.type == TokenType.RParen) {
        parser.increment()
    } else {
        parser.error("expected a ')' after 'radial-gradient'");
    }

}

func (cssParser : &mut CSSParser) parseConicGradient(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut GradientData, repeating : bool) {
    const next = parser.getToken()
    if(next.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' after 'conic-gradient'");
    }

    const con_data = builder.allocate<ConicGradientData>()
    new (con_data) ConicGradientData()

    data.kind = if(repeating) CSSGradientKind.RepeatingConic else CSSGradientKind.Conic
    data.data = con_data;

    // Parse from <angle> at <position>
    var has_from_or_at = false
    
    var token = parser.getToken()
    if(token.type == TokenType.Identifier) {
        if(token.value.equals("from")) {
            parser.increment()
            if(!cssParser.parseLength(parser, builder, con_data.from)) { // reusing parseLengthInto for angle
                 // parseLengthInto expects CSSLengthValueData, but from is CSSValue.
                 // We need to parse angle into CSSValue.
                 // Let's use parseLength which parses into CSSValue.
                 // But parseLength parses lengths. Angles are lengths in this parser context?
                 // Usually yes, or we have parseAngle.
                 // parseLength calls parseLengthInto.
                 // Let's use parseLength.
                 if(!cssParser.parseLength(parser, builder, con_data.from)) {
                     parser.error("expected angle after 'from'")
                 }
            }
            has_from_or_at = true
        }
    }
    
    token = parser.getToken()
    if(token.type == TokenType.Identifier && token.value.equals("at")) {
        parser.increment()
        var posX = CSSValue()
        var posY = CSSValue()
        if(cssParser.parsePositionValue(parser, builder, posX, posY)) {
            if(!posY.isUnknown()) {
                const pair = builder.allocate<CSSValuePair>()
                pair.first = posX
                pair.second = posY
                con_data.at.kind = CSSValueKind.Pair
                con_data.at.data = pair
            } else {
                con_data.at = posX
            }
            has_from_or_at = true
        } else {
            parser.error("expected position after 'at'")
        }
    }
    
    if(has_from_or_at) {
        const t = parser.getToken()
        if(t.type == TokenType.Comma) {
            parser.increment()
        }
    }
    
    // Parse color stops
    cssParser.parseColorStopList(parser, builder, con_data.color_stop_list)

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
                return true
            }
            comptime_fnv1_hash("linear-gradient") => {
                parser.increment()
                image.is_url = false;
                cssParser.parseLinearGradient(parser, builder, image.gradient, false)
                return true
            }
            comptime_fnv1_hash("repeating-linear-gradient") => {
                parser.increment()
                image.is_url = false;
                cssParser.parseLinearGradient(parser, builder, image.gradient, true)
                return true
            }
            comptime_fnv1_hash("radial-gradient") => {
                parser.increment()
                image.is_url = false;
                cssParser.parseRadialGradient(parser, builder, image.gradient, false)
                return true
            }
            comptime_fnv1_hash("repeating-radial-gradient") => {
                parser.increment()
                image.is_url = false;
                cssParser.parseRadialGradient(parser, builder, image.gradient, true)
                return true
            }
            comptime_fnv1_hash("conic-gradient") => {
                parser.increment()
                image.is_url = false;
                cssParser.parseConicGradient(parser, builder, image.gradient, false)
                return true
            }
            comptime_fnv1_hash("repeating-conic-gradient") => {
                parser.increment()
                image.is_url = false;
                cssParser.parseConicGradient(parser, builder, image.gradient, true)
                return true
            }
        }
    }

    return false;

}

func (cssParser : &mut CSSParser) parseBackgroundImage(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {

    const data = builder.allocate<MultipleBackgroundImageData>()
    new (data) MultipleBackgroundImageData()

    value.kind = CSSValueKind.MultipleBackgroundImage
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

func (cssParser : &mut CSSParser) parsePositionValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    posX : &mut CSSValue,
    posY : &mut CSSValue
) : bool {
    // Try to parse first value (length or keyword)
    if(cssParser.parseLength(parser, builder, posX)) {
        // Try to parse second value
        cssParser.parseLength(parser, builder, posY);
        return true;
    }
    
    // Try to parse keyword for first value
    const token = parser.getToken();
    if(token.type == TokenType.Identifier) {
        const hash = token.fnv1();
        const kind = getSideOrCornerKeywordKind(hash);
        if(kind != CSSKeywordKind.Unknown || hash == comptime_fnv1_hash("center")) {
            parser.increment();
            alloc_value_keyword(builder, posX, if(kind != CSSKeywordKind.Unknown) kind else CSSKeywordKind.Center, token.value);
            
            // Try second value
            const token2 = parser.getToken();
            if(token2.type == TokenType.Identifier) {
                const hash2 = token2.fnv1();
                const kind2 = getSideOrCornerKeywordKind(hash2);
                if(kind2 != CSSKeywordKind.Unknown || hash2 == comptime_fnv1_hash("center")) {
                    parser.increment();
                    alloc_value_keyword(builder, posY, if(kind2 != CSSKeywordKind.Unknown) kind2 else CSSKeywordKind.Center, token2.value);
                }
            } else {
                cssParser.parseLength(parser, builder, posY);
            }
            return true;
        }
    }
    
    return false;
}

func (cssParser : &mut CSSParser) parseBackgroundSizeValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    size : &mut CSSValue
) {
    // size can be: length | percentage | auto | cover | contain
    // and can be one or two values.
    // For simplicity, let's parse one length/keyword.
    // Ideally we should parse up to two values.
    
    // We will store it as a Pair if two values, or single value if one.
    // But size in CSSBackgroundLayerData is a single CSSValue.
    // If we have two values, we can use CSSValueKind.Pair.
    
    var first = CSSValue();
    
    if(cssParser.parseLength(parser, builder, first)) {
        // parsed first length
    } else {
        const token = parser.getToken();
        if(token.type == TokenType.Identifier) {
            const hash = token.fnv1();
            if(hash == comptime_fnv1_hash("auto") || hash == comptime_fnv1_hash("cover") || hash == comptime_fnv1_hash("contain")) {
                parser.increment();
                alloc_value_keyword(builder, first, CSSKeywordKind.Unknown, token.value); // We should map these keywords properly if we had them in enum
            }
        }
    }
    
    if(!first.isUnknown()) {
        // check for second value
        var second = CSSValue();
        
        if(cssParser.parseLength(parser, builder, second)) {
            // parsed second length
        } else {
             const token = parser.getToken();
             if(token.type == TokenType.Identifier && token.value.equals("auto")) {
                 parser.increment();
                 alloc_value_keyword(builder, second, CSSKeywordKind.Auto, token.value);
             }
        }
        
        if(!second.isUnknown()) {
            const pair = builder.allocate<CSSValuePair>();
            pair.first = first;
            pair.second = second;
            size.kind = CSSValueKind.Pair;
            size.data = pair;
        } else {
            *size = first;
        }
    }
    
}

func (cssParser : &mut CSSParser) parseBackgroundLayer(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    layer : &mut CSSBackgroundLayerData
) : bool {

    var any = false;

    while(true) {

        const token = parser.getToken();
        if(token.type == TokenType.Identifier) {

            const hash = token.fnv1();

            // check for background-repeat
            if(layer.repeat.isUnknown()) {
                const repeatKind = getBackgroundRepeatKeywordKind(hash);
                if(repeatKind != CSSKeywordKind.Unknown) {
                    parser.increment();
                    alloc_value_keyword(builder, layer.repeat, repeatKind, token.value);
                    any = true;
                    continue;
                }
            }

            // check for background-attachment
            if(layer.attachment.isUnknown()) {
                const attachmentKind = getBackgroundAttachmentKeywordKind(hash);
                if(attachmentKind != CSSKeywordKind.Unknown) {
                    parser.increment();
                    alloc_value_keyword(builder, layer.attachment, attachmentKind, token.value);
                    any = true;
                    continue;
                }
            }

            // check for background-origin / background-clip
            // they share the same keywords (border-box, padding-box, content-box)
            // if one is present, it sets origin. if two are present, first is origin, second is clip.
            const boxKind = getBackgroundBoxKeywordKind(hash);
            if(boxKind != CSSKeywordKind.Unknown) {
                parser.increment();
                if(layer.origin.isUnknown()) {
                    alloc_value_keyword(builder, layer.origin, boxKind, token.value);
                    any = true;
                    continue;
                } else if(layer.clip.isUnknown()) {
                    alloc_value_keyword(builder, layer.clip, boxKind, token.value);
                    any = true;
                    continue;
                }
            }

            // check for background-image (none)
            if(layer.image.isUnknown()) {
                if(hash == comptime_fnv1_hash("none")) {
                    parser.increment();
                    // explicit none for image
                    // we can represent this as an empty BackgroundImageData or a specific flag
                    // for now, let's assume empty image data means none if explicitly parsed?
                    // actually, let's use a keyword value for 'none' if needed, but BackgroundImageData structure is specific.
                    // Let's parse it as a keyword 'none' into the image value slot if we change the structure,
                    // but here image is CSSValue. Wait, in the struct definition I made image : CSSValue.
                    // So yes, we can put a keyword there.
                    alloc_value_keyword(builder, layer.image, CSSKeywordKind.None, token.value);
                    any = true;
                    continue;
                }
            }

        }

        // check for background-image (url, gradient)
        if(layer.image.isUnknown()) {
             // parseBackgroundImageInto expects a BackgroundImageData struct, but now image is CSSValue.
             // We need to adapt.
             // Let's check if it looks like an image.
             const image_data = builder.allocate<BackgroundImageData>();
             new (image_data) BackgroundImageData();
             if(cssParser.parseBackgroundImageInto(parser, builder, *image_data)) {
                 layer.image.kind = CSSValueKind.BackgroundImage;
                 layer.image.data = image_data;
                 any = true;
                 continue;
             }
        }

        // check for background-position / background-size
        if(layer.positionX.isUnknown()) {
             // try parsing position
             // Position can be 1 to 4 values.
             // For shorthand, it's usually 1 or 2 values for position, optionally followed by / size.
             // This is complex. Let's simplify: try to parse one or two length/keyword values.
             // If we find a slash, we parse size.

             // We need a proper parsePosition function that can handle the ambiguity or just try to parse lengths/keywords.
             // Let's try to parse one value.
             if(cssParser.parsePositionValue(parser, builder, layer.positionX, layer.positionY)) {
                 any = true;

                 // check for / size
                 const slash = parser.getToken();
                 if(slash.type == TokenType.Divide) {
                     parser.increment();
                     // parse size
                     cssParser.parseBackgroundSizeValue(parser, builder, layer.size);
                 }
                 continue;
             }
        }
        
        // if we reached here, we didn't match anything for this layer.
        break;
    }

    return any;

}

func (cssParser : &mut CSSParser) parseBackground(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {

    const data = builder.allocate<CSSBackgroundValueData>();
    new (data) CSSBackgroundValueData();

    value.kind = CSSValueKind.Background;
    value.data = data;

    while(true) {
        
        data.layers.push(CSSBackgroundLayerData());
        const layer = data.layers.last_ptr();

        var parsed = cssParser.parseBackgroundLayer(parser, builder, *layer);

        // check for color on the last layer (or the only layer)
        // Color is only allowed on the last layer.
        // But we don't know if this is the last layer until we see a comma or end of declaration.
        // Actually, if we find a color, it MUST be the last layer.
        if(data.color.isUnknown()) {
            if(cssParser.parseCSSColor(parser, builder, data.color)) {
                parsed = true;
            }
        }

        if(!parsed && data.color.isUnknown()) {
             // if we parsed nothing and no color, this layer is invalid or empty.
             // if it's the first layer and empty, maybe it's global keyword?
             // Global keywords are handled before calling specific parsers usually.
             break;
        }

        const token = parser.getToken();
        if(token.type == TokenType.Comma) {
            parser.increment();
            // if we found a color, we cannot have more layers
            if(!data.color.isUnknown()) {
                parser.error("background color must be defined in the last layer");
            }
        } else {
            break;
        }

    }

}