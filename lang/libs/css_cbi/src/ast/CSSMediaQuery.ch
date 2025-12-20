
// Media query types (screen, print, all, etc.)
enum MediaType {
    Unknown,
    All,
    Screen,
    Print,
    Speech
}

// Media query modifiers (only, not)
enum MediaModifier {
    None,
    Only,
    Not
}

// Media condition operators (and, or)
enum MediaConditionOp {
    None,
    And,
    Or
}

// Media feature comparison operators for range syntax
enum MediaFeatureComparison {
    None,           // No comparison (boolean feature or legacy syntax)
    Equal,          // =
    LessThan,       // <
    LessThanEqual,  // <=
    GreaterThan,    // >
    GreaterThanEqual // >=
}

// A media feature like (width: 500px) or (min-width: 400px) or (400px <= width <= 700px)
struct MediaFeature {
    
    var name : std::string_view
    
    // For legacy min-/max- syntax or simple feature queries
    var value : CSSValue
    
    // For range syntax: left comparison operator
    var leftOp : MediaFeatureComparison
    
    // For range syntax: left value (e.g., "400px" in "400px <= width")
    var leftValue : CSSValue
    
    // For range syntax: right comparison operator
    var rightOp : MediaFeatureComparison
    
    // For range syntax: right value (e.g., "700px" in "width <= 700px")
    var rightValue : CSSValue
    
    @make
    func make() {
        leftOp = MediaFeatureComparison.None
        rightOp = MediaFeatureComparison.None
    }
    
}

// A media condition is a combination of features with logical operators
// Example: (min-width: 400px) and (max-width: 700px)
struct MediaCondition {
    
    // The feature for this condition node
    var feature : *mut MediaFeature
    
    // Logical operator combining this with the next condition
    var op : MediaConditionOp
    
    // Next condition in the chain (for 'and'/'or' combinations)
    var next : *mut MediaCondition
    
    // 'not' modifier for this condition
    var isNot : bool
    
    @make
    func make() {
        feature = null
        op = MediaConditionOp.None
        next = null
        isNot = false
    }
    
}

// A single media query: [only|not]? mediaType? [and condition]*
// Examples:
//   screen
//   screen and (min-width: 400px)
//   only screen and (color)
//   (min-width: 400px)
struct MediaQuery {
    
    var modifier : MediaModifier
    
    var mediaType : MediaType
    
    // Custom media type name if mediaType is Unknown
    var customType : std::string_view
    
    // The condition chain (multiple features combined with and/or)
    var condition : *mut MediaCondition
    
    @make
    func make() {
        modifier = MediaModifier.None
        mediaType = MediaType.Unknown
        condition = null
    }
    
}

// A media query list is a comma-separated list of media queries
// Example: screen and (color), projection and (color)
struct MediaQueryList {
    
    var queries : std::vector<*mut MediaQuery>
    
}
