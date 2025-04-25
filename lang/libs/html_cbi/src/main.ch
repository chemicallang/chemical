
@no_mangle
public func html_parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
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

func symResNodeDeclaration(allocator : *mut ASTBuilder, resolver : *mut SymbolResolver, data : **mut void) {

}

func symResNodeReplacement(builder : *mut ASTBuilder, resolver : *mut SymbolResolver, data : *mut void) : *mut ASTNode {
    printf("running html symResNodeReplacement\n");
    fflush(null)
    const loc = compiler::get_raw_location();
    const root = data as *mut HtmlRoot;
    var scope = builder.make_scope(root.parent, loc);
    var scope_nodes = scope.getNodes();
    var str = std::string();
    convertHtmlRoot(resolver, builder, root, scope_nodes, str);
    return scope;
}

@no_mangle
public func html_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
    printf("running html_parseMacroNode\n");
    fflush(null)
    const loc = compiler::get_raw_location();
    if(parser.increment_if(TokenType.LBrace)) {
        var root = parseHtmlRoot(parser, builder);
        printf("parsed to html root\n")
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
    if(html.is_comment) {
        const provider = &lexer.provider
        const position = provider.getPosition();
        if(provider.read_comment_text(lexer.str)) {
            // comment has ended
            html.is_comment = false;
        }
        return Token {
            type : TokenType.CommentText as int,
            value : lexer.str.finalize_view(),
            position : position
        }
    }
    const t = getNextToken2(html, lexer);
    printf("I created token : '%s' with type %d\n", t.value.data(), t.type);
    return t;
}

@no_mangle
public func html_initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(sizeof(HtmlLexer), alignof(HtmlLexer)) as *mut HtmlLexer;
    new (ptr) HtmlLexer {
        has_lt : false,
        is_comment : false,
        other_mode : false,
        chemical_mode : false,
        lb_count : 0
    }
    lexer.setUserLexer(ptr, getNextToken)
}