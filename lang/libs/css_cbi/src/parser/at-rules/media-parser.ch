
// Helper function to parse media type from identifier
func parseMediaType(token : &Token) : MediaType {
    const hash = fnv1_hash_view(token.value)
    switch(hash) {
        comptime_fnv1_hash("all") => { return MediaType.All }
        comptime_fnv1_hash("screen") => { return MediaType.Screen }
        comptime_fnv1_hash("print") => { return MediaType.Print }
        comptime_fnv1_hash("speech") => { return MediaType.Speech }
        default => { return MediaType.Unknown }
    }
}

// Parse a media feature value (can be length, number, identifier, etc.)
func (cssParser : &mut CSSParser) parseMediaFeatureValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken()
    switch(token.type) {
        TokenType.Number => {
            parser.increment()
            alloc_value_length(parser, builder, value, token.value)
        }
        TokenType.Identifier => {
            parser.increment()
            alloc_value_keyword(builder, value, CSSKeywordKind.Unknown, token.value)
        }
        default => {
            parser.error("expected a value for media feature")
        }
    }
}

// Parse comparison operator
func parseComparisonOperator(parser : *mut Parser) : MediaFeatureComparison {
    const token = parser.getToken()
    switch(token.type) {
        TokenType.Equal => {
            parser.increment()
            return MediaFeatureComparison.Equal
        }
        TokenType.LessThan => {
            parser.increment()
            // Check for <=
            const next = parser.getToken()
            if(next.type == TokenType.Equal) {
                parser.increment()
                return MediaFeatureComparison.LessThanEqual
            }
            return MediaFeatureComparison.LessThan
        }
        TokenType.GreaterThan => {
            parser.increment()
            // Check for >=
            const next = parser.getToken()
            if(next.type == TokenType.Equal) {
                parser.increment()
                return MediaFeatureComparison.GreaterThanEqual
            }
            return MediaFeatureComparison.GreaterThan
        }
        default => {
            return MediaFeatureComparison.None
        }
    }
}

// Parse a media feature: (feature-name: value) or (min-feature: value) or (value < feature < value)
func (cssParser : &mut CSSParser) parseMediaFeature(
    parser : *mut Parser,
    builder : *mut ASTBuilder
) : *mut MediaFeature {
    
    const feature = builder.allocate<MediaFeature>()
    new (feature) MediaFeature()
    
    // Expect opening parenthesis
    if(!parser.increment_if(TokenType.LParen as int)) {
        parser.error("expected '(' for media feature")
        return feature
    }
    
    var token = parser.getToken()
    
    // Check for range syntax: value < feature
    if(token.type == TokenType.Number) {
        // Range syntax: left value first
        cssParser.parseMediaFeatureValue(parser, builder, feature.leftValue)
        
        // Parse left operator
        feature.leftOp = parseComparisonOperator(parser)
        if(feature.leftOp == MediaFeatureComparison.None) {
            parser.error("expected comparison operator in range media query")
        }
        
        // Parse feature name
        token = parser.getToken()
        if(token.type == TokenType.Identifier || token.type == TokenType.PropertyName) {
            feature.name = builder.allocate_view(token.value)
            parser.increment()
        } else {
            parser.error("expected feature name in media query")
        }
        
        // Check for second comparison (e.g., width < 700px)
        feature.rightOp = parseComparisonOperator(parser)
        if(feature.rightOp != MediaFeatureComparison.None) {
            cssParser.parseMediaFeatureValue(parser, builder, feature.rightValue)
        }
        
    } else if(token.type == TokenType.Identifier || token.type == TokenType.PropertyName) {
        // Standard syntax: feature-name or feature-name: value
        feature.name = builder.allocate_view(token.value)
        parser.increment()
        
        token = parser.getToken()
        
        if(token.type == TokenType.Colon) {
            parser.increment()
            // Parse value
            cssParser.parseMediaFeatureValue(parser, builder, feature.value)
        } else {
            // Check for range syntax: feature > value
            feature.leftOp = parseComparisonOperator(parser)
            if(feature.leftOp != MediaFeatureComparison.None) {
                cssParser.parseMediaFeatureValue(parser, builder, feature.leftValue)
                
                // Check for second comparison
                feature.rightOp = parseComparisonOperator(parser)
                if(feature.rightOp != MediaFeatureComparison.None) {
                    cssParser.parseMediaFeatureValue(parser, builder, feature.rightValue)
                }
            }
            // Else: boolean feature like (color) with no value
        }
    }
    
    // Expect closing parenthesis
    if(!parser.increment_if(TokenType.RParen as int)) {
        parser.error("expected ')' after media feature")
    }
    
    return feature
}

// Parse media condition (recursive for 'and'/'or' chains)
func (cssParser : &mut CSSParser) parseMediaCondition(
    parser : *mut Parser,
    builder : *mut ASTBuilder
) : *mut MediaCondition {
    
    const condition = builder.allocate<MediaCondition>()
    new (condition) MediaCondition()
    
    // Check for 'not'
    var token = parser.getToken()
    if(token.type == TokenType.Identifier) {
        if(fnv1_hash_view(token.value) == comptime_fnv1_hash("not")) {
            condition.isNot = true
            parser.increment()
        }
    }
    
    // Parse media feature
    condition.feature = cssParser.parseMediaFeature(parser, builder)
    
    // Check for 'and'/'or'
    token = parser.getToken()
    if(token.type == TokenType.Identifier) {
        const hash = fnv1_hash_view(token.value)
        if(hash == comptime_fnv1_hash("and")) {
            condition.op = MediaConditionOp.And
            parser.increment()
            condition.next = cssParser.parseMediaCondition(parser, builder)
        } else if(hash == comptime_fnv1_hash("or")) {
            condition.op = MediaConditionOp.Or
            parser.increment()
            condition.next = cssParser.parseMediaCondition(parser, builder)
        }
    }
    
    return condition
}

// Parse a single media query
func (cssParser : &mut CSSParser) parseMediaQuery(
    parser : *mut Parser,
    builder : *mut ASTBuilder
) : *mut MediaQuery {
    
    const query = builder.allocate<MediaQuery>()
    new (query) MediaQuery()
    
    var token = parser.getToken()
    
    // Check for 'only' or 'not' modifier
    if(token.type == TokenType.Identifier || token.type == TokenType.PropertyName) {
        const hash = fnv1_hash_view(token.value)
        if(hash == comptime_fnv1_hash("only")) {
            query.modifier = MediaModifier.Only
            parser.increment()
            token = parser.getToken()
        } else if(hash == comptime_fnv1_hash("not")) {
            query.modifier = MediaModifier.Not
            parser.increment()
            token = parser.getToken()
        }
    }
    
    // Check for media type
    if(token.type == TokenType.Identifier || token.type == TokenType.PropertyName) {
        const mediaType = parseMediaType(*token)
        if(mediaType != MediaType.Unknown) {
            query.mediaType = mediaType
            parser.increment()
            token = parser.getToken()
            
            // Check for 'and' before condition
            if(token.type == TokenType.Identifier || token.type == TokenType.PropertyName) {
                if(fnv1_hash_view(token.value) == comptime_fnv1_hash("and")) {
                    parser.increment()
                    query.condition = cssParser.parseMediaCondition(parser, builder)
                }
            }
        } else if(query.modifier == MediaModifier.None) {
            // Could be a custom media type or start of condition
            // If next token is 'and' or we see '(', it's a condition
            const next = token + 1
            if(next.type == TokenType.LParen) {
                // This is a media feature, parse as condition
                query.condition = cssParser.parseMediaCondition(parser, builder)
            } else {
                // Custom media type
                query.customType = builder.allocate_view(token.value)
                parser.increment()
            }
        }
    } else if(token.type == TokenType.LParen) {
        // Media query starts with a feature (no media type)
        query.condition = cssParser.parseMediaCondition(parser, builder)
    }
    
    return query
}

// Parse media query list (comma-separated queries)
func (cssParser : &mut CSSParser) parseMediaQueryList(
    parser : *mut Parser,
    builder : *mut ASTBuilder
) : *mut MediaQueryList {
    
    const queryList = builder.allocate<MediaQueryList>()
    new (queryList) MediaQueryList()
    
    // Parse first query
    var query = cssParser.parseMediaQuery(parser, builder)
    queryList.queries.push(query)
    
    // Parse additional queries separated by commas
    while(true) {
        const token = parser.getToken()
        if(token.type == TokenType.Comma) {
            parser.increment()
            query = cssParser.parseMediaQuery(parser, builder)
            queryList.queries.push(query)
        } else {
            break
        }
    }
    
    return queryList
}
