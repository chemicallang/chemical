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
                var base = builder.make_identifier(targetName, targetNode as *mut ASTNode, false, location)
                var pageId = builder.make_identifier(std::string_view("page"), support.pageNode, false, location)
                var call = builder.make_function_call_node(base as *mut Value, parent, location)
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
        state_vars : std::vector<std::string_view>(),
        current_func : root.signature.functionNode
    }

    const location = intrinsics::get_raw_location()

    var emitted = std::vector<size_t>();
    // emit_child_component_js_calls(builder, root.signature.functionNode as *mut ASTNode, root.components, support, body, emitted, location);

    if(root.body != null && root.body.kind == JsNodeKind.Block) {
        const block = root.body as *mut JsBlock;
        const returned = find_returned_jsx(block);
        if(returned != null) {
            // 1. JS definition emission (protected by require_component)
            // This ALWAYS happens even if attrs is null, to ensure the component JS is available.
            const selfHash = root.signature.functionNode.getEncodedLocation() as size_t;
            var selfReq = make_require_component_call_static(builder, support, selfHash, location)
            var selfIf = builder.make_if_stmt(selfReq as *mut Value, converter.parent, location)
            var selfBody = selfIf.get_body()
            selfBody.push(make_set_component_hash_call_static(builder, support, selfHash, converter.parent, location))
            
            var jsConv = JsConverter {
                builder : builder,
                support : &mut support,
                vec : selfBody,
                parent : selfIf as *mut ASTNode,
                str : std::string(),
                jsx_parent : view(""),
                t_counter : 0,
                state_vars : std::vector<std::string_view>(),
                current_func : root.signature.functionNode
            }
            jsConv.target = BufferType.JavaScript;
            append_universal_component_js(jsConv, root);
            jsConv.put_chain_in();
            
            converter.vec.push(selfIf as *mut ASTNode);

            // 2. Early return if attrs is null
            // if(attrs == null) return;
            const curr_func_params = root.signature.functionNode.get_params();
            const second_param = curr_func_params.get(1);
            var attrsId = builder.make_identifier(std::string_view("attrs"), second_param, false, location);
            var nullVal = builder.make_null_value(location);
            var condition = builder.make_expression_value(attrsId as *mut Value, nullVal as *mut Value, Operation.IsEqual, location);
            var ifNullStmt = builder.make_if_stmt(condition as *mut Value, converter.parent, location);
            var returnStmt = builder.make_return_stmt(null, null, converter.parent, location);
            ifNullStmt.get_body().push(returnStmt as *mut ASTNode);
            converter.vec.push(ifNullStmt as *mut ASTNode);

            // 3. SSR HTML emission
            converter.target = BufferType.HTML;
            converter.convertJsNode(returned);
            converter.put_chain_in();

            // 4. Hydration call (on every call site)
            var pageId = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
            const funcId = builder.make_identifier(std::string_view("append_js_char_ptr"), support.appendHeadJsCharPtrFn, false, location);
            const chain = builder.make_access_chain(std::span<*mut Value>([ pageId, funcId ]), location);

            var callHeader = builder.make_function_call_node(chain, converter.parent, location);
            var headerStr = std::string("$_um(document.currentScript, '");
            get_module_scoped_name(root.signature.functionNode as *mut ASTNode, root.signature.name, headerStr);
            headerStr.append_view("', {");
            callHeader.get_args().push(builder.make_string_value(builder.allocate_view(headerStr.to_view()), location));
            converter.vec.push(callHeader as *mut ASTNode);

            var callAttrs = builder.make_function_call_node(builder.make_identifier(std::string_view("renderJsAttrs"), support.renderJsAttrs, false, location), converter.parent, location);
            callAttrs.get_args().push(pageId as *mut Value);
            callAttrs.get_args().push(builder.make_identifier(std::string_view("attrs"), second_param, false, location)); 
            converter.vec.push(callAttrs as *mut ASTNode);

            var callTail = builder.make_function_call_node(chain, converter.parent, location);
            callTail.get_args().push(builder.make_string_value(view("});"), location));
            converter.vec.push(callTail as *mut ASTNode);
        }
    }

    var scope = builder.make_scope(root.signature.functionNode.getParent(), location);
    var scope_nodes = scope.getNodes();
    scope_nodes.push(root.signature.functionNode as *mut ASTNode);
    return scope;
}
