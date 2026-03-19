@no_mangle
public func universal_replacementNodeDeclare(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut JsComponentDecl;
    return root.signature.functionNode ;
}

func make_require_component_call_static(builder : *mut ASTBuilder, support : &mut SymResSupport, hash : size_t, location : ubigint) : *mut FunctionCall {
    var value = builder.make_ubigint_value(hash, location)
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("require_component"), support.requireComponentFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func make_set_component_hash_call_static(builder : *mut ASTBuilder, support : &mut SymResSupport, hash : size_t, parent : *mut ASTNode, location : ubigint) : *mut FunctionCallNode {
    var value = builder.make_ubigint_value(hash, location)
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("set_component_hash"), support.setComponentHashFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, parent, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

@no_mangle
public func universal_replacementNode(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut JsComponentDecl;
    const funcNode = root.signature.functionNode;
    const body = funcNode.add_body();

    var support = root.support

    var converter = JsConverter {
        builder : builder,
        support : &mut support,
        vec : body,
        parent : funcNode,
        str : std::string(),
        jsx_parent : view(""),
        t_counter : 0,
        state_vars : std::vector<std::string_view>(),
        current_func : funcNode
    }

    const location = intrinsics::get_raw_location()

    if(root.body != null && root.body.kind == JsNodeKind.Block) {
        const block = root.body as *mut JsBlock;
        const returned = find_returned_jsx(block);
        if(returned != null) {

            // Each universal component is very simple
            // it generates this in C
            // if(required_component(hash)) { set_component_required(hash, false); append_to_js_buffer("function Component(...) {...}") }
            // append_to_html_buffer("<span>SSR HTML</span>")

            // the encoded location of the component used as a hash
            const selfHash = funcNode.getEncodedLocation() as size_t;

            // 0. Put Existing Chain in
            converter.put_chain_in();

            // 1. Put Component Js Function (if required)
            // if(require_component(hash)) { set_required(); ...js... }
            var checkRequired = make_require_component_call_static(builder, support, selfHash, location)
            var ifRequiredStmt = builder.make_if_stmt(checkRequired, converter.parent, location);
            const ifRequiredBody = ifRequiredStmt.get_body()
            ifRequiredBody.push(make_set_component_hash_call_static(builder, support, selfHash, converter.parent, location))

            // generating the js into the if required body
            converter.target = BufferType.JavaScript;
            const rootBody = converter.vec
            converter.vec = ifRequiredBody
            append_universal_component_js(converter, root);
            converter.put_chain_in();
            converter.vec = rootBody

            // put the if statement
            converter.vec.push(ifRequiredStmt)

            // 3. HTML emission
            // Emit the actual component content, going into html buffer
            converter.target = BufferType.HTML;
            converter.convertJsNode(returned);
            converter.put_chain_in();

        }
    }

    var scope = builder.make_scope(root.signature.functionNode.getParent(), location);
    var scope_nodes = scope.getNodes();
    scope_nodes.push(root.signature.functionNode );
    return scope;
}
