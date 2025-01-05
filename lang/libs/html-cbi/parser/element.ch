import "@compiler/Parser.ch"
import "@compiler/ASTBuilder.ch"
import "../utils/stdutils.ch"
import "./attribute.ch"

// forward declare
func parseElementChild(parser : *mut Parser, builder : *mut ASTBuilder) : *mut HtmlChild

func parseElement(parser : *mut Parser, builder : *mut ASTBuilder) : *mut HtmlElement {

    const lt = parser.getToken();

    printf("parsing element at %d, %d\n", lt.position.line, lt.position.character)
    fflush(null)

    if(lt.type == TokenType.LessThan) {

        parser.increment();
        const id = parser.getToken();
        if(id.type != TokenType.Identifier) {
            parser.error("expected an identifier after '<'");
            return null
        }
        parser.increment();

        printf("size of HtmlElement is %d\n", #sizeof(HtmlElement))
        fflush(null)

        var element : *mut HtmlElement = builder.allocate<HtmlElement>();
        new (element) HtmlElement {
            HtmlChild : HtmlChild {
                kind : HtmlChildKind.Element
            },
            name : builder.allocate_view(id.value),
            attributes : std::vector<*HtmlAttribute>(),
            children : std::vector<*HtmlChild>()
        }

        printf("let's checkout size of the children : %d\n", element.children.size())

        printf("parsing attributes for %s\n", id.value.data());
        fflush(null)

        while(true) {
            var attr = parseAttribute(parser, builder);
            if(attr != null) {
                element.attributes.push(attr)
            } else {
                break;
            }
        }

        const gt = parser.getToken();
        if(gt.type != TokenType.GreaterThan) {
            parser.error("expected a greater than sign '>' after the identifier");
        } else {
            parser.increment();
        }

        printf("let's checkout size of the children : %d\n", element.children.size())
        printf("parsing children for %s\n", id.value.data());
        fflush(null)

        printf("let's checkout size of the children : %d\n", element.children.size())
        while(true) {
            var child = parseElementChild(parser, builder);
            printf("checking received element child\n");
            fflush(null)
            if(child != null) {
                printf("adding received element child\n");
                fflush(null)
                printf("let's checkout size of the children : %d\n", element.children.size())
                element.children.push(child)
                printf("added received element child\n");
                fflush(null)
            } else {
                printf("received element child is null\n");
                fflush(null)
                break;
            }
        }

        printf("parsing end of element %s\n", id.value.data())
        fflush(null)

        const last_lt = parser.getToken();
        if(last_lt.type == TokenType.LessThan) {
            parser.increment();
            const fwd = parser.getToken();
            if(fwd.type == TokenType.FwdSlash) {
                parser.increment();
                const last_id = parser.getToken();
                if(last_id.type == TokenType.Identifier) {
                    parser.increment();
                    if(strncmp(last_id.value.data(), id.value.data(), id.value.size()) != 0) {
                        parser.error("expected correct identifier for ending tag");
                    }
                    const last_gt = parser.getToken();
                    if(last_gt.type == TokenType.GreaterThan) {
                        parser.increment();
                    } else {
                        parser.error("expected '>' after the ending tag");
                    }
                } else {
                    parser.error("expected identifier after the '</'");
                }
            } else {
                parser.error("expected a '</' for ending the element");
            }
        } else {
            parser.error("expected a '</' for ending the element");
        }

        return element;

    } else {
        return null;
    }
}
