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
            var condition = builder.make_expression_value(attrsId, nullVal, Operation.IsEqual, location);
            var ifNullStmt = builder.make_if_stmt(condition, converter.parent, location);
            var returnStmt = builder.make_return_stmt(null, null, converter.parent, location);
            ifNullStmt.get_body().push(returnStmt);
            converter.vec.push(ifNullStmt);

            // 3. SSR HTML emission
            converter.target = BufferType.HTML;
            const location2 = location
            const pageId2 = builder.make_identifier(std::string_view("page"), support.pageNode, false, location2);
            const second_param2 = curr_func_params.get(1);

            // Generate unique uId: var uId = page.get_next_u_id();
            const chain = builder.make_access_chain(std::span<*mut Value>([ pageId2, builder.make_identifier(std::string_view("get_next_u_id"), support.getNextUIdFn, false, location2) ]), location2);
            var getNextIdCall = builder.make_function_call_value(chain, location2);
            var uIdDecl = builder.make_varinit_stmt(true, false, std::string_view("uId"), null, getNextIdCall, AccessSpecifier.Internal, converter.parent, location2);
            converter.vec.push(uIdDecl);
            var uIdVal = builder.make_identifier(std::string_view("uId"), uIdDecl, false, location2);

            // Emit <div id="u
            converter.str.append_view("<div id=\"u");
            converter.put_chain_in();
            
            // Emit uId
            const chain2 = builder.make_access_chain(std::span<*mut Value>([ pageId2, builder.make_identifier(std::string_view("append_html_uinteger"), support.appendHtmlUIntFn, false, location2) ]), location2);
            var appendUIntCall = builder.make_function_call_node(chain2, converter.parent, location2);
            appendUIntCall.get_args().push(uIdVal as *mut Value);
            converter.vec.push(appendUIntCall as *mut ASTNode);

            // Emit " data-u-comp="
            converter.str.append_view("\" data-u-comp=\"");
            get_module_scoped_name(root.signature.functionNode as *mut ASTNode, root.signature.name, converter.str);
            converter.str.append_view("\">");
            converter.put_chain_in();

            // Emit the actual component content
            converter.convertJsNode(returned);
            converter.put_chain_in();

            // Emit </div>
            converter.str.append_view("</div>");
            converter.put_chain_in();

            // 4. Hydration call: window.$_uq.push(['u{uId}', 'Name', {props}])
            // This goes to HeadJS (or just JS)
            const jsHeaderId = builder.make_identifier(std::string_view("append_js_char_ptr"), support.appendHeadJsCharPtrFn, false, location2);
            const jsHeaderChain = builder.make_access_chain(std::span<*mut Value>([ pageId2, jsHeaderId ]), location2);
            
            // window.$_uq.push(['u
            var callPushStart = builder.make_function_call_node(jsHeaderChain, converter.parent, location2);
            callPushStart.get_args().push(builder.make_string_value(view("window.$_uq.push(['u"), location2));
            converter.vec.push(callPushStart as *mut ASTNode);

            // {uId}
            var jsUIntId = builder.make_identifier(std::string_view("append_js_uinteger"), support.appendHeadJsUIntFn, false, location2);
            var jsUIntChain = builder.make_access_chain(std::span<*mut Value>([ pageId2, jsUIntId ]), location2);
            var callPushId = builder.make_function_call_node(jsUIntChain, converter.parent, location2);
            callPushId.get_args().push(uIdVal as *mut Value);
            converter.vec.push(callPushId as *mut ASTNode);

            // ', 'Name', {
            var pushMidStr = std::string("','");
            get_module_scoped_name(root.signature.functionNode as *mut ASTNode, root.signature.name, pushMidStr);
            pushMidStr.append_view("',{");
            var callPushMid = builder.make_function_call_node(jsHeaderChain, converter.parent, location2);
            callPushMid.get_args().push(builder.make_string_value(builder.allocate_view(pushMidStr.to_view()), location2));
            converter.vec.push(callPushMid as *mut ASTNode);

            // {props} -> renderJsAttrs
            var callAttrs = builder.make_function_call_node(builder.make_identifier(std::string_view("renderJsAttrs"), support.renderJsAttrs, false, location2), converter.parent, location2);
            callAttrs.get_args().push(pageId2 as *mut Value);
            callAttrs.get_args().push(builder.make_identifier(std::string_view("attrs"), second_param2, false, location2));
            converter.vec.push(callAttrs as *mut ASTNode);

            // }]);
            var callPushEnd = builder.make_function_call_node(jsHeaderChain, converter.parent, location2);
            callPushEnd.get_args().push(builder.make_string_value(view("}]);"), location2));
            converter.vec.push(callPushEnd as *mut ASTNode);
        }
    }

    var scope = builder.make_scope(root.signature.functionNode.getParent(), location);
    var scope_nodes = scope.getNodes();
    scope_nodes.push(root.signature.functionNode as *mut ASTNode);
    return scope;
}
