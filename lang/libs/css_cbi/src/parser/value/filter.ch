
func (cssParser : &mut CSSParser) parseBackdropFilter(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    var firstFilter : *mut CSSBackdropFilterValueData = null
    var currentFilter : *mut CSSBackdropFilterValueData = null

    // value kind
    value.kind = CSSValueKind.BackdropFilter

    while(true) {
        const token = parser.getToken();
        if(token.type == TokenType.Identifier) {
             if(token.value.equals("none")) {
                 parser.increment()
                 // if none is the only value, we are done
                 // if mixed with others, it's invalid? standard says 'none' is exclusive
                 if(firstFilter == null) {
                     // store nothing or a specific none representation?
                     // value.data will be null effectively if we don't allocate
                     // but usually we set kind to None or Keyword.
                     // Here we'll just return with data=null but kind=BackdropFilter ?
                     // Or maybe we should parse it as a keyword?
                     // Let's allocation a filter with function "none"
                 }
                 // Handle 'none' as keyword
                 var noneVal = builder.allocate<CSSKeywordValueData>()
                 new (noneVal) CSSKeywordValueData { kind : CSSKeywordKind.None, value : token.value }
                 value.kind = CSSValueKind.Keyword
                 value.data = noneVal
                 return
             }
             
             var filter = builder.allocate<CSSBackdropFilterValueData>()
             new (filter) CSSBackdropFilterValueData {
                 function : CSSKeywordValueData { kind : CSSKeywordKind.Unknown, value : builder.allocate_view(token.value) },
                 arguments : std::vector<CSSValue>(),
                 next : null
             }
             
             if(firstFilter == null) {
                 firstFilter = filter
                 value.data = filter
             } else {
                 currentFilter.next = filter
             }
             currentFilter = filter
             
             parser.increment()
             
             // expect '('
             if(parser.increment_if(TokenType.LParen as int)) {
                 while(true) {
                     const argTok = parser.getToken()
                     if(argTok.type == TokenType.RParen) {
                         parser.increment()
                         break
                     }
                     
                     var argVal = CSSValue()
                     if(cssParser.parseLength(parser, builder, argVal)) {
                         filter.arguments.push(argVal)
                     } else if(argTok.type == TokenType.Percentage) {
                         parser.increment()
                         var pVal = builder.allocate<CSSLengthValueData>()
                         new (pVal) CSSLengthValueData { kind : CSSLengthKind.LengthPERCENTAGE, value : argTok.value }
                         argVal.kind = CSSValueKind.Length
                         argVal.data = pVal
                         filter.arguments.push(argVal)
                     } else if(argTok.type == TokenType.Identifier) {
                         parser.increment()
                         var kVal = builder.allocate<CSSKeywordValueData>()
                         new (kVal) CSSKeywordValueData { kind : CSSKeywordKind.Unknown, value : builder.allocate_view(argTok.value) }
                         argVal.kind = CSSValueKind.Keyword
                         argVal.data = kVal
                         filter.arguments.push(argVal)
                     } else if(argTok.type == TokenType.Number) {
                          parser.increment();
                          var nVal = builder.allocate<CSSLengthValueData>()
                          new (nVal) CSSLengthValueData { kind : CSSLengthKind.None, value : builder.allocate_view(argTok.value) }
                          argVal.kind = CSSValueKind.Length
                          argVal.data = nVal
                          filter.arguments.push(argVal)
                     } else {
                         // error or skip
                         break
                     }
                     
                     if(parser.increment_if(TokenType.Comma as int)) {
                         continue
                     }
                 }
             } else {
                 parser.error("expected '(' after filter function")
             }
             
        } else {
            break
        }
        
        // multiple filters separated by space? standard says space-separated list
        // so we loop
    }
}
