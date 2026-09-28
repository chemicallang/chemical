@no_mangle
public func js_symResNode(visitor : *mut SymResLinkBody, node : *mut EmbeddedNode) {
    visitor.visitEmbeddedNode(node)
    const resolver = visitor.getSymbolResolver();
    const loc = node.getEncodedLocation();
    const root = node.getDataPtr() as *mut JsRoot;
    sym_res_root(root, visitor, loc)
}

@no_mangle
public func js_replacementNode(builder : *mut ASTBuilder, diagnoser : *mut ASTDiagnoser, value : *mut EmbeddedNode) : *ASTNode {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut JsRoot;
    var scope = builder.make_scope(root.parent, loc);
    var scope_nodes = scope.getNodes();
    var converter = JsConverter {
        builder : builder,
        support : &raw mut root.support,
        vec : scope_nodes,
        parent : root.parent,
        str : std::string()
    }

    if(root.shared) {
        // `#globaljs`: emit the whole block exactly once, keyed by this macro's
        // source location. With a shared sink attached the guard lives in the
        // sink, so N pages emit it once; without a sink it lands on the page.
        // The appends themselves route to the sink because no `begin_local_js`
        // bracket is opened.
        const key = value.getEncodedLocation()
        var ifStmt = builder.make_if_stmt(make_require_js_hash_call(&mut converter, key, loc), root.parent, loc);
        const guardBody = ifStmt.get_body()
        guardBody.push(make_set_js_hash_call(&mut converter, key, root.parent, loc))
        converter.vec = guardBody
        converter.convertJsRoot(root)
        converter.vec = scope_nodes
        scope_nodes.push(ifStmt as *mut ASTNode)
    } else {
        // `#js` is always the page's own JS (page bundle), never the shared bundle.
        const beginFn = converter.support.beginLocalJsFn
        scope_nodes.push(js_page_noarg_call(&mut converter, beginFn, std::string_view("begin_local_js"), loc) as *mut ASTNode)
        converter.convertJsRoot(root);
        const endFn = converter.support.endLocalJsFn
        scope_nodes.push(js_page_noarg_call(&mut converter, endFn, std::string_view("end_local_js"), loc) as *mut ASTNode)
    }
    return scope;
}

// Builds a no-argument call on the page object: `page.<name>()`.
func js_page_noarg_call(converter : &mut JsConverter, fnNode : *mut ASTNode, fnName : std::string_view, loc : ubigint) : *mut FunctionCallNode {
    const builder = converter.builder
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, loc)
    var id = builder.make_identifier(&fnName, fnNode, false, loc)
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), loc)
    return builder.make_function_call_node(chain, converter.parent, loc)
}

// Builds `page.require_js_hash(<hash>)` (a value call, usable as an if condition).
func make_require_js_hash_call(converter : &mut JsConverter, hash : ubigint, loc : ubigint) : *mut FunctionCall {
    const builder = converter.builder
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, loc)
    var id = builder.make_identifier(std::string_view("require_js_hash"), converter.support.requireJsHashFn, false, loc)
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), loc)
    var call = builder.make_function_call_value(chain, loc)
    call.get_args().push(builder.make_ubigint_value(hash, loc))
    return call
}

// Builds `page.set_js_hash(<hash>)` (a statement call).
func make_set_js_hash_call(converter : &mut JsConverter, hash : ubigint, parent : *mut ASTNode, loc : ubigint) : *mut FunctionCallNode {
    const builder = converter.builder
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, loc)
    var id = builder.make_identifier(std::string_view("set_js_hash"), converter.support.setJsHashFn, false, loc)
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), loc)
    var call = builder.make_function_call_node(chain, parent, loc)
    call.get_args().push(builder.make_ubigint_value(hash, loc))
    return call
}

public func node_known_type_func(value : *EmbeddedNode) : *BaseType {
    return null;
}

public func node_child_res_func(value : *EmbeddedNode, name : &std::string_view) : *ASTNode {
    return null;
}

@no_mangle
public func js_symResValue(visitor : *mut SymResLinkBody, value : *mut EmbeddedValue) : bool {
    visitor.visitEmbeddedValue(value)
    const resolver = visitor.getSymbolResolver();
    const loc = value.getEncodedLocation()
    const root = value.getDataPtr() as *mut JsRoot;
    sym_res_root(root, visitor, loc)
    return true;
}

@no_mangle
public func js_replacementValue(builder : *mut ASTBuilder, diagnoser : *mut ASTDiagnoser, value : *EmbeddedValue) : *Value {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut JsRoot;
    var block_val = builder.make_block_value(root.parent, loc)
    var scope_nodes = block_val.get_body()
    
    var converter = JsConverter {
        builder : builder,
        support : &raw mut root.support,
        vec : scope_nodes,
        parent : root.parent,
        str : std::string()
    }
    
    // `#js` is always the page's own JS (page bundle), never the shared bundle.
    const beginFn = converter.support.beginLocalJsFn
    scope_nodes.push(js_page_noarg_call(&mut converter, beginFn, std::string_view("begin_local_js"), loc) as *mut ASTNode)
    converter.convertJsRoot(root);
    const endFn = converter.support.endLocalJsFn
    scope_nodes.push(js_page_noarg_call(&mut converter, endFn, std::string_view("end_local_js"), loc) as *mut ASTNode)
    
    const view2 = builder.allocate_view(converter.str.to_view())
    const strValue = builder.make_string_value(&view2, loc)
    block_val.setCalculatedValue(strValue)
    return block_val;
}

@no_mangle
public func js_parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
    const tok = parser.getToken()
    const loc = parser.getEncodedLocation(tok)
    if(parser.increment_if(JsTokenType.LBrace as int)) {
        var root = parseJsRoot(parser, builder);
        const type = builder.make_string_type(loc)
        const nodes_arr : []*mut ASTNode = []
        const value = builder.make_embedded_value(std::string_view("js"), root, type, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), loc);
        if(!parser.increment_if(JsTokenType.RBrace as int)) {
            parser.error("expected a rbrace for ending the js macro");
        }
        return value;
    } else {
        parser.error("expected a lbrace");
    }
    return null;
}

@no_mangle
public func js_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
    const tok = parser.getToken()
    const loc = parser.getEncodedLocation(tok)
    if(parser.increment_if(JsTokenType.LBrace as int)) {
        var root = parseJsRoot(parser, builder);
        const nodes_arr : []*mut ASTNode = []
        const node = builder.make_embedded_node(AccessSpecifier.Internal, std::string_view("js"), root, node_known_type_func, node_child_res_func, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), root.parent, loc);
        if(!parser.increment_if(JsTokenType.RBrace as int)) {
            parser.error("expected a rbrace for ending the js macro");
        }
        return node;
    } else {
        parser.error("expected a lbrace");
        return null;
    }
}

// `#globaljs { ... }` — app-wide JS. Unlike `#js` (always the page's own JS), a
// globaljs block prefers the shared assets sink when the page has one attached,
// falling back to the page, and is emitted once per source location (see
// `js_replacementNode`). It reuses the `js` embedded node with `shared = true`.
@no_mangle
public func globaljs_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
    const tok = parser.getToken()
    const loc = parser.getEncodedLocation(tok)
    if(parser.increment_if(JsTokenType.LBrace as int)) {
        var root = parseJsRoot(parser, builder);
        root.shared = true
        const nodes_arr : []*mut ASTNode = []
        const node = builder.make_embedded_node(AccessSpecifier.Internal, std::string_view("js"), root, node_known_type_func, node_child_res_func, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), root.parent, loc);
        if(!parser.increment_if(JsTokenType.RBrace as int)) {
            parser.error("expected a rbrace for ending the globaljs macro");
        }
        return node;
    } else {
        parser.error("expected a lbrace");
        return null;
    }
}

public func getNextToken(js : &mut JsLexer, lexer : &mut Lexer) : Token {
    return nextJsToken(js, lexer, false)
}
@no_mangle
public func js_initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(sizeof(JsLexer), alignof(JsLexer)) as *mut JsLexer;
    new (ptr) JsLexer { }
    lexer.setUserLexer(ptr, getNextToken as UserLexerSubroutineType)
}
