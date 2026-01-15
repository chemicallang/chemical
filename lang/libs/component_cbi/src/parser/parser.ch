struct JsParser {
    var dyn_values : *mut std::vector<*mut Value>
    var components : *mut std::vector<*mut JsJSXElement>
}

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
    Exclamation,
    Arrow, // =>
    Const,
    Let,
    For,
    While,
    LogicalAnd,      // &&
    LogicalOr,       // ||
    Question,        // ?
    PlusPlus,        // ++
    MinusMinus,      // --
    Break,
    Continue,
    Switch,
    Case,
    Default,
    Do,
    Try,
    Catch,
    Finally,
    Throw,
    TemplateLiteral,  // `string`
    True,
    False,
    Null,
    Undefined,
    PlusEqual,
    MinusEqual,
    StarEqual,
    SlashEqual,
    Typeof,
    Void,
    Delete,
    In,
    InstanceOf,
    BitwiseNot, // ~
    BitwiseAnd, // &
    BitwiseOr,  // |
    BitwiseXor, // ^
    LeftShift,  // <<
    RightShift, // >>
    RightShiftUnsigned, // >>>
    New,
    Of,
    This,
    ThreeDots, // ...
    Async,
    Await,
    Class,
    Extends,
    Super,
    Static,
    Import,
    Export,
    Yield,
    Debugger,
    JSXText
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
                     var body : *mut JsNode = null;
                     if(parser.getToken().type == JsTokenType.LBrace as int) {
                         body = jsParser.parseBlock(parser, builder);
                     } else {
                         body = jsParser.parseExpression(parser, builder);
                     }
                     var params = std::vector<std::string_view>();
                     params.push(idVal);
                 
                     var arrow = builder.allocate<JsArrowFunction>()
                     new (arrow) JsArrowFunction {
                         base : JsNode { kind : JsNodeKind.ArrowFunction },
                         params : params,
                         body : body,
                         is_async : true
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
                 if(!parser.increment_if(JsTokenType.Colon as int)) {
                     parser.error("expected :");
                 }
                 var val = jsParser.parseExpression(parser, builder);
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
            var body : *mut JsNode = null;
            if(parser.getToken().type == JsTokenType.LBrace as int) {
                body = jsParser.parseBlock(parser, builder);
            } else {
                body = jsParser.parseExpression(parser, builder);
            }
            var params = std::vector<std::string_view>();
            params.push(id.value);
            
            var arrow = builder.allocate<JsArrowFunction>()
            new (arrow) JsArrowFunction {
                base : JsNode { kind : JsNodeKind.ArrowFunction },
                params : params,
                body : body,
                is_async : false
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
        if(!parser.increment_if(JsTokenType.RBrace as int)) {
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
                var body : *mut JsNode = null;
                if(parser.getToken().type == JsTokenType.LBrace as int) {
                    body = jsParser.parseBlock(parser, builder);
                } else {
                    body = jsParser.parseExpression(parser, builder);
                }
                var arrow = builder.allocate<JsArrowFunction>()
                new (arrow) JsArrowFunction {
                    base : JsNode { kind : JsNodeKind.ArrowFunction },
                    params : std::vector<std::string_view>(),
                    body : body,
                    is_async : false,
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
                    var body : *mut JsNode = null;
                    if(parser.getToken().type == JsTokenType.LBrace as int) {
                        body = jsParser.parseBlock(parser, builder);
                    } else {
                        body = jsParser.parseExpression(parser, builder);
                    }
                    var arrow = builder.allocate<JsArrowFunction>()
                    new (arrow) JsArrowFunction {
                        base : JsNode { kind : JsNodeKind.ArrowFunction },
                        params : params,
                        body : body,
                        is_async : false,
                    }
                    node = arrow as *mut JsNode;
                } else if(parser.getToken().type == JsTokenType.RParen as int) {
                    parser.increment(); // consume )
                    if(parser.getToken().type == JsTokenType.Arrow as int) {
                        parser.increment(); // consume =>
                        var body : *mut JsNode = null;
                        if(parser.getToken().type == JsTokenType.LBrace as int) {
                            body = jsParser.parseBlock(parser, builder);
                        } else {
                            body = jsParser.parseExpression(parser, builder);
                        }
                        var params = std::vector<std::string_view>();
                        params.push(firstId);
                        var arrow = builder.allocate<JsArrowFunction>()
                        new (arrow) JsArrowFunction {
                            base : JsNode { kind : JsNodeKind.ArrowFunction },
                            params : params,
                            body : body,
                            is_async : false,
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
                    var id = builder.allocate<JsIdentifier>()
                    new (id) JsIdentifier {
                        base : JsNode { kind : JsNodeKind.Identifier },
                        value : firstId
                    }
                    node = jsParser.parseExpressionContinuation(parser, builder, id as *mut JsNode);
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
    
    // Postfix loop
    while(true) {
        const t = parser.getToken();
        if(t.type == JsTokenType.Dot as int) {
            parser.increment();
            const idToken = parser.getToken();
            if(idToken.type != JsTokenType.Identifier as int) {
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


func (jsParser : &mut JsParser) parseClassDecl(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    parser.increment(); // consume class
    
    var name = std::string_view();
    if(parser.getToken().type == JsTokenType.Identifier as int) {
        name = builder.allocate_view(parser.getToken().value);
        parser.increment();
    }
    
    var superClass = std::string_view();
    if(parser.increment_if(JsTokenType.Extends as int)) {
         if(parser.getToken().type == JsTokenType.Identifier as int) {
             superClass = builder.allocate_view(parser.getToken().value);
             parser.increment();
         } else {
             parser.error("expected identifier after extends");
         }
    }
    
    if(!parser.increment_if(JsTokenType.LBrace as int)) {
        parser.error("expected { for class body");
    }
    
    var methods = std::vector<JsClassMethod>();
    while(parser.getToken().type != JsTokenType.RBrace as int) {
        if(parser.getToken().type == JsTokenType.EndOfFile as int) {
             parser.error("unexpected eof in class body");
             break;
        }
        
        var is_static = false;
        if(parser.getToken().type == JsTokenType.Static as int) {
             parser.increment();
             is_static = true;
        }
        
        // Method name
        var methodName = std::string_view();
        const t = parser.getToken();
        if(t.type == JsTokenType.Identifier as int || t.type == JsTokenType.String as int) {
             methodName = builder.allocate_view(t.value);
             parser.increment();
        } else {
             parser.error("expected method name");
             break;
        }
        
        if(!parser.increment_if(JsTokenType.LParen as int)) {
             parser.error("expected ( for method");
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
        
        methods.push(JsClassMethod {
            name : methodName,
            params : params,
            body : body,
            is_static : is_static
        });
    }
    
    parser.increment(); // consume }
    
    var classDecl = builder.allocate<JsClassDecl>()
    new (classDecl) JsClassDecl {
        base : JsNode { kind : JsNodeKind.ClassDecl },
        name : name,
        superClass : superClass,
        methods : methods
    }
    return classDecl as *mut JsNode;
}

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
    if(token.type == JsTokenType.Var as int || token.type == JsTokenType.Const as int || token.type == JsTokenType.Let as int) {
        var keyword = builder.allocate_view(token.value);
        parser.increment();
        const idToken = parser.getToken();
        if(idToken.type != JsTokenType.Identifier as int) {
            parser.error("expected identifier");
            return null;
        }
        var name = builder.allocate_view(idToken.value);
        parser.increment();
        
        if(parser.increment_if(JsTokenType.Equal as int)) {
            var val = jsParser.parseExpression(parser, builder);
            var varDecl = builder.allocate<JsVarDecl>()
            new (varDecl) JsVarDecl {
                base : JsNode { kind : JsNodeKind.VarDecl },
                name : name,
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
        if(parser.getToken().type != JsTokenType.SemiColon as int) {
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
               parser.getToken().type == JsTokenType.Let as int) {
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
        // If initNode caused a semicolon consumption (via parseStatement for VarDecl), we might have issues if For-In doesn't use semicolons.
        // JS: for(var x in y) -> VarDecl parses "var x". If parseStatement expects semicolon, it might error or consume it?
        // In current parseStatement: VarDecl parses "var x = val;" expecting semicolon.
        // But "var x in y" does NOT have semicolon after x.
        // So parseStatement for VarDecl is problematic here if it enforces Semicolon.
        // Let's look at parseStatement for VarDecl: 
        // 499: parser.increment_if(JsTokenType.SemiColon as int);
        // It optionally consumes semicolon? No `increment_if` consumes if present.
        // But for `for(var x in y)`, there is no semicolon.
        // So `parseStatement` returns VarDecl. Next token is `in`.
        // So we can check for `in` or `of`.
        
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
        // Ensure semicolon was consumed if it was a statement.
        // If initNode is expression statement created manually, we didn't consume semicolon.
        // If initNode came from parseStatement (VarDecl), it might have consumed semicolon if present.
        // Loop expects semicolon after init.
        
        if(parser.getToken().type == JsTokenType.SemiColon as int) {
            parser.increment();
        } else if(!is_decl) {
             // If valid loop, semicolon required. 
             // If var decl, parseStatement tries to eat semicolon. If not there, it's fine.
             // But we are here, so we expect semicolon separator.
             // If var decl consumed it (e.g. for(var x; ...)), then we might see next token.
             // Wait, parseStatement eats semicolon.
             // So if `var x;`, we are at `cond`.
             // But valid JS `for(var x; ...)` has valid semicolon.
             // `for(var x in y)` has NO semicolon.
             // So relying on `parseStatement` is tricky.
             // But for now, assuming standard loop structure or in/of structure.
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

func (jsParser : &mut JsParser) parseImport(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    parser.increment(); // consume import
    
    var specifiers = std::vector<ImportSpecifier>();
    
    // Check if string immediately (import "source")
    if(parser.getToken().type == JsTokenType.String as int) {
        var source = builder.allocate_view(parser.getToken().value);
        parser.increment();
        parser.increment_if(JsTokenType.SemiColon as int);
        
        var imp = builder.allocate<JsImport>()
        new (imp) JsImport {
            base : JsNode { kind : JsNodeKind.Import },
            source : source,
            specifiers : specifiers
        }
        return imp as *mut JsNode;
    }
    
    // Parse specifiers
    if(parser.getToken().type == JsTokenType.Identifier as int) {
        // Default import
        var local = builder.allocate_view(parser.getToken().value);
        specifiers.push(ImportSpecifier { imported : view("default"), local : local });
        parser.increment();
        if(parser.getToken().type == JsTokenType.Comma as int) {
            parser.increment();
        }
    }
    
    if(parser.getToken().type == JsTokenType.Star as int) {
        // Namespace import: * as name
        parser.increment(); // *
        if(parser.getToken().type == JsTokenType.Identifier as int && parser.getToken().value.equals(view("as"))) {
            parser.increment(); // as
            if(parser.getToken().type == JsTokenType.Identifier as int) {
                 var local = builder.allocate_view(parser.getToken().value);
                 specifiers.push(ImportSpecifier { imported : view("*"), local : local });
                 parser.increment();
            } else {
                 parser.error("expected identifier after as");
            }
        } else {
            parser.error("expected as after *");
        }
    } else if(parser.getToken().type == JsTokenType.LBrace as int) {
        parser.increment(); // {
        while(parser.getToken().type != JsTokenType.RBrace as int) {
            if(parser.getToken().type == JsTokenType.Identifier as int) {
                var imported = builder.allocate_view(parser.getToken().value);
                var local = imported;
                parser.increment();
                
                if(parser.getToken().type == JsTokenType.Identifier as int && parser.getToken().value.equals(view("as"))) {
                    parser.increment(); // as
                    if(parser.getToken().type == JsTokenType.Identifier as int) {
                        local = builder.allocate_view(parser.getToken().value);
                        parser.increment();
                    } else {
                        parser.error("expected identifier after as");
                    }
                }
                
                specifiers.push(ImportSpecifier { imported : imported, local : local });
                
                if(parser.getToken().type == JsTokenType.Comma as int) {
                    parser.increment();
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        if(!parser.increment_if(JsTokenType.RBrace as int)) {
            parser.error("expected }");
        }
    }
    
    // Expect 'from'
    if(parser.getToken().type == JsTokenType.Identifier as int && parser.getToken().value.equals(view("from"))) {
        parser.increment();
    } else {
        parser.error("expected 'from'");
    }
    
    var source = std::string_view();
    if(parser.getToken().type == JsTokenType.String as int) {
        source = builder.allocate_view(parser.getToken().value);
        parser.increment();
    } else {
        parser.error("expected string source");
    }
    
    parser.increment_if(JsTokenType.SemiColon as int);
    
    var imp2 = builder.allocate<JsImport>()
    new (imp2) JsImport {
        base : JsNode { kind : JsNodeKind.Import },
        source : source,
        specifiers : specifiers
    }
    return imp2 as *mut JsNode;
}

func (jsParser : &mut JsParser) parseExport(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    parser.increment(); // consume export
    var is_default = false;
    
    if(parser.getToken().type == JsTokenType.Default as int) {
        parser.increment();
        is_default = true;
    }
    
    var decl : *mut JsNode = null;
    if(is_default) {
        decl = jsParser.parseExpression(parser, builder);
    } else {
        if(parser.getToken().type == JsTokenType.LBrace as int) {
             parser.error("export { ... } not yet implemented");
             // Just consume to avoid infinite loop if user tries
             parser.increment();
             while(parser.getToken().type != JsTokenType.RBrace as int && parser.getToken().type != JsTokenType.EndOfFile as int) {
                 parser.increment();
             }
             parser.increment_if(JsTokenType.RBrace as int);
        } else {
             decl = jsParser.parseStatement(parser, builder);
        }
    }
    
    parser.increment_if(JsTokenType.SemiColon as int);
    
    var exp = builder.allocate<JsExport>()
    new (exp) JsExport {
        base : JsNode { kind : JsNodeKind.Export },
        declaration : decl,
        is_default : is_default
    }
    return exp as *mut JsNode;
}