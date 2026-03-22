func (jsParser : &mut JsParser) parseBlock(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
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
        var stmt = jsParser.parseStatement(parser, builder);
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

func (jsParser : &mut JsParser) parseStatement(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    const token = parser.getToken();
    if(token.type == JsTokenType.Var as int || token.type == JsTokenType.Const as int || token.type == JsTokenType.Let as int || token.type == JsTokenType.State as int) {
        var keyword = builder.allocate_view(token.value);
        parser.increment();

        var name = std::string_view();
        var pattern : *mut JsNode = null;

        if(parser.getToken().type == JsTokenType.LBracket as int) {
            pattern = jsParser.parseArrayDestructuring(parser, builder);
        } else if(parser.getToken().type == JsTokenType.Identifier as int) {
            name = builder.allocate_view(parser.getToken().value);
            parser.increment();
        } else {
            parser.error("expected identifier or [");
            return null;
        }

        if(parser.increment_if(JsTokenType.Equal as int)) {
            var val = jsParser.parseExpression(parser, builder);
            var varDecl = builder.allocate<JsVarDecl>()
            new (varDecl) JsVarDecl {
                base : JsNode { kind : JsNodeKind.VarDecl },
                name : name,
                pattern : pattern,
                value : val,
                keyword : keyword
            }
            parser.increment_if(JsTokenType.SemiColon as int);
            return varDecl as *mut JsNode;
        } else {
            var varDecl = builder.allocate<JsVarDecl>()
            new (varDecl) JsVarDecl {
                base : JsNode { kind : JsNodeKind.VarDecl },
                name : name,
                pattern : pattern,
                value : null,
                keyword : keyword
            }
            parser.increment_if(JsTokenType.SemiColon as int);
            return varDecl as *mut JsNode;
        }
    } else if(token.type == JsTokenType.If as int) {
        parser.increment();
        if(!parser.increment_if(JsTokenType.LParen as int)) {
            parser.error("expected ( after if");
        }
        var condition = jsParser.parseExpression(parser, builder);
        if(!parser.increment_if(JsTokenType.RParen as int)) {
            parser.error("expected ) after if condition");
        }
        var thenBlock = jsParser.parseStatement(parser, builder);
        var elseBlock : *mut JsNode = null;
        if(parser.increment_if(JsTokenType.Else as int)) {
            elseBlock = jsParser.parseStatement(parser, builder);
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
        if(parser.getToken().type != JsTokenType.SemiColon as int &&
           parser.getToken().type != JsTokenType.RBrace as int &&
           parser.getToken().type != JsTokenType.EndOfFile as int) {
            val = jsParser.parseExpression(parser, builder);
        }
        var ret = builder.allocate<JsReturn>()
        new (ret) JsReturn {
            base : JsNode { kind : JsNodeKind.Return },
            value : val
        }
        parser.increment_if(JsTokenType.SemiColon as int);
        return ret as *mut JsNode;
    } else if(token.type == JsTokenType.Class as int) {
        return jsParser.parseClassDecl(parser, builder);
    } else if(token.type == JsTokenType.Async as int) {
        parser.increment(); // consume async
        if(parser.getToken().type != JsTokenType.Function as int) {
             parser.error("expected function after async");
             return null;
        }
        parser.increment(); // consume function

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

        var body = jsParser.parseBlock(parser, builder);

        var funcDecl = builder.allocate<JsFunctionDecl>()
        new (funcDecl) JsFunctionDecl {
            base : JsNode { kind : JsNodeKind.FunctionDecl },
            name : name,
            params : params,
            body : body,
            is_async : true,
            is_generator : false
        }
        return funcDecl as *mut JsNode;
    } else if(token.type == JsTokenType.Function as int) {
        parser.increment();
        var is_generator = false;
        if(parser.getToken().type == JsTokenType.Star as int) {
             parser.increment();
             is_generator = true;
        }

        const idToken = parser.getToken();
        var name = std::string_view();
        if(idToken.type == JsTokenType.Identifier as int) {
            name = builder.allocate_view(idToken.value);
            parser.increment();
        }

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

        var body = jsParser.parseBlock(parser, builder);

        var funcDecl = builder.allocate<JsFunctionDecl>()
        new (funcDecl) JsFunctionDecl {
            base : JsNode { kind : JsNodeKind.FunctionDecl },
            name : name,
            params : params,
            body : body,
            is_async : false,
            is_generator : is_generator
        }
        return funcDecl as *mut JsNode;
    } else if(token.type == JsTokenType.For as int) {
        parser.increment();
        if(!parser.increment_if(JsTokenType.LParen as int)) {
            parser.error("expected ( after for");
        }

        var initNode : *mut JsNode = null;
        var is_decl = false;
        if(parser.getToken().type != JsTokenType.SemiColon as int) {
            if(parser.getToken().type == JsTokenType.Var as int ||
               parser.getToken().type == JsTokenType.Const as int ||
               parser.getToken().type == JsTokenType.Let as int ||
               parser.getToken().type == JsTokenType.State as int) {
                initNode = jsParser.parseStatement(parser, builder);
                is_decl = true;
            } else {
                var expr = jsParser.parseExpression(parser, builder);
                if(expr != null) {
                    // Don't consume semicolon yet, we check next token
                    var stmt = builder.allocate<JsExpressionStatement>()
                    new (stmt) JsExpressionStatement {
                         base : JsNode { kind : JsNodeKind.ExpressionStatement },
                         expression : expr
                    }
                    initNode = stmt as *mut JsNode;
                }
            }
        }

        // Check for 'in' or 'of'

        if(parser.getToken().type == JsTokenType.In as int || parser.getToken().type == JsTokenType.Of as int) {
             const is_in = parser.getToken().type == JsTokenType.In as int;
             parser.increment(); // consume in/of

             var right = jsParser.parseExpression(parser, builder);
             if(!parser.increment_if(JsTokenType.RParen as int)) {
                 parser.error("expected )");
             }
             var body = jsParser.parseStatement(parser, builder);

             if(is_in) {
                 var forIn = builder.allocate<JsForIn>()
                 new (forIn) JsForIn {
                     base : JsNode { kind : JsNodeKind.ForIn },
                     left : initNode,
                     right : right,
                     body : body
                 }
                 return forIn as *mut JsNode;
             } else {
                 var forOf = builder.allocate<JsForOf>()
                 new (forOf) JsForOf {
                     base : JsNode { kind : JsNodeKind.ForOf },
                     left : initNode,
                     right : right,
                     body : body
                 }
                 return forOf as *mut JsNode;
             }
        }

        // Standard For Loop

        if(parser.getToken().type == JsTokenType.SemiColon as int) {
            parser.increment();
        } else if(!is_decl) {
             // If valid loop, semicolon required.
        }

        var condition : *mut JsNode = null;
        if(parser.getToken().type != JsTokenType.SemiColon as int) {
            condition = jsParser.parseExpression(parser, builder);
        }
        if(!parser.increment_if(JsTokenType.SemiColon as int)) {
            parser.error("expected ; after condition");
        }

        var update : *mut JsNode = null;
        if(parser.getToken().type != JsTokenType.RParen as int) {
            update = jsParser.parseExpression(parser, builder);
        }
        if(!parser.increment_if(JsTokenType.RParen as int)) {
            parser.error("expected )");
        }

        var body = jsParser.parseStatement(parser, builder);

        var forStmt = builder.allocate<JsFor>()
        new (forStmt) JsFor {
            base : JsNode { kind : JsNodeKind.For },
            init : initNode,
            condition : condition,
            update : update,
            body : body
        }
        return forStmt as *mut JsNode;
    } else if(token.type == JsTokenType.While as int) {
        parser.increment();
        if(!parser.increment_if(JsTokenType.LParen as int)) {
            parser.error("expected ( after while");
        }
        var condition = jsParser.parseExpression(parser, builder);
        if(!parser.increment_if(JsTokenType.RParen as int)) {
            parser.error("expected )");
        }
        var body = jsParser.parseStatement(parser, builder);
        var whileStmt = builder.allocate<JsWhile>()
        new (whileStmt) JsWhile {
            base : JsNode { kind : JsNodeKind.While },
            condition : condition,
            body : body
        }
        return whileStmt as *mut JsNode;
    } else if(token.type == JsTokenType.Break as int) {
        parser.increment();
        parser.increment_if(JsTokenType.SemiColon as int);
        var breakStmt = builder.allocate<JsBreak>()
        new (breakStmt) JsBreak {
            base : JsNode { kind : JsNodeKind.Break }
        }
        return breakStmt as *mut JsNode;
    } else if(token.type == JsTokenType.Continue as int) {
        parser.increment();
        parser.increment_if(JsTokenType.SemiColon as int);
        var continueStmt = builder.allocate<JsContinue>()
        new (continueStmt) JsContinue {
            base : JsNode { kind : JsNodeKind.Continue }
        }
        return continueStmt as *mut JsNode;
    } else if(token.type == JsTokenType.Do as int) {
        parser.increment();
        var body = jsParser.parseStatement(parser, builder);
        if(!parser.increment_if(JsTokenType.While as int)) {
            parser.error("expected while after do body");
        }
        if(!parser.increment_if(JsTokenType.LParen as int)) {
            parser.error("expected ( after while");
        }
        var condition = jsParser.parseExpression(parser, builder);
        if(!parser.increment_if(JsTokenType.RParen as int)) {
            parser.error("expected ) after condition");
        }
        parser.increment_if(JsTokenType.SemiColon as int);
        var doWhile = builder.allocate<JsDoWhile>()
        new (doWhile) JsDoWhile {
            base : JsNode { kind : JsNodeKind.DoWhile },
            condition : condition,
            body : body
        }
        return doWhile as *mut JsNode;
    } else if(token.type == JsTokenType.Switch as int) {
        parser.increment();
        if(!parser.increment_if(JsTokenType.LParen as int)) {
            parser.error("expected ( after switch");
        }
        var discriminant = jsParser.parseExpression(parser, builder);
        if(!parser.increment_if(JsTokenType.RParen as int)) {
            parser.error("expected ) after switch discriminant");
        }
        if(!parser.increment_if(JsTokenType.LBrace as int)) {
            parser.error("expected { for switch body");
        }
        var switchStmt = builder.allocate<JsSwitch>()
        new (switchStmt) JsSwitch {
            base : JsNode { kind : JsNodeKind.Switch },
            discriminant : discriminant,
            cases : std::vector<JsCase>()
        }
        var cases = &mut switchStmt.cases;
        while(parser.getToken().type != JsTokenType.RBrace as int) {
            if(parser.getToken().type == JsTokenType.Case as int) {
                parser.increment();
                var test = jsParser.parseExpression(parser, builder);
                if(!parser.increment_if(JsTokenType.Colon as int)) {
                    parser.error("expected : after case");
                }
                cases.push(JsCase { test : test, body : std::vector<*mut JsNode>() });
                const casePtr = cases.last_ptr()
                while(parser.getToken().type != JsTokenType.Case as int &&
                      parser.getToken().type != JsTokenType.Default as int &&
                      parser.getToken().type != JsTokenType.RBrace as int) {
                    var stmt = jsParser.parseStatement(parser, builder);
                    if(stmt != null) casePtr.body.push(stmt); else break;
                }
            } else if(parser.getToken().type == JsTokenType.Default as int) {
                parser.increment();
                if(!parser.increment_if(JsTokenType.Colon as int)) {
                    parser.error("expected : after default");
                }
                cases.push(JsCase { test : null, body : std::vector<*mut JsNode>() });
                const casePtr = cases.last_ptr()
                while(parser.getToken().type != JsTokenType.Case as int &&
                      parser.getToken().type != JsTokenType.Default as int &&
                      parser.getToken().type != JsTokenType.RBrace as int) {
                    var stmt = jsParser.parseStatement(parser, builder);
                    if(stmt != null) casePtr.body.push(stmt); else break;
                }

            } else {
                break;
            }
        }
        if(!parser.increment_if(JsTokenType.RBrace as int)) {
            parser.error("expected closing }");
        }
        return switchStmt as *mut JsNode;
    } else if(token.type == JsTokenType.Throw as int) {
        parser.increment();
        var arg = jsParser.parseExpression(parser, builder);
        parser.increment_if(JsTokenType.SemiColon as int);
        var throwStmt = builder.allocate<JsThrow>()
        new (throwStmt) JsThrow {
            base : JsNode { kind : JsNodeKind.Throw },
            argument : arg
        }
        return throwStmt as *mut JsNode;
    } else if(token.type == JsTokenType.Try as int) {
        parser.increment();
        var tryBlock = jsParser.parseBlock(parser, builder);
        var catchParam = std::string_view();
        var catchBlock : *mut JsNode = null;
        var finallyBlock : *mut JsNode = null;

        if(parser.increment_if(JsTokenType.Catch as int)) {
            if(parser.increment_if(JsTokenType.LParen as int)) {
                const paramToken = parser.getToken();
                if(paramToken.type == JsTokenType.Identifier as int) {
                    catchParam = builder.allocate_view(paramToken.value);
                    parser.increment();
                }
                if(!parser.increment_if(JsTokenType.RParen as int)) {
                    parser.error("expected )");
                }
            }
            catchBlock = jsParser.parseBlock(parser, builder);
        }

        if(parser.increment_if(JsTokenType.Finally as int)) {
            finallyBlock = jsParser.parseBlock(parser, builder);
        }

        var tryCatch = builder.allocate<JsTryCatch>()
        new (tryCatch) JsTryCatch {
            base : JsNode { kind : JsNodeKind.TryCatch },
            tryBlock : tryBlock,
            catchParam : catchParam,
            catchBlock : catchBlock,
            finallyBlock : finallyBlock
        }
        return tryCatch as *mut JsNode;
    } else if(token.type == JsTokenType.Import as int) {
        return jsParser.parseImport(parser, builder);
    } else if(token.type == JsTokenType.Export as int) {
        return jsParser.parseExport(parser, builder);
    } else if(token.type == JsTokenType.Debugger as int) {
        parser.increment();
        parser.increment_if(JsTokenType.SemiColon as int);
        var dbg = builder.allocate<JsDebugger>()
        new (dbg) JsDebugger {
            base : JsNode { kind : JsNodeKind.Debugger }
        }
        return dbg as *mut JsNode;
    } else if(token.type == JsTokenType.LBrace as int) {
        return jsParser.parseBlock(parser, builder);
    } else {
        // Expression statement
        var expr = jsParser.parseExpression(parser, builder);
        if(expr != null) {
            parser.increment_if(JsTokenType.SemiColon as int);
            var stmt = builder.allocate<JsExpressionStatement>()
            new (stmt) JsExpressionStatement {
                base : JsNode { kind : JsNodeKind.ExpressionStatement },
                expression : expr
            }
            return stmt as *mut JsNode;
        }
    }

    parser.error("unexpected token");
    parser.increment();
    return null;
}
