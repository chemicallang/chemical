import "@compiler/Token.ch"
import "@compiler/Lexer.ch"
import "./lexer/TokenType.ch"
import "@compiler/Parser.ch"
import "@compiler/ASTBuilder.ch"
import "@compiler/ChemicalTokenType.ch"
import "./ast/HtmlElement.ch"
import "./lexer/HtmlLexer.ch"
import "./parser/root.ch"

public func parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
    printf("wow create macro value\n");
    const loc = compiler::get_raw_location();
    if(parser.increment_if(TokenType.LBrace)) {
        const value = builder.make_int_value(10, loc);
        if(!parser.increment_if(TokenType.RBrace)) {
            parser.error("expected a rbrace");
        }
        return value;
    } else {
        parser.error("expected a lbrace");
    }
    return null;
}

public func parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
    printf("wow create macro node\n");
    const loc = compiler::get_raw_location();
    if(parser.increment_if(TokenType.LBrace)) {
        var scope = builder.make_scope(parser.getParentNode(), loc);
        var scope_nodes = scope.getNodes();
        var str = std::string();
        var root = parseHtmlRoot(parser, builder);
        printf("parsed to html root\n")
        fflush(null)
        convertHtmlRoot(builder, root, scope_nodes, str);
        if(!parser.increment_if(TokenType.RBrace)) {
            parser.error("expected a rbrace for ending the html macro");
        }
        return scope;
    } else {
        parser.error("expected a lbrace");
    }
}

public func getNextToken(html : &mut HtmlLexer, lexer : &mut Lexer) : Token {
    if(html.other_mode) {
        if(html.chemical_mode) {
            var nested = lexer.getEmbeddedToken();
            if(nested.type == ChemicalTokenType.LBrace) {
                html.lb_count++;
                printf("lb_count increases to %d in chemical mode\n", html.lb_count);
            } else if(nested.type == ChemicalTokenType.RBrace) {
                html.lb_count--;
                printf("lb_count decreased to %d in chemical mode\n", html.lb_count);
                if(html.lb_count == 1) {
                    html.other_mode = false;
                    html.chemical_mode = false;
                    printf("since lb_count decreased to 1, we're switching to html mode\n");
                }
            }
            printf("in chemical mode, created token '%s'\n", nested.value.data());
            return nested;
        }
    }
    const t = getNextToken2(html, lexer);
    printf("I created token : '%s' with type %d\n", t.value.data(), t.type);
    return t;
}

public func initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(#sizeof(HtmlLexer), #alignof(HtmlLexer)) as *mut HtmlLexer;
    const x = new (ptr) HtmlLexer {
        has_lt : false,
        other_mode : false,
        chemical_mode : false,
        lb_count : 0
    }
    lexer.other_mode = true;
    lexer.user_mode = true;
    lexer.user_lexer = UserLexerFn {
        instance : ptr,
        subroutine : getNextToken
    }
}