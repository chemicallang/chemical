
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
        keyframes : std::vector<*mut CSSKeyframesRule>(),
        dyn_values : std::vector<*mut Value>(),
        className : std::string_view(),
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
                    if(!cssParser.parseNestedRule(&mut *root, parser, builder)) {
                        parser.error("failed to parse declaration or nested rule");
                        keep_parsing = false;
                    }
                }
            }
            TokenType.At => {
                const next_token = token + 1;
                if(next_token.type == TokenType.PropertyName || next_token.type == TokenType.Identifier) {
                    const hash = fnv1_hash_view(&next_token.value);
                    if(hash == comptime_fnv1_hash("media")) {
                        if(!cssParser.parseMediaRule(&mut *root, parser, builder)) {
                            keep_parsing = false;
                        }
                    } else if(hash == comptime_fnv1_hash("keyframes")) {
                        if(!cssParser.parseKeyframesRule(&mut *root, parser, builder)) {
                            keep_parsing = false;
                        }
                    } else {
                        parser.error("unsupported at-rule");
                        keep_parsing = false;
                    }
                } else {
                    parser.error("expected identifier after '@'");
                    keep_parsing = false;
                }
            }
            TokenType.Ampersand, TokenType.ClassName, TokenType.Id, TokenType.Colon, TokenType.LBrace, TokenType.Multiply => {
                if(!cssParser.parseNestedRule(&mut *root, parser, builder)) {
                    parser.error("failed to parse nested rule");
                    keep_parsing = false;
                }
            }
            default => {
                keep_parsing = false;
            }
        }
    }

    root.dyn_values = std::replace(&mut cssParser.dyn_values, std::vector<*mut Value>());

    return root;
}