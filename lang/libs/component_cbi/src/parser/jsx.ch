func (jsParser : &mut JsParser) parseJSXAttribute(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    if(parser.getToken().type == JsTokenType.LBrace as int) {
         // Spread attribute { ... }
         parser.increment();
         if(parser.getToken().type == JsTokenType.ThreeDots as int) {
             parser.increment();
             var arg = jsParser.parseExpression(parser, builder);
             if(!parser.increment_if(JsTokenType.RBrace as int)) {
                 parser.error("expected }");
             }
             var spread = builder.allocate<JsJSXSpreadAttribute>()
             new (spread) JsJSXSpreadAttribute {
                 base : JsNode { kind : JsNodeKind.JSXSpreadAttribute },
                 argument : arg
             }
             return spread as *mut JsNode;
         } else {
             // Just expression container inside attribute? No, spread must have ...
             // But wait, <div {x} > is invalid in React? No, <div {...x}> is spread.
             // <div title={x}> is attribute.
             // If we are parsing *inside* the tag, attributes are expected.
             // If we see {, it's likely a spread attribute {...x} OR invalid syntax? 
             // React allows only spread attributes as {...x}.
             // So if no ..., error? 
             // Actually, some parsers might interpret {x} as property shorthand? No.
             parser.error("expected ... in jsx spread attribute"); 
             return null;
         }
    }

    var name = std::string_view();
    if(parser.getToken().type == JsTokenType.Identifier as int) {
         name = builder.allocate_view(parser.getToken().value);
         parser.increment();
         // Handle namespaced attributes ns:attr ?
         if(parser.getToken().type == JsTokenType.Colon as int) {
             parser.increment();
             if(parser.getToken().type == JsTokenType.Identifier as int) {
                 // Append :attr
                 // For now, simplify.
                 const suffix = parser.getToken().value;
                 // name = name + ":" + suffix. 
                 // We need to concat string views. Builder might not support concat easily.
                 // Let's rely on standard identifier parsing for now. 
                 // If the lexer returned separate tokens, we'd need to merge.
                 parser.increment();
             }
         }
         // Handle hyphenated attributes? data-foo.
         if(parser.getToken().type == JsTokenType.Minus as int) {
              parser.increment();
              if(parser.getToken().type == JsTokenType.Identifier as int) {
                   parser.increment();
              }
         }
         // OK, for now assume simple identifiers.
    } else {
         return null;
    }

    var value : *mut JsNode = null;
    if(parser.increment_if(JsTokenType.Equal as int)) {
        if(parser.getToken().type == JsTokenType.String as int) {
             var lit = builder.allocate<JsLiteral>()
             new (lit) JsLiteral {
                 base : JsNode { kind : JsNodeKind.Literal },
                 value : builder.allocate_view(parser.getToken().value)
             }
             value = lit as *mut JsNode;
             parser.increment();
        } else if(parser.getToken().type == JsTokenType.LBrace as int) {
             parser.increment(); // {
             var expr = jsParser.parseExpression(parser, builder);
             if(!parser.increment_if(JsTokenType.RBrace as int)) {
                 parser.error("expected }");
             }
             var container = builder.allocate<JsJSXExpressionContainer>()
             new (container) JsJSXExpressionContainer {
                 base : JsNode { kind : JsNodeKind.JSXExpressionContainer },
                 expression : expr
             }
             value = container as *mut JsNode;
        } else {
             parser.error("expected string or expression for attribute value");
        }
    }

    var attr = builder.allocate<JsJSXAttribute>()
    new (attr) JsJSXAttribute {
        base : JsNode { kind : JsNodeKind.JSXAttribute },
        name : name,
        value : value
    }
    return attr as *mut JsNode;
}

func (jsParser : &mut JsParser) parseJSXElement(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    // We are at <
    parser.increment(); 

    // Fragment <>
    if(parser.getToken().type == JsTokenType.GreaterThan as int) {
        parser.increment(); // >
        var children = std::vector<*mut JsNode>();
        
        // Lexer automatically handles mode via depth/tag state
        
        while(true) {
            const t = parser.getToken();
            if(t.type == JsTokenType.JSXText as int) {
                 var text = builder.allocate<JsJSXText>()
                 new (text) JsJSXText {
                     base : JsNode { kind : JsNodeKind.JSXText },
                     value : builder.allocate_view(t.value)
                 }
                 children.push(text as *mut JsNode);
                 parser.increment();
            } else if(t.type == JsTokenType.LBrace as int) {
                 // Expression container
                 parser.increment(); // {
                 var expr = jsParser.parseExpression(parser, builder);
                 if(!parser.increment_if(JsTokenType.RBrace as int)) {
                     parser.error("expected }");
                 }
                 var container = builder.allocate<JsJSXExpressionContainer>()
                 new (container) JsJSXExpressionContainer {
                     base : JsNode { kind : JsNodeKind.JSXExpressionContainer },
                     expression : expr
                 }
                 children.push(container as *mut JsNode);
            } else if(t.type == JsTokenType.LessThan as int) {
                 // Tag or closing tag.
                 // We need to peek if it's </
                 parser.increment(); // <
                 if(parser.getToken().type == JsTokenType.Slash as int) {
                     // </ closing tag
                     parser.increment(); // /
                     if(parser.getToken().type == JsTokenType.GreaterThan as int) {
                         // </>
                         parser.increment();
                         break;
                     } else {
                         parser.error("expected > for fragment close");
                         break;
                     }
                 } else {
                     // New element
                     var elem = jsParser.parseJSXElementBody(parser, builder);
                     children.push(elem);
                 }
            } else {
                 // Should not happen if lexer is working correctly?
                 // Possibly EOF
                 if(t.type == JsTokenType.EndOfFile as int) {
                     parser.error("unexpected eof in jsx");
                     break;
                 }
                 parser.increment();
            }
        }
        
        var frag = builder.allocate<JsJSXFragment>()
        new (frag) JsJSXFragment {
            base : JsNode { kind : JsNodeKind.JSXFragment },
            children : children
        }
        return frag as *mut JsNode;
    }

    // Normal Element <Tag ...
    return jsParser.parseJSXElementBody(parser, builder);
}

func (jsParser : &mut JsParser) parseJSXElementBody(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    // Assumes < is already consumed or handled (for recursive calls inside fragment loop, check logic).
    
    var tagName : *mut JsNode = null;
    // Parse tag name (Identifier or MemberExpression)
    if(parser.getToken().type == JsTokenType.Identifier as int) {
         var id = builder.allocate<JsIdentifier>()
         new (id) JsIdentifier {
             base : JsNode { kind : JsNodeKind.Identifier },
             value : builder.allocate_view(parser.getToken().value)
         }
         tagName = id as *mut JsNode;
         parser.increment();
         
         // Helper for MemberExpression component names <Foo.Bar>
         while(parser.increment_if(JsTokenType.Dot as int)) {
              if(parser.getToken().type == JsTokenType.Identifier as int) {
                  var prop = builder.allocate_view(parser.getToken().value);
                  parser.increment();
                  var mem = builder.allocate<JsMemberAccess>()
                  new (mem) JsMemberAccess {
                      base : JsNode { kind : JsNodeKind.MemberAccess },
                      object : tagName,
                      property : prop
                  }
                  tagName = mem as *mut JsNode;
              } else {
                  parser.error("expected identifier");
              }
         }
    } else {
        parser.error("expected tag name");
        return null; // or empty node
    }

    var attributes = std::vector<*mut JsNode>();
    while(true) {
        const t = parser.getToken();
        if(t.type == JsTokenType.Slash as int || t.type == JsTokenType.GreaterThan as int) {
            break;
        }
        var attr = jsParser.parseJSXAttribute(parser, builder);
        if(attr != null) {
            attributes.push(attr);
        } else {
            // parsing error or end?
             break;
        }
    }
    
    var selfClosing = false;
    if(parser.increment_if(JsTokenType.Slash as int)) {
        selfClosing = true;
    }
    
    if(!parser.increment_if(JsTokenType.GreaterThan as int)) {
        parser.error("expected >");
    }
    
    var children = std::vector<*mut JsNode>();
    
    if(!selfClosing) {
        while(true) {
            const t = parser.getToken();
            if(t.type == JsTokenType.JSXText as int) {
                 var text = builder.allocate<JsJSXText>()
                 new (text) JsJSXText {
                     base : JsNode { kind : JsNodeKind.JSXText },
                     value : builder.allocate_view(t.value)
                 }
                 children.push(text as *mut JsNode);
                 parser.increment();
            } else if(t.type == JsTokenType.LBrace as int) {
                 parser.increment(); 
                 var expr = jsParser.parseExpression(parser, builder);
                 if(!parser.increment_if(JsTokenType.RBrace as int)) {
                     parser.error("expected }");
                 }
                 var container = builder.allocate<JsJSXExpressionContainer>()
                 new (container) JsJSXExpressionContainer {
                     base : JsNode { kind : JsNodeKind.JSXExpressionContainer },
                     expression : expr
                 }
                 children.push(container as *mut JsNode); 
            } else if(t.type == JsTokenType.LessThan as int) {
                 parser.increment(); // <
                 if(parser.getToken().type == JsTokenType.Slash as int) {
                     // Check if it matches closing tag
                     parser.increment(); // /
                     // Parse TagName
                     if(parser.getToken().type == JsTokenType.Identifier as int) {
                          parser.increment(); // consume name
                     }
                     if(!parser.increment_if(JsTokenType.GreaterThan as int)) {
                         parser.error("expected >");
                     }
                     break; // Done with element
                 } else {
                     // Child element
                     var elem = jsParser.parseJSXElementBody(parser, builder);
                     children.push(elem);
                 }
            } else {
                if(t.type == JsTokenType.EndOfFile as int) {
                     parser.error("unexpected eof in jsx element");
                     break;
                 }
                 parser.increment();
            }
        }
    }

    var element = builder.allocate<JsJSXElement>()
    new (element) JsJSXElement {
        base : JsNode { kind : JsNodeKind.JSXElement },
        opening : JsJSXOpeningElement {
            tagName : tagName,
            attributes : attributes,
            selfClosing : selfClosing
        },
        children : children,
        closing : JsJSXClosingElement {
            tagName : tagName
        }
    }

    if(tagName.kind == JsNodeKind.Identifier) {
        var id = tagName as *mut JsIdentifier;
        if(id.value.size() > 0) {
             const first = id.value.get(0);
             if(first >= 'A' && first <= 'Z') {
                 jsParser.components.push(element);
             }
        }
    }

    return element as *mut JsNode;
}
