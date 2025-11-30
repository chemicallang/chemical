
public enum JsTokenType {
    Var = 100,
    Identifier,
    Equal,
    Number,
    LBrace,
    RBrace,
    EndOfFile
}

func parseJsRoot(parser : *mut Parser, builder : *mut ASTBuilder) : *JsRoot {
    var root = builder.allocate<JsRoot>()
    new (root) JsRoot {
        statements : std::vector<*mut JsNode>(),
        parent : parser.getParentNode(),
        support : SymResSupport {}
    }

    while(true) {
        const token = parser.getToken();
        if(token.type == JsTokenType.Var as int) {
            // var x = 10
            parser.increment(); // consume var
            
            const idToken = parser.getToken();
            if(idToken.type != JsTokenType.Identifier as int) {
                parser.error("expected identifier");
                break;
            }
            parser.increment(); // consume identifier
            
            const eqToken = parser.getToken();
            if(eqToken.type != JsTokenType.Equal as int) {
                parser.error("expected =");
                break;
            }
            parser.increment(); // consume =
            
            const numToken = parser.getToken();
            if(numToken.type != JsTokenType.Number as int) {
                parser.error("expected number");
                break;
            }
            parser.increment(); // consume number
            
            var literal = builder.allocate<JsLiteral>()
            new (literal) JsLiteral {
                base : JsNode { kind : JsNodeKind.Literal },
                value : builder.allocate_view(numToken.value)
            }
            
            var varDecl = builder.allocate<JsVarDecl>()
            new (varDecl) JsVarDecl {
                base : JsNode { kind : JsNodeKind.VarDecl },
                name : builder.allocate_view(idToken.value),
                value : literal as *mut JsNode
            }
            
            root.statements.push(varDecl as *mut JsNode)
            
        } else if (token.type == JsTokenType.RBrace as int || token.type == JsTokenType.EndOfFile as int) {
            break;
        } else {
            parser.error("unexpected token in js block")
            parser.increment(); // avoid infinite loop
        }
    }
    return root;
}
