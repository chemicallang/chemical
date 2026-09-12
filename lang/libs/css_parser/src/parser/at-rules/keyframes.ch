
func (cssParser : &mut CSSParser) parseKeyframesRule(om : &mut CSSOM, parser : *mut Parser, builder : *mut ASTBuilder) : bool {
    var token = parser.getToken();
    if(token.type == TokenType.At) {
        parser.increment();
    } else {
        return false;
    }

    token = parser.getToken();
    if(token.type == TokenType.PropertyName || token.type == TokenType.Identifier) {
        if(fnv1_hash_view(&token.value) == comptime_fnv1_hash("keyframes")) {
             parser.increment();
        } else {
            parser.error("expected 'keyframes'");
            return false;
        }
    } else {
        parser.error("expected identifier 'keyframes' after '@'");
        return false;
    }

    // Name of keyframes
    token = parser.getToken();
    var name = std::string_view();
    if(token.type == TokenType.Identifier || token.type == TokenType.PropertyName) {
        name = token.value;
        parser.increment();
    } else {
        parser.error("expected name for @keyframes");
        return false;
    }

    var rule = builder.allocate<CSSKeyframesRule>();
    new (rule) CSSKeyframesRule {
        name : builder.allocate_view(&name),
        keyframes : std::vector<*mut CSSKeyframe>(),
        parent : om.parent
    }
    om.keyframes.push(rule);

    if (parser.increment_if(TokenType.LBrace as int)) {
        while(true) {
            const body_token = parser.getToken();
            if(body_token.type == TokenType.RBrace) {
                parser.increment();
                break;
            }
            if(body_token.type == TokenType.EndOfFile) {
                parser.error("unexpected end of file in @keyframes block");
                return false;
            }

            // Keyframe selector (from, to, percentage) — supports comma-separated
            // lists like "0%, 80%, 100% { ... }"
            var selector = std::string();
            while(true) {
                const sel_token = parser.getToken();
                if(sel_token.type == TokenType.Identifier || sel_token.type == TokenType.PropertyName) {
                    selector.append_view(&sel_token.value);
                    parser.increment();
                } else if(sel_token.type == TokenType.Number) {
                    const start = sel_token.value.data();
                    var size = sel_token.value.size();
                    parser.increment();
                    const next_tok = parser.getToken();
                    if(next_tok.type == TokenType.Percentage) {
                        size += next_tok.value.size();
                        parser.increment();
                    }
                    var sel_view = std::string_view(start, size);
                    selector.append_view(&sel_view);
                } else if(sel_token.type == TokenType.Percentage) {
                    selector.append_view(&sel_token.value);
                    parser.increment();
                } else {
                    parser.error("expected keyframe selector (e.g., 'from', 'to', or '50%')");
                    return false;
                }
                // A comma means another selector follows in the list
                const comma_tok = parser.getToken();
                if(comma_tok.type == TokenType.Comma) {
                    parser.increment();
                    selector.append_view(", ");
                    continue;
                }
                break;
            }
            var selector_arena = selector.to_view();
            var selector_view = builder.allocate_view(&selector_arena);

            var keyframe = builder.allocate<CSSKeyframe>();
            new (keyframe) CSSKeyframe {
                selector : selector_view,
                declarations : std::vector<*mut CSSDeclaration>()
            }
            rule.keyframes.push(keyframe);

            if(parser.increment_if(TokenType.LBrace as int)) {
                while(true) {
                    const inner_token = parser.getToken();
                    if(inner_token.type == TokenType.RBrace) {
                        parser.increment();
                        break;
                    }
                    if(inner_token.type == TokenType.EndOfFile) {
                        parser.error("unexpected end of file in keyframe block");
                        return false;
                    }
                    
                    var decl = cssParser.parseDeclaration(parser, builder);
                    if(decl) {
                        keyframe.declarations.push(decl);
                    } else {
                        parser.error("expected declaration inside keyframe block");
                        return false;
                    }
                }
            } else {
                parser.error("expected '{' after keyframe selector");
                return false;
            }
        }
    } else {
        parser.error("expected '{' after @keyframes name");
        return false;
    }

    return true;
}
