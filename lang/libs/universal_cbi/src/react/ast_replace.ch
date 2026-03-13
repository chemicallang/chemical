@no_mangle
public func universal_replacementNodeDeclare(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut JsComponentDecl;
    return root.signature.functionNode as *mut ASTNode;
}

func emit_child_component_js_calls(
    builder : *mut ASTBuilder,
    parent : *mut ASTNode,
    components : &std::vector<*mut JsJSXElement>,
    support : &mut SymResSupport,
    vec : *mut VecRef<ASTNode>,
    emitted : &mut std::vector<size_t>,
    location : ubigint
) {
    for(var i : uint = 0; i < components.size(); i++) {
        const element = components.get(i);
        if(element.componentSignature != null) {
            const signature = element.componentSignature;
            const hash = signature.functionNode.getEncodedLocation() as size_t;

            var already_emitted = false;
            for(var j : uint = 0; j < emitted.size(); j++) {
                if(emitted.get(j) == hash) {
                    already_emitted = true;
                    break;
                }
            }

            if(!already_emitted) {
                emitted.push(hash);

                var requireCall = make_require_component_call_static(builder, support, hash, location)
                var ifStmt = builder.make_if_stmt(requireCall as *mut Value, parent, location)
                var thenBody = ifStmt.get_body()

                thenBody.push(make_set_component_hash_call_static(builder, support, hash, parent, location))

                var targetNode = signature.functionNode as *mut FunctionDeclaration;
                var targetName = signature.name;
                if(signature.mountStrategy == MountStrategy.Universal && signature.jsEmitFunctionNode != null) {
                    targetNode = signature.jsEmitFunctionNode;
                    targetName = signature.jsEmitFunctionNode.getName();
                }
                var base = builder.make_identifier(targetName, targetNode as *mut ASTNode, false, location)
                var pageId = builder.make_identifier(std::string_view("page"), support.pageNode, false, location)
                var call = builder.make_function_call_node(base as *mut ChainValue, parent, location)
                call.get_args().push(pageId as *mut Value)
                thenBody.push(call as *mut ASTNode)

                vec.push(ifStmt as *mut ASTNode)
            }
        }
    }
}

func make_require_component_call_static(builder : *mut ASTBuilder, support : &mut SymResSupport, hash : size_t, location : ubigint) : *mut FunctionCall {
    var value = builder.make_ubigint_value(hash, location)
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("require_component"), support.requireComponentFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func make_set_component_hash_call_static(builder : *mut ASTBuilder, support : &mut SymResSupport, hash : size_t, parent : *mut ASTNode, location : ubigint) : *mut FunctionCallNode {
    var value = builder.make_ubigint_value(hash, location)
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("set_component_hash"), support.setComponentHashFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, parent, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

@no_mangle
public func universal_replacementNode(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut JsComponentDecl;
    const body = root.signature.functionNode.add_body();

    var support = root.support

    var converter = JsConverter {
        builder : builder,
        support : &mut support,
        vec : body,
        parent : root.signature.functionNode as *mut ASTNode,
        str : std::string(),
        jsx_parent : view(""),
        t_counter : 0,
        state_vars : std::vector<std::string_view>()
    }

    const location = intrinsics::get_raw_location()

    var emitted = std::vector<size_t>();
    emit_child_component_js_calls(builder, root.signature.functionNode as *mut ASTNode, root.components, support, body, emitted, location);

    if(root.signature.jsEmitFunctionNode != null) {
        const selfHash = root.signature.functionNode.getEncodedLocation() as size_t;
        converter.put_chain_in()
        var selfReq = make_require_component_call_static(builder, support, selfHash, location)
        var selfIf = builder.make_if_stmt(selfReq as *mut Value, converter.parent, location)
        var selfBody = selfIf.get_body()
        selfBody.push(make_set_component_hash_call_static(builder, support, selfHash, converter.parent, location))
        var emitBase = builder.make_identifier(root.signature.jsEmitFunctionNode.getName(), root.signature.jsEmitFunctionNode as *mut ASTNode, false, location)
        var emitPage = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
        var emitCall = builder.make_function_call_node(emitBase as *mut ChainValue, converter.parent, location)
        emitCall.get_args().push(emitPage as *mut Value)
        selfBody.push(emitCall as *mut ASTNode)
        converter.vec.push(selfIf as *mut ASTNode)
    }

    if(root.body != null && root.body.kind == JsNodeKind.Block) {
        const block = root.body as *mut JsBlock;
        const returned = find_returned_jsx(block);
        if(returned != null) {
            // 1. Perform SSR (emit to pageHtml buffer)
            converter.target = BufferType.HTML;
            converter.convertJsNode(returned);
            converter.put_chain_in();

            // 2. Perform hydration script emission (emit to pageJs buffer)
            // The JsJSXComponent conversion inside convertJsNode(returned) already
            // emits hydration for child components.
            // We just need to make sure the ROOT of THIS component also hydrates if needed.
            // Actually, the Component itself is a target of hydration in the parent.
        }
    }

    if(root.signature.jsEmitFunctionNode != null) {
        const emitBody = root.signature.jsEmitFunctionNode.add_body();
        var emitSupport = root.support;

        var emitted_js = std::vector<size_t>();
        emit_child_component_js_calls(builder, root.signature.jsEmitFunctionNode as *mut ASTNode, root.components, emitSupport, emitBody, emitted_js, location);

        var emitConverter = JsConverter {
            builder : builder,
            support : &mut emitSupport,
            vec : emitBody,
            parent : root.signature.jsEmitFunctionNode as *mut ASTNode,
            str : std::string(),
            jsx_parent : view(""),
            t_counter : 0,
            state_vars : std::vector<std::string_view>()
        }
        emitConverter.target = BufferType.JavaScript;
        append_universal_component_js(emitConverter, root);
        emitConverter.put_chain_in();
    }

    var scope = builder.make_scope(root.signature.functionNode.getParent(), location);
    var scope_nodes = scope.getNodes();
    if(root.signature.jsEmitFunctionNode != null) {
        scope_nodes.push(root.signature.jsEmitFunctionNode as *mut ASTNode);
    }
    scope_nodes.push(root.signature.functionNode as *mut ASTNode);
    return scope;
}
