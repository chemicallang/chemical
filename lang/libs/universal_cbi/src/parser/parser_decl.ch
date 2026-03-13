func (jsParser : &mut JsParser) parseArrayDestructuring(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    parser.increment(); // consume [
    var elements = std::vector<*mut JsNode>();
    while(parser.getToken().type != JsTokenType.RBracket as int) {
        if(parser.getToken().type == JsTokenType.Comma as int) {
             elements.push(null);
             parser.increment();
             continue;
        }
        const t = parser.getToken();
        if(t.type == JsTokenType.Identifier as int) {
             var id = builder.allocate<JsIdentifier>()
             new (id) JsIdentifier {
                 base : JsNode { kind : JsNodeKind.Identifier },
                 value : builder.allocate_view(t.value)
             }
             elements.push(id as *mut JsNode);
             parser.increment();
        } else if(t.type == JsTokenType.LBracket as int) {
             elements.push(jsParser.parseArrayDestructuring(parser, builder));
        } else {
             parser.error("expected identifier or [ in destructuring");
             break;
        }

        if(parser.getToken().type == JsTokenType.Comma as int) {
            parser.increment();
        } else {
            break;
        }
    }
    if(!parser.increment_if(JsTokenType.RBracket as int)) {
        parser.error("expected ]");
    }
    var dest = builder.allocate<JsArrayLiteral>() // Using ArrayLiteral for pattern storage
    new (dest) JsArrayLiteral {
        base : JsNode { kind : JsNodeKind.ArrayDestructuring },
        elements : elements
    }
    return dest as *mut JsNode;
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

    var jsExp = builder.allocate<JsExport>()
    new (jsExp) JsExport {
        base : JsNode { kind : JsNodeKind.Export },
        declaration : decl,
        is_default : is_default
    }
    return jsExp as *mut JsNode;
}
