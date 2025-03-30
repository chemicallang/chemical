import "@compiler/ASTBuilder.ch"
import "@compiler/Parser.ch"
import "../ast/CSSOM.ch"
import "./declaration.ch"
import "./CSSParser.ch"

func parseCSSOM(parser : *mut Parser, builder : *mut ASTBuilder) : *CSSOM {
    var root = builder.allocate<CSSOM>()
    new (root) CSSOM {
        parent : parser.getParentNode(),
        has_dynamic_values : false,
        declarations : std::vector<*mut CSSDeclaration>(),
        className : std::string_view()
    }
    var cssParser = CSSParser();
    while(true) {
        var decl = cssParser.parseDeclaration(parser, builder);
        if(decl) {
            root.declarations.push(decl)
        } else {
            break;
        }
    }
    root.has_dynamic_values = cssParser.has_dynamic_values;
    return root;
}