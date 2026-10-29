func (jsParser : &mut JsParser) parseArrowBody(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
        unsafe {
        const tokenType = parser.getToken().type
        if(tokenType == JsTokenType.LBrace as int || tokenType == JsTokenType.If as int) {
            return jsParser.parseStatement(parser, builder)
        } else {
            return jsParser.parseExpression(parser, builder)
        }

        }
    }

func (jsParser : &mut JsParser) parsePostfixContinuation(parser : *mut Parser, builder : *mut ASTBuilder, start : *mut JsNode) : *mut JsNode {
        unsafe {
        var node = start
        while(true) {
            const t = parser.getToken();
            if(t.type == JsTokenType.Dot as int) {
                parser.increment();
                const idToken = parser.getToken();
                if(!isIdOrKwInt(idToken.type)) {
                    parser.error("expected identifier after dot");
                    break;
                }
                var prop = builder.allocate_view(&idToken.value);
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
                var op = builder.allocate_view(&t.value);
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
    }

func (jsParser : &mut JsParser) parsePrimary(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
        unsafe {
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
            var op = builder.allocate_view(&token.value);
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
                value : builder.allocate_view(&token.value)
            }
            node = literal as *mut JsNode;
        } else if(token.type == JsTokenType.String as int) {
            parser.increment();
            var literal = builder.allocate<JsLiteral>()
            new (literal) JsLiteral {
                base : JsNode { kind : JsNodeKind.Literal },
                value : builder.allocate_view(&token.value)
            }
            node = literal as *mut JsNode;
        } else if(token.type == JsTokenType.Regex as int) {
            // A regular-expression literal is emitted verbatim (`/pattern/flags`);
            // the lexer has already resolved `/` against division.
            parser.increment();
            var literal = builder.allocate<JsLiteral>()
            new (literal) JsLiteral {
                base : JsNode { kind : JsNodeKind.Literal },
                value : builder.allocate_view(&token.value)
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
                     name = builder.allocate_view(&parser.getToken().value);
                     parser.increment();
                 }

                 if(!parser.increment_if(JsTokenType.LParen as int)) {
                     parser.error("expected (");
                 }

                 var params = std::vector<JsParam>();
                 if(parser.getToken().type != JsTokenType.RParen as int) {
                     while(true) {
                         const paramToken = parser.getToken();
                         if(paramToken.type != JsTokenType.Identifier as int) {
                             parser.error("expected identifier");
                             break;
                         }
                         parser.increment();
                         var param = JsParam { name : builder.allocate_view(&paramToken.value), default_value : null }
                         if(parser.getToken().type == JsTokenType.Equal as int) {
                             parser.increment(); // consume =
                             param.default_value = jsParser.parseExpression(parser, builder);
                         }
                         params.push(param);
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
                     var idVal = builder.allocate_view(&t.value);
                     parser.increment();
                     if(parser.increment_if(JsTokenType.Arrow as int)) {
                         var body = jsParser.parseArrowBody(parser, builder);
                         var params = std::vector<JsParam>();
                         params.push(JsParam { name : idVal, default_value : null });

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
                 name = builder.allocate_view(&parser.getToken().value);
                 parser.increment();
            }

            if(!parser.increment_if(JsTokenType.LParen as int)) {
                 parser.error("expected (");
            }

            var params = std::vector<JsParam>();
            if(parser.getToken().type != JsTokenType.RParen as int) {
                while(true) {
                    const paramToken = parser.getToken();
                    if(paramToken.type != JsTokenType.Identifier as int) {
                        parser.error("expected identifier");
                        break;
                    }
                    parser.increment();
                    var param = JsParam { name : builder.allocate_view(&paramToken.value), default_value : null }
                    if(parser.getToken().type == JsTokenType.Equal as int) {
                        parser.increment(); // consume =
                        param.default_value = jsParser.parseExpression(parser, builder);
                    }
                    params.push(param);

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
                value : builder.allocate_view(&token.value)
            }
            node = literal as *mut JsNode;
        } else if(token.type == JsTokenType.LBrace as int) {
             parser.increment(); // consume {
             var props = std::vector<JsProperty>();
             if(parser.getToken().type != JsTokenType.RBrace as int) {
                 while(true) {
                     const t = parser.getToken();
                     if(t.type == JsTokenType.ThreeDots as int) {
                         var spread = jsParser.parseExpression(parser, builder);
                         props.push(JsProperty { key : std::string_view(), value : spread });
                     } else {
                         var key = std::string_view();
                         if(t.type == JsTokenType.Identifier as int || t.type == JsTokenType.String as int) {
                             key = builder.allocate_view(&t.value);
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
                     }
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
        } else if(token.type == JsTokenType.Style as int) {
            if(jsParser.style_fn != null) {
                node = jsParser.style_fn(parser, builder, jsParser.dyn_values)
            } else {
                parser.error("style blocks are not supported here")
                node = null
            }
        } else if(token.type == JsTokenType.Identifier as int || token.type == JsTokenType.This as int || token.type == JsTokenType.Super as int) {
            parser.increment();
            var id = builder.allocate<JsIdentifier>()
            new (id) JsIdentifier {
                base : JsNode { kind : JsNodeKind.Identifier },
                value : builder.allocate_view(&token.value)
            }

            if(parser.getToken().type == JsTokenType.Arrow as int) {
                parser.increment(); // consume =>
                var body = jsParser.parseArrowBody(parser, builder);
                var params = std::vector<JsParam>();
                params.push(JsParam { name : id.value, default_value : null });

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
                        params : std::vector<JsParam>(),
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
                    var firstId = builder.allocate_view(&nextToken.value);
                    parser.increment();

                    if(parser.getToken().type == JsTokenType.Comma as int) {
                        // Definitely arrow params
                        var params = std::vector<JsParam>();
                        params.push(JsParam { name : firstId, default_value : null });
                        while(true) {
                            parser.increment(); // consume ,
                            const pToken = parser.getToken();
                            if(pToken.type != JsTokenType.Identifier as int) {
                                parser.error("expected identifier");
                                break;
                            }
                            parser.increment();
                            var param = JsParam { name : builder.allocate_view(&pToken.value), default_value : null }
                            if(parser.getToken().type == JsTokenType.Equal as int) {
                                parser.increment(); // consume =
                                param.default_value = jsParser.parseExpression(parser, builder);
                            }
                            params.push(param);
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
                            var params = std::vector<JsParam>();
                            params.push(JsParam { name : firstId, default_value : null });
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
                            // Explicit grouping is preserved in every mode. Dropping
                            // it in plain-JS mode turned `!(id in local)` into
                            // `!id in local` (i.e. `(!id) in local`), silently
                            // changing what the emitted bundle means.
                            var paren = builder.allocate<JsParen>()
                            new (paren) JsParen {
                                base : JsNode { kind : JsNodeKind.Paren },
                                expression : id as *mut JsNode
                            }
                            node = paren as *mut JsNode;
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
                        // Preserve the explicit group as an operand (see above).
                        var paren = builder.allocate<JsParen>()
                        new (paren) JsParen {
                            base : JsNode { kind : JsNodeKind.Paren },
                            expression : node
                        }
                        node = paren as *mut JsNode;
                    }
                } else {
                    // (expr)
                    node = jsParser.parseExpression(parser, builder);
                    if(!parser.increment_if(JsTokenType.RParen as int)) {
                        parser.error("expected )");
                    }
                    // Preserve the explicit group as an operand (see above).
                    var paren = builder.allocate<JsParen>()
                    new (paren) JsParen {
                        base : JsNode { kind : JsNodeKind.Paren },
                        expression : node
                    }
                    node = paren as *mut JsNode;
                }
            }
        } else {
            parser.error("unexpected token in expression");
            return null;
        }

        return jsParser.parsePostfixContinuation(parser, builder, node);

        }
    }

// Integer token kinds from the CBI lexer may include values outside the JS
// enum (for example EOF or host-specific punctuation). Compare as integers
// instead of forging an enum value.
func isIdOrKwInt(type : int) : bool {
        unsafe {
        return type == (JsTokenType.Identifier as int) ||
               type == (JsTokenType.Function as int) ||
               type == (JsTokenType.Return as int) ||
               type == (JsTokenType.If as int) ||
               type == (JsTokenType.Else as int) ||
               type == (JsTokenType.Const as int) ||
               type == (JsTokenType.Let as int) ||
               type == (JsTokenType.For as int) ||
               type == (JsTokenType.While as int) ||
               type == (JsTokenType.Break as int) ||
               type == (JsTokenType.Continue as int) ||
               type == (JsTokenType.Switch as int) ||
               type == (JsTokenType.Case as int) ||
               type == (JsTokenType.Default as int) ||
               type == (JsTokenType.Do as int) ||
               type == (JsTokenType.State as int) ||
               type == (JsTokenType.Try as int) ||
               type == (JsTokenType.Catch as int) ||
               type == (JsTokenType.Finally as int) ||
               type == (JsTokenType.Throw as int) ||
               type == (JsTokenType.True as int) ||
               type == (JsTokenType.False as int) ||
               type == (JsTokenType.Null as int) ||
               type == (JsTokenType.Undefined as int) ||
               type == (JsTokenType.Typeof as int) ||
               type == (JsTokenType.Void as int) ||
               type == (JsTokenType.Delete as int) ||
               type == (JsTokenType.In as int) ||
               type == (JsTokenType.InstanceOf as int) ||
               type == (JsTokenType.New as int) ||
               type == (JsTokenType.Of as int) ||
               type == (JsTokenType.This as int) ||
               type == (JsTokenType.Async as int) ||
               type == (JsTokenType.Await as int) ||
               type == (JsTokenType.Class as int) ||
               type == (JsTokenType.Extends as int) ||
               type == (JsTokenType.Super as int) ||
               type == (JsTokenType.Static as int) ||
               type == (JsTokenType.Import as int) ||
               type == (JsTokenType.Export as int) ||
               type == (JsTokenType.Yield as int) ||
               type == (JsTokenType.Debugger as int)

        }
    }

// JS operator precedence (higher binds tighter). Mirrors the ECMAScript
// grammar: || < && < | < ^ < & < equality < relational < shift < additive <
// multiplicative. Returns 0 for tokens that are not binary operators.
func binary_operator_precedence(tokenType : int) : int {
        unsafe {
        if(tokenType == (JsTokenType.LogicalOr as int) ||
           tokenType == (JsTokenType.NullishCoalescing as int)) { return 1 }
        if(tokenType == (JsTokenType.LogicalAnd as int)) { return 2 }
        if(tokenType == (JsTokenType.BitwiseOr as int)) { return 3 }
        if(tokenType == (JsTokenType.BitwiseXor as int)) { return 4 }
        if(tokenType == (JsTokenType.BitwiseAnd as int)) { return 5 }
        if(tokenType == (JsTokenType.EqualEqual as int) ||
           tokenType == (JsTokenType.StrictEqual as int) ||
           tokenType == (JsTokenType.NotEqual as int) ||
           tokenType == (JsTokenType.StrictNotEqual as int)) { return 6 }
        if(tokenType == (JsTokenType.LessThan as int) ||
           tokenType == (JsTokenType.GreaterThan as int) ||
           tokenType == (JsTokenType.LessThanEqual as int) ||
           tokenType == (JsTokenType.GreaterThanEqual as int) ||
           tokenType == (JsTokenType.In as int) ||
           tokenType == (JsTokenType.InstanceOf as int)) { return 7 }
        if(tokenType == (JsTokenType.LeftShift as int) ||
           tokenType == (JsTokenType.RightShift as int) ||
           tokenType == (JsTokenType.RightShiftUnsigned as int)) { return 8 }
        if(tokenType == (JsTokenType.Plus as int) ||
           tokenType == (JsTokenType.Minus as int)) { return 9 }
        if(tokenType == (JsTokenType.Star as int) ||
           tokenType == (JsTokenType.Slash as int) ||
           tokenType == (JsTokenType.Percent as int)) { return 10 }
        if(tokenType == (JsTokenType.Exponent as int)) { return 11 }
        return 0

        }
    }

// Precedence-climbing binary expression parser. Parses operators with
// precedence >= minPrec left-associatively; the right side is parsed with a
// strictly higher minimum so higher-precedence operators bind tighter
// (e.g. `a !== b || c` becomes ||(!==(a,b), c), not !==(||(a,b), c)).
func (jsParser : &mut JsParser) parseBinaryPrecedence(parser : *mut Parser, builder : *mut ASTBuilder, minPrec : int, left : *mut JsNode) : *mut JsNode {
        unsafe {
        var node = left
        while(true) {
            const token = parser.getToken()
            const prec = binary_operator_precedence(token.type as int)
            if(prec == 0 || prec < minPrec) break
            var op = builder.allocate_view(&token.value)
            parser.increment()
            var right = jsParser.parsePrimary(parser, builder)
            if(right == null) {
                parser.error("expected expression after operator")
                break
            }
            right = jsParser.parseBinaryPrecedence(parser, builder, prec + 1, right)
            var binOp = builder.allocate<JsBinaryOp>()
            new (binOp) JsBinaryOp {
                base : JsNode { kind : JsNodeKind.BinaryOp },
                left : node,
                right : right,
                op : op
            }
            node = binOp as *mut JsNode
        }
        return node

        }
    }

func (jsParser : &mut JsParser) parseExpressionContinuation(parser : *mut Parser, builder : *mut ASTBuilder, left : *mut JsNode) : *mut JsNode {
        unsafe {
        var node = left;
        while(true) {
            const token = parser.getToken();
            if(token.type == JsTokenType.Equal as int ||
               token.type == JsTokenType.PlusEqual as int ||
               token.type == JsTokenType.MinusEqual as int ||
               token.type == JsTokenType.StarEqual as int ||
               token.type == JsTokenType.SlashEqual as int ||
               token.type == JsTokenType.PercentEqual as int) {

                var op = builder.allocate_view(&token.value);
                parser.increment();

                var right = jsParser.parseExpression(parser, builder);
                var binOp = builder.allocate<JsBinaryOp>()
                new (binOp) JsBinaryOp {
                    base : JsNode { kind : JsNodeKind.BinaryOp },
                    left : node,
                    right : right,
                    op : op
                }
                node = binOp as *mut JsNode;
            } else if(binary_operator_precedence(token.type as int) > 0) {
                // Consume all binary operators (with correct precedence), then
                // re-check for assignment / ternary continuations.
                node = jsParser.parseBinaryPrecedence(parser, builder, 1, node);
            } else if(token.type == JsTokenType.QuestionDot as int) {
                // Optional chaining: a?.b — treat as member access for now;
                // the converter handles null safety.
                parser.increment();
                var prop = parser.getToken();
                if(prop.type == JsTokenType.Identifier as int) {
                    var propView = builder.allocate_view(&prop.value);
                    parser.increment();
                    var mem = builder.allocate<JsMemberAccess>()
                    new (mem) JsMemberAccess {
                        base : JsNode { kind : JsNodeKind.MemberAccess },
                        object : node,
                        property : propView
                    }
                    node = mem as *mut JsNode;
                } else if(prop.type == JsTokenType.LBracket as int) {
                    parser.increment();
                    var index = jsParser.parseExpression(parser, builder);
                    parser.increment_if(JsTokenType.RBracket as int);
                    var idx = builder.allocate<JsIndexAccess>()
                    new (idx) JsIndexAccess {
                        base : JsNode { kind : JsNodeKind.IndexAccess },
                        object : node,
                        index : index
                    }
                    node = idx as *mut JsNode;
                } else if(prop.type == JsTokenType.LParen as int) {
                    // a?.() — optional call: treat as regular call
                    parser.increment();
                    var args = std::vector<*mut JsNode>()
                    if(prop.type != JsTokenType.RParen as int) {
                        while(true) {
                            var arg = jsParser.parseExpression(parser, builder);
                            args.push(arg);
                            if(!parser.increment_if(JsTokenType.Comma as int)) break;
                        }
                    }
                    parser.increment_if(JsTokenType.RParen as int);
                    var call = builder.allocate<JsFunctionCall>()
                    new (call) JsFunctionCall {
                        base : JsNode { kind : JsNodeKind.FunctionCall },
                        callee : node,
                        args : args
                    }
                    node = call as *mut JsNode;
                }
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
    }

func (jsParser : &mut JsParser) parseExpression(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
        unsafe {
        var left = jsParser.parsePrimary(parser, builder);
        if(left == null) return null;
        return jsParser.parseExpressionContinuation(parser, builder, left);

        }
    }
