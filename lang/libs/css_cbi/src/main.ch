
func symResValueReplacement(builder : *mut ASTBuilder, resolver : *mut SymbolResolver, data : *mut void) : *mut Value {
    printf("running css symResValueReplacement\n");
    fflush(null)
    const loc = intrinsics::get_raw_location();
    const root = data as *mut CSSOM;
    var scope = builder.make_block_value(root.parent, loc);
    var scope_nodes = scope.get_body();
    var str = std::string();
    convertCSSOM(resolver, builder, root, scope_nodes, str);
    const classNameVal = builder.make_string_value(root.className, loc)
    const node = builder.make_value_wrapper(classNameVal, root.parent)
    scope_nodes.push(node)
    return scope;
}

@no_mangle
public func css_parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
    printf("running css_parseMacroValue\n");
    fflush(null)
    const loc = intrinsics::get_raw_location();
    if(parser.increment_if(TokenType.LBrace as int)) {
        var root = parseCSSOM(parser, builder);
        printf("parsed to css om\n")
        fflush(null)
        const value = builder.make_sym_res_value(symResValueReplacement, root, loc);
        if(!parser.increment_if(TokenType.RBrace as int)) {
            parser.error("expected a rbrace for ending the css macro");
        }
        return value;
    } else {
        parser.error("expected a lbrace");
        return null;
    }
}

func symResNodeDeclaration(allocator : *mut ASTBuilder, resolver : *mut SymbolResolver, data : **mut void) {

}

func symResNodeReplacement(builder : *mut ASTBuilder, resolver : *mut SymbolResolver, data : *mut void) : *mut ASTNode {
    printf("running css symResNodeReplacement\n");
    fflush(null)
    const loc = intrinsics::get_raw_location();
    const root = data as *mut CSSOM;
    var scope = builder.make_scope(root.parent, loc);
    var scope_nodes = scope.getNodes();
    var str = std::string();
    convertCSSOM(resolver, builder, root, scope_nodes, str);
    return scope;
}

@no_mangle
public func css_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
    printf("running css_parseMacroNode\n");
    fflush(null)
    const loc = intrinsics::get_raw_location();
    if(parser.increment_if(TokenType.LBrace as int)) {
        var root = parseCSSOM(parser, builder);
        printf("parsed to css om\n")
        fflush(null)
        const node = builder.make_sym_res_node(symResNodeDeclaration, symResNodeReplacement, root, root.parent, loc);
        if(!parser.increment_if(TokenType.RBrace as int)) {
            parser.error("expected a rbrace for ending the css macro");
        }
        return node;
    } else {
        parser.error("expected a lbrace");
        return null;
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

@no_mangle
public func css_initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(sizeof(CSSLexer), alignof(CSSLexer)) as *mut CSSLexer;
    new (ptr) CSSLexer {
        other_mode : false,
        chemical_mode : false,
        lb_count : 0
    }
    lexer.setUserLexer(ptr, getNextToken)
}