
public enum JsTokenType {
    Var = 100,
    Identifier,
    Equal,
    Number,
    LBrace,
    RBrace,
    EndOfFile,
    ChemicalStart, // ${
    LParen,
    RParen,
    LBracket,
    RBracket,
    SemiColon,
    String,
    Function,
    Return,
    If,
    Else,
    Comma,
    Colon,
    Dot,
    Plus,
    Minus,
    Star,
    Slash,
    EqualEqual,
    NotEqual,
    LessThan,
    GreaterThan,
    LessThanEqual,
    GreaterThanEqual,
    Exclamation
}

func parsePrimary(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    const token = parser.getToken();
    if(token.type == JsTokenType.Number as int) {
        parser.increment();
        var literal = builder.allocate<JsLiteral>()
        new (literal) JsLiteral {
            base : JsNode { kind : JsNodeKind.Literal },
            value : builder.allocate_view(token.value)
        }
        return literal as *mut JsNode;
    } else if(token.type == JsTokenType.String as int) {
        parser.increment();
        var literal = builder.allocate<JsLiteral>()
        new (literal) JsLiteral {
            base : JsNode { kind : JsNodeKind.Literal },
            value : builder.allocate_view(token.value)
        }
        return literal as *mut JsNode;
    } else if(token.type == JsTokenType.Identifier as int) {
        parser.increment();
        var id = builder.allocate<JsIdentifier>()
        new (id) JsIdentifier {
            base : JsNode { kind : JsNodeKind.Identifier },
            value : builder.allocate_view(token.value)
        }
        
        // Check for function call
        if(parser.getToken().type == JsTokenType.LParen as int) {
            parser.increment(); // consume (
            var call = builder.allocate<JsFunctionCall>()
            new (call) JsFunctionCall {
                base : JsNode { kind : JsNodeKind.FunctionCall },
                callee : id as *mut JsNode,
                args : std::vector<*mut JsNode>()
            }
            
            if(parser.getToken().type != JsTokenType.RParen as int) {
                while(true) {
                    var arg = parseExpression(parser, builder);
                    if(arg != null) {
                        call.args.push(arg);
                    }
                    if(parser.getToken().type == JsTokenType.Comma as int) {
                        parser.increment();
                    } else {
                        break;
                    }
                }
            }
            
            if(!parser.increment_if(JsTokenType.RParen as int)) {
                parser.error("expected )");
            }
            return call as *mut JsNode;
        }
        
        return id as *mut JsNode;
    } else if(token.type == JsTokenType.ChemicalStart as int) {
        parser.increment();
        var val = parser.parseExpression(builder);
        if(!parser.increment_if(JsTokenType.RBrace as int)) {
            parser.error("expected } after chemical value");
        }
        var chem = builder.allocate<JsChemicalValue>()
        new (chem) JsChemicalValue {
            base : JsNode { kind : JsNodeKind.ChemicalValue },
            value : val
        }
        return chem as *mut JsNode;
    } else if(token.type == JsTokenType.LParen as int) {
        parser.increment();
        var expr = parseExpression(parser, builder);
        if(!parser.increment_if(JsTokenType.RParen as int)) {
            parser.error("expected )");
        }
        return expr;
    }
    
    parser.error("unexpected token in expression");
    return null;
}

func parseExpression(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    var left = parsePrimary(parser, builder);
    if(left == null) return null;
    
    // Simple binary op handling (left-associative, no precedence yet)
    while(true) {
        const token = parser.getToken();
        if(token.type == JsTokenType.Plus as int || 
           token.type == JsTokenType.Minus as int || 
           token.type == JsTokenType.Star as int || 
           token.type == JsTokenType.Slash as int ||
           token.type == JsTokenType.Equal as int ||
           token.type == JsTokenType.EqualEqual as int ||
           token.type == JsTokenType.NotEqual as int ||
           token.type == JsTokenType.LessThan as int ||
           token.type == JsTokenType.GreaterThan as int ||
           token.type == JsTokenType.LessThanEqual as int ||
           token.type == JsTokenType.GreaterThanEqual as int) { // Assignment is also handled here for simplicity
            
            var op = builder.allocate_view(token.value);
            parser.increment();
            
            var right = parsePrimary(parser, builder);
            var binOp = builder.allocate<JsBinaryOp>()
            new (binOp) JsBinaryOp {
                base : JsNode { kind : JsNodeKind.BinaryOp },
                left : left,
                right : right,
                op : op
            }
            left = binOp as *mut JsNode;
        } else {
            break;
        }
    }
    return left;
}

func parseBlock(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    if(!parser.increment_if(JsTokenType.LBrace as int)) {
        parser.error("expected {");
        return null;
    }
    
    var block = builder.allocate<JsBlock>()
    new (block) JsBlock {
        base : JsNode { kind : JsNodeKind.Block },
        statements : std::vector<*mut JsNode>()
    }
    
    while(true) {
        if(parser.getToken().type == JsTokenType.RBrace as int || parser.getToken().type == JsTokenType.EndOfFile as int) {
            break;
        }
        var stmt = parseStatement(parser, builder);
        if(stmt != null) {
            block.statements.push(stmt);
        } else {
            break;
        }
    }
    
    if(!parser.increment_if(JsTokenType.RBrace as int)) {
        parser.error("expected }");
    }
    return block as *mut JsNode;
}

func parseStatement(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    const token = parser.getToken();
    if(token.type == JsTokenType.Var as int) {
        parser.increment();
        const idToken = parser.getToken();
        if(idToken.type != JsTokenType.Identifier as int) {
            parser.error("expected identifier");
            return null;
        }
        var name = builder.allocate_view(idToken.value);
        parser.increment();
        
        if(parser.increment_if(JsTokenType.Equal as int)) {
            var val = parseExpression(parser, builder);
            var varDecl = builder.allocate<JsVarDecl>()
            new (varDecl) JsVarDecl {
                base : JsNode { kind : JsNodeKind.VarDecl },
                name : name,
                value : val
            }
            parser.increment_if(JsTokenType.SemiColon as int);
            return varDecl as *mut JsNode;
        } else {
            var varDecl = builder.allocate<JsVarDecl>()
            new (varDecl) JsVarDecl {
                base : JsNode { kind : JsNodeKind.VarDecl },
                name : name,
                value : null
            }
            parser.increment_if(JsTokenType.SemiColon as int);
            return varDecl as *mut JsNode;
        }
    } else if(token.type == JsTokenType.If as int) {
        parser.increment();
        if(!parser.increment_if(JsTokenType.LParen as int)) {
            parser.error("expected ( after if");
        }
        var condition = parseExpression(parser, builder);
        if(!parser.increment_if(JsTokenType.RParen as int)) {
            parser.error("expected ) after if condition");
        }
        var thenBlock = parseStatement(parser, builder);
        var elseBlock : *mut JsNode = null;
        if(parser.increment_if(JsTokenType.Else as int)) {
            elseBlock = parseStatement(parser, builder);
        }
        
        var ifStmt = builder.allocate<JsIf>()
        new (ifStmt) JsIf {
            base : JsNode { kind : JsNodeKind.If },
            condition : condition,
            thenBlock : thenBlock,
            elseBlock : elseBlock
        }
        return ifStmt as *mut JsNode;
    } else if(token.type == JsTokenType.Return as int) {
        parser.increment();
        var val : *mut JsNode = null;
        if(parser.getToken().type != JsTokenType.SemiColon as int) {
            val = parseExpression(parser, builder);
        }
        var ret = builder.allocate<JsReturn>()
        new (ret) JsReturn {
            base : JsNode { kind : JsNodeKind.Return },
            value : val
        }
        parser.increment_if(JsTokenType.SemiColon as int);
        return ret as *mut JsNode;
    } else if(token.type == JsTokenType.Function as int) {
        parser.increment();
        const idToken = parser.getToken();
        if(idToken.type != JsTokenType.Identifier as int) {
            parser.error("expected identifier");
            return null;
        }
        var name = builder.allocate_view(idToken.value);
        parser.increment();
        
        if(!parser.increment_if(JsTokenType.LParen as int)) {
            parser.error("expected (");
        }
        
        var params = std::vector<std::string_view>();
        if(parser.getToken().type != JsTokenType.RParen as int) {
            while(true) {
                const paramToken = parser.getToken();
                if(paramToken.type != JsTokenType.Identifier as int) {
                    parser.error("expected identifier");
                    break;
                }
                params.push(builder.allocate_view(paramToken.value));
                parser.increment();
                
                if(parser.getToken().type == JsTokenType.Comma as int) {
                    parser.increment();
                } else {
                    break;
                }
            }
        }
        
        if(!parser.increment_if(JsTokenType.RParen as int)) {
            parser.error("expected )");
        }
        
        var body = parseBlock(parser, builder);
        
        var funcDecl = builder.allocate<JsFunctionDecl>()
        new (funcDecl) JsFunctionDecl {
            base : JsNode { kind : JsNodeKind.FunctionDecl },
            name : name,
            params : params,
            body : body
        }
        return funcDecl as *mut JsNode;
    } else if(token.type == JsTokenType.LBrace as int) {
        return parseBlock(parser, builder);
    } else {
        // Expression statement
        var expr = parseExpression(parser, builder);
        if(expr != null) {
            parser.increment_if(JsTokenType.SemiColon as int);
            return expr;
        }
    }
    
    parser.error("unexpected token");
    parser.increment();
    return null;
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
        if(token.type == JsTokenType.RBrace as int || token.type == JsTokenType.EndOfFile as int) {
            break;
        }
        
        var stmt = parseStatement(parser, builder);
        if(stmt != null) {
            root.statements.push(stmt);
        } else {
            // Error recovery or break
            if(token.type == JsTokenType.EndOfFile as int) break;
        }
    }
    return root;
}
