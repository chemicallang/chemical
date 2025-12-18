
struct CSSNestedRule {
    var selector : *mut SelectorList
    var declarations : std::vector<*mut CSSDeclaration>
    var nested_rules : std::vector<*mut CSSNestedRule>
}

func parseCSSOM(parser : *mut Parser, builder : *mut ASTBuilder) : *CSSOM {
    var root = builder.allocate<CSSOM>()
    new (root) CSSOM {
        parent : parser.getParentNode(),
        declarations : std::vector<*mut CSSDeclaration>(),
        media_queries : std::vector<*mut CSSMediaRule>(),
        dyn_values : std::vector<*mut Value>(),
        className : std::string_view(),
        global : GlobalBlock(),
        support : SymResSupport()
    }
    var cssParser = CSSParser();
    var keep_parsing = true;
    while(keep_parsing) {
        const token = parser.getToken();
        switch(token.type) {
            TokenType.PropertyName, TokenType.Identifier => {
                const decl = cssParser.parseDeclaration(parser, builder);
                if(decl) {
                    root.declarations.push(decl);
                } else {
                    if(!cssParser.parseNestedRule(*root, parser, builder)) {
                        parser.error("failed to parse declaration or nested rule");
                        keep_parsing = false;
                    }
                }
            }
            TokenType.At => {
                if(!cssParser.parseMediaRule(*root, parser, builder)) {
                    parser.error("failed to parse media rule");
                    keep_parsing = false;
                    // Try generic at-rule fallback if media rule parsing failed?
                    // Currently parseMediaRule errors if it fails.
                }
            }
            TokenType.Ampersand, TokenType.ClassName, TokenType.Id, TokenType.Colon, TokenType.LBrace, TokenType.Multiply => {
                if(!cssParser.parseNestedRule(*root, parser, builder)) {
                    parser.error("failed to parse nested rule");
                    keep_parsing = false;
                }
            }
            default => {
                keep_parsing = false;
            }
        }
    }

    root.dyn_values = std::replace(cssParser.dyn_values, std::vector<*mut Value>());

    cssParser.parseAtRule(*root, parser, builder)
    return root;
}