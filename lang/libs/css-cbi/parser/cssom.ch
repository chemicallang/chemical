import "@compiler/ASTBuilder.ch"
import "@compiler/Parser.ch"
import "../ast/CSSOM.ch"
import "./declaration.ch"

func parseCSSOM(parser : *mut Parser, builder : *mut ASTBuilder) : *CSSOM {
    var root = builder.allocate<CSSOM>()
    new (root) CSSOM {
        parent : parser.getParentNode(),
        declarations : std::vector<*mut CSSDeclaration>()
    }
    while(true) {
        var decl = parseDeclaration(parser, builder);
        if(decl) {
            root.declarations.push(decl)
        } else {
            break;
        }
    }
    return root;
}