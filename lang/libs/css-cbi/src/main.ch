import "@compiler/Token.ch"
import "@compiler/Lexer.ch"
import "./lexer/TokenType.ch"
import "@compiler/Parser.ch"
import "@compiler/ASTBuilder.ch"
import "@compiler/ChemicalTokenType.ch"
import "./lexer/CSSLexer.ch"
import "./parser/cssom.ch"
import "@compiler/ast/base/Value.ch"
import "@compiler/ast/base/ASTNode.ch"
import "@compiler/SymbolResolver.ch"


public func parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
    printf("wow create macro node\n");
    const loc = compiler::get_raw_location();
    if(parser.increment_if(TokenType.LBrace)) {
        var root = parseCSSOM(parser, builder);
        printf("parsed to css om\n")
        fflush(null)
        const node = builder.make_sym_res_node(symResNodeDeclaration, symResNodeReplacement, root, root.parent, loc);
        if(!parser.increment_if(TokenType.RBrace)) {
            parser.error("expected a rbrace for ending the html macro");
        }
        return node;
    } else {
        parser.error("expected a lbrace");
    }
}

func symResNodeDeclaration(allocator : *mut ASTBuilder, resolver : *mut SymbolResolver, data : **mut void) {

}

func symResNodeReplacement(builder : *mut ASTBuilder, resolver : *mut SymbolResolver, data : *mut void) : *mut ASTNode {
    const loc = compiler::get_raw_location();
    const root = data as *mut CSSOM;
    var scope = builder.make_scope(root.parent, loc);
    var scope_nodes = scope.getNodes();
    var str = std::string();
    convertCSSOM(resolver, builder, root, scope_nodes, str);
    return scope;
}

public func parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
    printf("wow create macro node\n");
    const loc = compiler::get_raw_location();
    if(parser.increment_if(TokenType.LBrace)) {
        var root = parseCSSOM(parser, builder);
        printf("parsed to css om\n")
        fflush(null)
        const node = builder.make_sym_res_node(symResNodeDeclaration, symResNodeReplacement, root, root.parent, loc);
        if(!parser.increment_if(TokenType.RBrace)) {
            parser.error("expected a rbrace for ending the css macro");
        }
        return node;
    } else {
        parser.error("expected a lbrace");
    }
}

public func getNextToken(css : &mut CSSLexer, lexer : &mut Lexer) : Token {
    if(css.other_mode) {
        if(css.chemical_mode) {
            var nested = lexer.getEmbeddedToken();
            if(nested.type == ChemicalTokenType.LBrace) {
                css.lb_count++;
                printf("lb_count increases to %d in chemical mode\n", css.lb_count);
            } else if(nested.type == ChemicalTokenType.RBrace) {
                css.lb_count--;
                printf("lb_count decreased to %d in chemical mode\n", css.lb_count);
                if(css.lb_count == 1) {
                    css.other_mode = false;
                    css.chemical_mode = false;
                    printf("since lb_count decreased to 1, we're switching to css mode\n");
                }
            }
            printf("in chemical mode, created token '%s'\n", nested.value.data());
            return nested;
        }
    }
    const t = getNextToken2(css, lexer);
    printf("I created token : '%s' with type %d\n", t.value.data(), t.type);
    return t;
}

public func initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(sizeof(CSSLexer), alignof(CSSLexer)) as *mut CSSLexer;
    new (ptr) CSSLexer {
        other_mode : false,
        chemical_mode : false,
        lb_count : 0
    }
    lexer.setUserLexer(ptr, getNextToken)
}