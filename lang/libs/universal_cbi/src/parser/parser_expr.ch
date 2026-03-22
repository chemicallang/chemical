func (jsParser : &mut JsParser) parseArrowBody(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    const tokenType = parser.getToken().type
    if(tokenType == JsTokenType.LBrace as int || tokenType == JsTokenType.If as int) {
        return jsParser.parseStatement(parser, builder)
    } else {
        return jsParser.parseExpression(parser, builder)
    }
}

func (jsParser : &mut JsParser) parsePostfixContinuation(parser : *mut Parser, builder : *mut ASTBuilder, start : *mut JsNode) : *mut JsNode {
    var node = start
    while(true) {
        const t = parser.getToken();
        if(t.type == JsTokenType.Dot as int) {
            parser.increment();
            const idToken = parser.getToken();
            if(idToken.type != JsTokenType.Identifier as int && idToken.type != JsTokenType.Class as int) {
                parser.error("expected identifier after dot");
                break;
            }
            var prop = builder.allocate_view(idToken.value);
            parser.increment();

            var access = builder.allocate<JsMemberAccess>()
            new (access) JsMemberAccess {
                base : JsNode { kind : JsNodeKind.MemberAccess },
                object : node,
                property : prop
            }
            node = access as *mut JsNode;
        } else if(t.type == JsTokenType.LParen as int) {
            parser.increment(); // consume (
            var call = builder.allocate<JsFunctionCall>()
            new (call) JsFunctionCall {
                base : JsNode { kind : JsNodeKind.FunctionCall },
                callee : node,
                args : std::vector<*mut JsNode>()
            }

            if(parser.getToken().type != JsTokenType.RParen as int) {
                while(true) {
                    var arg = jsParser.parseExpression(parser, builder);
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
            node = call as *mut JsNode;
        } else if(t.type == JsTokenType.LBracket as int) {
            parser.increment(); // consume [
            var indexExpr = jsParser.parseExpression(parser, builder);
            if(!parser.increment_if(JsTokenType.RBracket as int)) {
                parser.error("expected ] after index");
            }
            var indexAcc = builder.allocate<JsIndexAccess>()
            new (indexAcc) JsIndexAccess {
                base : JsNode { kind : JsNodeKind.IndexAccess },
                object : node,
                index : indexExpr
            }
            node = indexAcc as *mut JsNode;
        } else if(t.type == JsTokenType.PlusPlus as int || t.type == JsTokenType.MinusMinus as int) {
            var op = builder.allocate_view(t.value);
            parser.increment();
            var unary = builder.allocate<JsUnaryOp>()
            new (unary) JsUnaryOp {
                base : JsNode { kind : JsNodeKind.UnaryOp },
                operator : op,
                operand : node,
                prefix : false
            }
            node = unary as *mut JsNode;
        } else {
            break;
        }
    }
    return node;
}

func (jsParser : &mut JsParser) parsePrimary(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    var node : *mut JsNode = null;
    const token = parser.getToken();

    // Check for unary operators first
    if(token.type == JsTokenType.Exclamation as int ||
       token.type == JsTokenType.Plus as int ||
       token.type == JsTokenType.Minus as int ||
       token.type == JsTokenType.PlusPlus as int ||
       token.type == JsTokenType.MinusMinus as int ||
       token.type == JsTokenType.BitwiseNot as int ||
       token.type == JsTokenType.Typeof as int ||
       token.type == JsTokenType.Void as int ||
       token.type == JsTokenType.Delete as int ||
       token.type == JsTokenType.New as int ||
       token.type == JsTokenType.Await as int) {
        var op = builder.allocate_view(token.value);
        parser.increment();
        var operand = jsParser.parsePrimary(parser, builder);
        var unary = builder.allocate<JsUnaryOp>()
        new (unary) JsUnaryOp {
            base : JsNode { kind : JsNodeKind.UnaryOp },
            operator : op,
            operand : operand,
            prefix : true
        }
        return unary as *mut JsNode;
    }

    if(token.type == JsTokenType.ThreeDots as int) {
        parser.increment();
        var arg = jsParser.parsePrimary(parser, builder);
        var spread = builder.allocate<JsSpread>()
        new (spread) JsSpread {
            base : JsNode { kind : JsNodeKind.Spread },
            argument : arg
        }
        return spread as *mut JsNode;
    }

    if(token.type == JsTokenType.Number as int) {
        parser.increment();
        var literal = builder.allocate<JsLiteral>()
        new (literal) JsLiteral {
            base : JsNode { kind : JsNodeKind.Literal },
            value : builder.allocate_view(token.value)
        }
        node = literal as *mut JsNode;
    } else if(token.type == JsTokenType.String as int) {
        parser.increment();
        var literal = builder.allocate<JsLiteral>()
        new (literal) JsLiteral {
            base : JsNode { kind : JsNodeKind.Literal },
            value : builder.allocate_view(token.value)
        }
        node = literal as *mut JsNode;
    } else if(token.type == JsTokenType.Async as int) {
        parser.increment(); // consume async
        // Check for function
        if(parser.getToken().type == JsTokenType.Function as int) {
             // async function expression
             parser.increment(); // consume function

             var name = std::string_view();
             if(parser.getToken().type == JsTokenType.Identifier as int) {
                 name = builder.allocate_view(parser.getToken().value);
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
                 is_async : true,
                 is_generator : false
             }
             node = funcDecl as *mut JsNode;
        } else {
            // Async Arrow: async x => ... or async (x, y) => ...
            const t = parser.getToken();
            if(t.type == JsTokenType.Identifier as int) {
                 var idVal = builder.allocate_view(t.value);
                 parser.increment();
                 if(parser.increment_if(JsTokenType.Arrow as int)) {
                     var body = jsParser.parseArrowBody(parser, builder);
                     var params = std::vector<std::string_view>();
                     params.push(idVal);

                     var arrow = builder.allocate<JsArrowFunction>()
                     new (arrow) JsArrowFunction {
                         base : JsNode { kind : JsNodeKind.ArrowFunction },
                         params : params,
                         body : body,
                         is_async : true,
                         contains_jsx : checkHasJSX(body)
                     }
                     node = arrow as *mut JsNode;
                 } else {
                     parser.error("expected => after async identifier");
                 }
            } else if(t.type == JsTokenType.LParen as int) {
                 // TODO: support async (a,b) => ...
                 parser.error("async (params) => is not yet supported in this parser branch");
            }
        }
    } else if(token.type == JsTokenType.Class as int) {
        node = jsParser.parseClassDecl(parser, builder);
    } else if(token.type == JsTokenType.Function as int) {
        parser.increment(); // consume function
        var is_generator = false;
        if(parser.getToken().type == JsTokenType.Star as int) {
             parser.increment();
             is_generator = true;
        }

        var name = std::string_view();
        if(parser.getToken().type == JsTokenType.Identifier as int) {
             name = builder.allocate_view(parser.getToken().value);
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
        node = funcDecl as *mut JsNode;
    } else if(token.type == JsTokenType.Yield as int) {
        parser.increment();
        var delegate = false;
        if(parser.getToken().type == JsTokenType.Star as int) {
             parser.increment();
             delegate = true;
        }

        var arg : *mut JsNode = null;
        // Simple check to see if we should parse an argument
        const t = parser.getToken();
        if(t.type != JsTokenType.SemiColon as int &&
           t.type != JsTokenType.RBrace as int &&
           t.type != JsTokenType.RParen as int &&
           t.type != JsTokenType.RBracket as int &&
           t.type != JsTokenType.Comma as int &&
           t.type != JsTokenType.Colon as int) {
             arg = jsParser.parseExpression(parser, builder);
        }

        var yieldExpr = builder.allocate<JsYield>()
        new (yieldExpr) JsYield {
             base : JsNode { kind : JsNodeKind.Yield },
             argument : arg,
             delegate : delegate
        }
        node = yieldExpr as *mut JsNode;
    } else if(token.type == JsTokenType.True as int ||
              token.type == JsTokenType.False as int ||
              token.type == JsTokenType.Null as int ||
              token.type == JsTokenType.Undefined as int) {

        parser.increment();
        var literal = builder.allocate<JsLiteral>()
        new (literal) JsLiteral {
            base : JsNode { kind : JsNodeKind.Literal },
            value : builder.allocate_view(token.value)
        }
        node = literal as *mut JsNode;
    } else if(token.type == JsTokenType.LBrace as int) {
         parser.increment(); // consume {
         var props = std::vector<JsProperty>();
         if(parser.getToken().type != JsTokenType.RBrace as int) {
             while(true) {
                 const t = parser.getToken();
                 var key = std::string_view();
                 if(t.type == JsTokenType.Identifier as int || t.type == JsTokenType.String as int) {
                     key = builder.allocate_view(t.value);
                     parser.increment();
                 } else {
                     parser.error("expected identifier or string key");
                     break;
                 }
                 var val : *mut JsNode = null;
                 if(parser.increment_if(JsTokenType.Colon as int)) {
                     val = jsParser.parseExpression(parser, builder);
                 } else {
                     // Support object shorthand: { key }
                     var id = builder.allocate<JsIdentifier>()
                     new (id) JsIdentifier {
                         base : JsNode { kind : JsNodeKind.Identifier },
                         value : key
                     }
                     val = id as *mut JsNode;
                 }
                 props.push(JsProperty { key : key, value : val });
                 if(parser.getToken().type == JsTokenType.Comma as int) {
                      parser.increment();
                 } else {
                      break;
                 }
             }
         }
         if(!parser.increment_if(JsTokenType.RBrace as int)) {
             parser.error("expected }");
         }
         var obj = builder.allocate<JsObjectLiteral>()
         new (obj) JsObjectLiteral {
             base : JsNode { kind : JsNodeKind.ObjectLiteral },
             properties : props
         }
         node = obj as *mut JsNode;
    } else if(token.type == JsTokenType.LBracket as int) {
         parser.increment(); // consume [
         var elements = std::vector<*mut JsNode>();
         if(parser.getToken().type != JsTokenType.RBracket as int) {
             while(true) {
                 if(parser.getToken().type == JsTokenType.Comma as int) {
                     elements.push(null);
                     parser.increment();
                     continue;
                 }
                 var elem = jsParser.parseExpression(parser, builder);
                 elements.push(elem);
                 if(parser.getToken().type == JsTokenType.Comma as int) {
                     parser.increment();
                 } else {
                     break;
                 }
             }
         }
         if(!parser.increment_if(JsTokenType.RBracket as int)) {
             parser.error("expected ]");
         }
         var arr = builder.allocate<JsArrayLiteral>()
         new (arr) JsArrayLiteral {
             base : JsNode { kind : JsNodeKind.ArrayLiteral },
             elements : elements
         }
         node = arr as *mut JsNode;
    } else if(token.type == JsTokenType.Identifier as int || token.type == JsTokenType.This as int || token.type == JsTokenType.Super as int) {
        parser.increment();
        var id = builder.allocate<JsIdentifier>()
        new (id) JsIdentifier {
            base : JsNode { kind : JsNodeKind.Identifier },
            value : builder.allocate_view(token.value)
        }

        if(parser.getToken().type == JsTokenType.Arrow as int) {
            parser.increment(); // consume =>
            var body = jsParser.parseArrowBody(parser, builder);
            var params = std::vector<std::string_view>();
            params.push(id.value);

            var arrow = builder.allocate<JsArrowFunction>()
            new (arrow) JsArrowFunction {
                base : JsNode { kind : JsNodeKind.ArrowFunction },
                params : params,
                body : body,
                is_async : false,
                contains_jsx : checkHasJSX(body)
            }
            node = arrow as *mut JsNode;
        } else {
            node = id as *mut JsNode;
        }
    } else if(token.type == JsTokenType.ChemicalStart as int) {
        parser.increment();
        var val = parser.parseExpression(builder);
        if(val == null) {
            parser.error("expected a chemical expression");
        } else {
            jsParser.dyn_values.push(val)
        }
        if(!parser.increment_if(ChemicalTokenType.RBrace as int)) {
            parser.error("expected } after chemical value");
        }
        var chem = builder.allocate<JsChemicalValue>()
        new (chem) JsChemicalValue {
            base : JsNode { kind : JsNodeKind.ChemicalValue },
            value : val
        }
        node = chem as *mut JsNode;
        node = chem as *mut JsNode;
    } else if(token.type == JsTokenType.LessThan as int) {
        node = jsParser.parseJSXElement(parser, builder);
    } else if(token.type == JsTokenType.LParen as int) {
        parser.increment();

        if(parser.getToken().type == JsTokenType.RParen as int) {
            parser.increment(); // consume )
            if(parser.getToken().type == JsTokenType.Arrow as int) {
                parser.increment(); // consume =>
                var body = jsParser.parseArrowBody(parser, builder);
                var arrow = builder.allocate<JsArrowFunction>()
                new (arrow) JsArrowFunction {
                    base : JsNode { kind : JsNodeKind.ArrowFunction },
                    params : std::vector<std::string_view>(),
                    body : body,
                    is_async : false,
                    contains_jsx : checkHasJSX(body)
                }
                node = arrow as *mut JsNode;
            } else {
                parser.error("expected => after ()");
                return null;
            }
        } else {
            // Check if it's identifier list for arrow
            const nextToken = parser.getToken();
            if(nextToken.type == JsTokenType.Identifier as int) {
                var firstId = builder.allocate_view(nextToken.value);
                parser.increment();

                if(parser.getToken().type == JsTokenType.Comma as int) {
                    // Definitely arrow params
                    var params = std::vector<std::string_view>();
                    params.push(firstId);
                    while(true) {
                        parser.increment(); // consume ,
                        const pToken = parser.getToken();
                        if(pToken.type != JsTokenType.Identifier as int) {
                            parser.error("expected identifier");
                            break;
                        }
                        params.push(builder.allocate_view(pToken.value));
                        parser.increment();
                        if(parser.getToken().type != JsTokenType.Comma as int) break;
                    }
                    if(!parser.increment_if(JsTokenType.RParen as int)) {
                        parser.error("expected )");
                    }
                    if(!parser.increment_if(JsTokenType.Arrow as int)) {
                        parser.error("expected =>");
                    }
                    var body = jsParser.parseArrowBody(parser, builder);
                    var arrow = builder.allocate<JsArrowFunction>()
                    new (arrow) JsArrowFunction {
                        base : JsNode { kind : JsNodeKind.ArrowFunction },
                        params : params,
                        body : body,
                        is_async : false,
                        contains_jsx : checkHasJSX(body)
                    }
                    node = arrow as *mut JsNode;
                } else if(parser.getToken().type == JsTokenType.RParen as int) {
                    parser.increment(); // consume )
                    if(parser.getToken().type == JsTokenType.Arrow as int) {
                        parser.increment(); // consume =>
                        var body = jsParser.parseArrowBody(parser, builder);
                        var params = std::vector<std::string_view>();
                        params.push(firstId);
                        var arrow = builder.allocate<JsArrowFunction>()
                        new (arrow) JsArrowFunction {
                            base : JsNode { kind : JsNodeKind.ArrowFunction },
                            params : params,
                            body : body,
                            is_async : false,
                            contains_jsx : checkHasJSX(body)
                        }
                        node = arrow as *mut JsNode;
                    } else {
                        // (id) ... -> Expression
                        var id = builder.allocate<JsIdentifier>()
                        new (id) JsIdentifier {
                            base : JsNode { kind : JsNodeKind.Identifier },
                            value : firstId
                        }
                        node = id as *mut JsNode;
                    }
                } else {
                    // (id + ...) -> Expression
                    var firstNode = builder.allocate<JsIdentifier>()
                    new (firstNode) JsIdentifier {
                        base : JsNode { kind : JsNodeKind.Identifier },
                        value : firstId
                    }
                    node = jsParser.parsePostfixContinuation(parser, builder, firstNode as *mut JsNode);
                    node = jsParser.parseExpressionContinuation(parser, builder, node);
                    if(!parser.increment_if(JsTokenType.RParen as int)) {
                        parser.error("expected )");
                    }
                }
            } else {
                // (expr)
                node = jsParser.parseExpression(parser, builder);
                if(!parser.increment_if(JsTokenType.RParen as int)) {
                    parser.error("expected )");
                }
            }
        }
    } else {
        parser.error("unexpected token in expression");
        return null;
    }

    return jsParser.parsePostfixContinuation(parser, builder, node);
}

func (jsParser : &mut JsParser) parseExpressionContinuation(parser : *mut Parser, builder : *mut ASTBuilder, left : *mut JsNode) : *mut JsNode {
    var node = left;
    while(true) {
        const token = parser.getToken();
        if(token.type == JsTokenType.Plus as int ||
           token.type == JsTokenType.Minus as int ||
           token.type == JsTokenType.Star as int ||
           token.type == JsTokenType.Slash as int ||
           token.type == JsTokenType.Equal as int ||
           token.type == JsTokenType.PlusEqual as int ||
           token.type == JsTokenType.MinusEqual as int ||
           token.type == JsTokenType.StarEqual as int ||
           token.type == JsTokenType.SlashEqual as int ||
           token.type == JsTokenType.EqualEqual as int ||
           token.type == JsTokenType.NotEqual as int ||
           token.type == JsTokenType.LessThan as int ||
           token.type == JsTokenType.GreaterThan as int ||
           token.type == JsTokenType.LessThanEqual as int ||
           token.type == JsTokenType.GreaterThanEqual as int ||
           token.type == JsTokenType.GreaterThanEqual as int ||
           token.type == JsTokenType.LogicalAnd as int ||
           token.type == JsTokenType.LogicalOr as int ||
           token.type == JsTokenType.BitwiseAnd as int ||
           token.type == JsTokenType.BitwiseOr as int ||
           token.type == JsTokenType.BitwiseXor as int ||
           token.type == JsTokenType.LeftShift as int ||
           token.type == JsTokenType.RightShift as int ||
           token.type == JsTokenType.RightShiftUnsigned as int ||
           token.type == JsTokenType.In as int ||
           token.type == JsTokenType.InstanceOf as int) {

            const is_assignment = token.type == JsTokenType.Equal as int ||
                                  token.type == JsTokenType.PlusEqual as int ||
                                  token.type == JsTokenType.MinusEqual as int ||
                                  token.type == JsTokenType.StarEqual as int ||
                                  token.type == JsTokenType.SlashEqual as int;

            var op = builder.allocate_view(token.value);
            parser.increment();

            var right : *mut JsNode = null;
            if(is_assignment) {
                 right = jsParser.parseExpression(parser, builder);
            } else {
                 right = jsParser.parsePrimary(parser, builder);
            }
            var binOp = builder.allocate<JsBinaryOp>()
            new (binOp) JsBinaryOp {
                base : JsNode { kind : JsNodeKind.BinaryOp },
                left : node,
                right : right,
                op : op
            }
            node = binOp as *mut JsNode;
        } else if(token.type == JsTokenType.Question as int) {
            parser.increment();
            var consequent = jsParser.parseExpression(parser, builder);
            if(!parser.increment_if(JsTokenType.Colon as int)) {
                parser.error("expected : in ternary");
            }
            var alternate = jsParser.parseExpression(parser, builder);
            var ternary = builder.allocate<JsTernary>()
            new (ternary) JsTernary {
                base : JsNode { kind : JsNodeKind.Ternary },
                condition : node,
                consequent : consequent,
                alternate : alternate
            }
            node = ternary as *mut JsNode;
        } else {
            break;
        }
    }
    return node;
}

func (jsParser : &mut JsParser) parseExpression(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    var left = jsParser.parsePrimary(parser, builder);
    if(left == null) return null;
    return jsParser.parseExpressionContinuation(parser, builder, left);
}
