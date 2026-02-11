func (cssParser : &mut CSSParser) parseGridTemplateTracks(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    var vals = builder.allocate<CSSMultipleValues>()
    new (vals) CSSMultipleValues {
        values : std::vector<CSSValue>()
    }
    value.kind = CSSValueKind.Multiple
    value.data = vals

    while(true) {
        const token = parser.getToken()
        if(token.type == TokenType.Semicolon || token.type == TokenType.RBrace) {
            break
        }
        
        var trackValue = CSSValue()
        if(cssParser.parseLength(parser, builder, trackValue)) {
            vals.values.push(trackValue)
            continue
        } else if(token.type == TokenType.Identifier) {
             const view2 = token.value;
             if(view2.equals("auto") || view2.equals("min-content") || view2.equals("max-content") || view2.equals("fit-content")) {
                 parser.increment()
                 var kwKind = CSSKeywordKind.Auto
                 if(view2.equals("min-content")) kwKind = CSSKeywordKind.MinContent
                 else if(view2.equals("max-content")) kwKind = CSSKeywordKind.MaxContent
                 else if(view2.equals("fit-content")) kwKind = CSSKeywordKind.FitContent
                 
                 var kwVal = builder.allocate<CSSKeywordValueData>()
                 new (kwVal) CSSKeywordValueData {
                     kind : kwKind,
                     value : builder.allocate_view(view2)
                 }
                 trackValue.kind = CSSValueKind.Keyword
                 trackValue.data = kwVal
                 vals.values.push(trackValue)
                 continue
             } else if(view2.equals("repeat")) {
                 parser.increment()
                 if(parser.increment_if(TokenType.LParen as int)) {
                     var repeatData = builder.allocate<GridRepeatData>()
                     new (repeatData) GridRepeatData {
                         count = CSSValue(),
                         tracks = std::vector<CSSValue>()
                     }
                     
                     // Parse count (number or keyword like auto-fill)
                     if(!cssParser.parseRandomValue(parser, builder, repeatData.count)) {
                         parser.error("expected repeat count")
                     }
                     
                     if(!parser.increment_if(TokenType.Comma as int)) {
                         parser.error("expected comma in repeat()")
                     }
                     
                     // Parse tracks
                     while(true) {
                         const t2 = parser.getToken()
                         if(t2.type == TokenType.RParen || t2.type == TokenType.Semicolon) break
                         
                         var v2 = CSSValue()
                         if(cssParser.parseLength(parser, builder, v2)) {
                             repeatData.tracks.push(v2)
                         } else if(t2.type == TokenType.Identifier) {
                             // Handle auto, min-content etc inside repeat
                             parser.increment()
                             var kwVal = builder.allocate<CSSKeywordValueData>()
                             new (kwVal) CSSKeywordValueData {
                                 kind = CSSKeywordKind.Auto, // simplified
                                 value = builder.allocate_view(t2.value)
                             }
                             v2.kind = CSSValueKind.Keyword
                             v2.data = kwVal
                             repeatData.tracks.push(v2)
                         } else {
                             break
                         }
                     }
                     
                     if(!parser.increment_if(TokenType.RParen as int)) {
                         parser.error("expected ) in repeat()")
                     }
                     
                     trackValue.kind = CSSValueKind.GridRepeat
                     trackValue.data = repeatData
                     vals.values.push(trackValue)
                     continue
                 }
             }
        }
        
        break
    }
}

func (cssParser : &mut CSSParser) parseGridLine(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) : bool {
    const token = parser.getToken()
    var is_span = false
    if(token.type == TokenType.Identifier && token.value.equals("span")) {
        parser.increment()
        is_span = true
    }
    
    var lineVal = CSSValue()
    if(cssParser.parseRandomValue(parser, builder, lineVal)) {
        var data = builder.allocate<GridLineData>()
        new (data) GridLineData {
            is_span = is_span,
            value = lineVal
        }
        value.kind = CSSValueKind.GridLine
        value.data = data
        return true
    }
    return false
}

func (cssParser : &mut CSSParser) parseGridLineOrPair(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    if(!cssParser.parseGridLine(parser, builder, value)) return;
    
    if(parser.increment_if(TokenType.Divide as int)) {
        var first = value
        var second = CSSValue()
        if(cssParser.parseGridLine(parser, builder, second)) {
            var pair = builder.allocate<CSSValuePair>()
            new (pair) CSSValuePair()
            pair.first = value
            pair.second = second;
            value.kind = CSSValueKind.Pair
            value.data = pair
        }
    }
}

func (cssParser : &mut CSSParser) parseGridColumn(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    cssParser.parseGridLineOrPair(parser, builder, value)
}

func (cssParser : &mut CSSParser) parseGridRow(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    cssParser.parseGridLineOrPair(parser, builder, value)
}

func (cssParser : &mut CSSParser) parseGridArea(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    // grid-area can have up to 4 values separated by /
    cssParser.parseGridLine(parser, builder, value)
    var current = value
    while(parser.increment_if(TokenType.Divide as int)) {
        var next = CSSValue()
        if(cssParser.parseGridLine(parser, builder, next)) {
            var pair = builder.allocate<CSSValuePair>()
            new (pair) CSSValuePair {
                first = *current,
                second = next
            }
            current.kind = CSSValueKind.Pair
            current.data = pair
        } else {
            break
        }
    }
}

func (cssParser : &mut CSSParser) parseGridTemplateRows(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    cssParser.parseGridTemplateTracks(parser, builder, value)
}

func (cssParser : &mut CSSParser) parseGridTemplateColumns(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    cssParser.parseGridTemplateTracks(parser, builder, value)
}
