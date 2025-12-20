
// Serialize media type to string
func writeMediaType(type : MediaType, customType : std::string_view, str : &mut std::string) {
    switch(type) {
        MediaType.All => {
            str.append_view(std::string_view("all"))
        }
        MediaType.Screen => {
            str.append_view(std::string_view("screen"))
        }
        MediaType.Print => {
            str.append_view(std::string_view("print"))
        }
        MediaType.Speech => {
            str.append_view(std::string_view("speech"))
        }
        MediaType.Unknown => {
            if(!customType.empty()) {
                str.append_view(customType)
            }
        }
    }
}

// Serialize comparison operator to string
func writeComparisonOperator(op : MediaFeatureComparison, str : &mut std::string) {
    switch(op) {
        MediaFeatureComparison.Equal => {
            str.append('=')
        }
        MediaFeatureComparison.LessThan => {
            str.append('<')
        }
        MediaFeatureComparison.LessThanEqual => {
            str.append_view(std::string_view("<="))
        }
        MediaFeatureComparison.GreaterThan => {
            str.append('>')
        }
        MediaFeatureComparison.GreaterThanEqual => {
            str.append_view(std::string_view(">="))
        }
        MediaFeatureComparison.None => {
            // No operator
        }
    }
}

// Serialize media feature to string
func (converter : &mut ASTConverter) writeMediaFeature(feature : *mut MediaFeature, str : &mut std::string) {
    str.append('(')
    
    // Check if this is range syntax
    if(feature.leftOp != MediaFeatureComparison.None) {
        // Range syntax: leftValue op feature [op rightValue]
        if(feature.leftValue.kind != CSSValueKind.Unknown) {
            converter.writeValue(feature.leftValue)
            str.append(' ')
            writeComparisonOperator(feature.leftOp, str)
            str.append(' ')
        }
        
        str.append_view(feature.name)
        
        if(feature.rightOp != MediaFeatureComparison.None && feature.rightValue.kind != CSSValueKind.Unknown) {
            str.append(' ')
            writeComparisonOperator(feature.rightOp, str)
            str.append(' ')
            converter.writeValue(feature.rightValue)
        }
    } else {
        // Legacy syntax: feature-name or feature-name: value
        str.append_view(feature.name)
        
        if(feature.value.kind != CSSValueKind.Unknown) {
            str.append(':')
            str.append(' ')
            converter.writeValue(feature.value)
        }
    }
    
    str.append(')')
}

// Serialize media condition to string (recursive)
func (converter : &mut ASTConverter) writeMediaCondition(condition : *mut MediaCondition, str : &mut std::string) {
    if(condition == null) {
        return
    }
    
    if(condition.isNot) {
        str.append_view(std::string_view("not "))
    }
    
    if(condition.feature != null) {
        converter.writeMediaFeature(condition.feature, str)
    }
    
    if(condition.next != null) {
        switch(condition.op) {
            MediaConditionOp.And => {
                str.append_view(std::string_view(" and "))
            }
            MediaConditionOp.Or => {
                str.append_view(std::string_view(" or "))
            }
            MediaConditionOp.None => {
                // No operator
            }
        }
        converter.writeMediaCondition(condition.next, str)
    }
}

// Serialize a single media query to string
func (converter : &mut ASTConverter) writeMediaQuery(query : *mut MediaQuery, str : &mut std::string) {
    // Write modifier
    switch(query.modifier) {
        MediaModifier.Only => {
            str.append_view(std::string_view("only "))
        }
        MediaModifier.Not => {
            str.append_view(std::string_view("not "))
        }
        MediaModifier.None => {
            // No modifier
        }
    }
    
    // Write media type
    if(query.mediaType != MediaType.Unknown || !query.customType.empty()) {
        writeMediaType(query.mediaType, query.customType, str)
        
        if(query.condition != null) {
            str.append_view(std::string_view(" and "))
        }
    }
    
    // Write condition
    if(query.condition != null) {
        converter.writeMediaCondition(query.condition, str)
    }
}

// Serialize media query list to string
func (converter : &mut ASTConverter) writeMediaQueryList(queryList : *mut MediaQueryList, str : &mut std::string) {
    var i : uint = 0
    const size = queryList.queries.size()
    
    while(i < size) {
        if(i > 0) {
            str.append_view(std::string_view(", "))
        }
        
        var query = queryList.queries.get(i)
        converter.writeMediaQuery(query, str)
        
        i++
    }
}
