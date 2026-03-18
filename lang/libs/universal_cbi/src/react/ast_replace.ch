@no_mangle
public func universal_replacementNodeDeclare(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut JsComponentDecl;
    return root.signature.functionNode ;
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
                var ifStmt = builder.make_if_stmt(requireCall, parent, location)
                var thenBody = ifStmt.get_body()

                thenBody.push(make_set_component_hash_call_static(builder, support, hash, parent, location))

                var targetNode = signature.functionNode as *mut FunctionDeclaration;
                var targetName = signature.name;
                var base = builder.make_identifier(targetName, targetNode , false, location)
                var pageId = builder.make_identifier(std::string_view("page"), support.pageNode, false, location)
                var call = builder.make_function_call_node(base, parent, location)
                call.get_args().push(pageId)
                thenBody.push(call)

                vec.push(ifStmt)
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

    var emitted = std::vector<size_t>();
    // emit_child_component_js_calls(builder, root.signature.functionNode , root.components, support, body, emitted, location);

    if(root.body != null && root.body.kind == JsNodeKind.Block) {
        const block = root.body as *mut JsBlock;
        const returned = find_returned_jsx(block);
        if(returned != null) {

            // the encoded location of the component used as a hash
            const selfHash = funcNode.getEncodedLocation() as size_t;

            // 1. We are going to emit this
            // var no_ssr_html = attrs == null
            // if(no_ssr_html && !require_component(hash)) {
            //  return;
            // }
            // reason: we return early if user only wanted the js and it already exists

            // generating: var no_ssr_html = atts == null
            const curr_func_params = funcNode.get_params();
            const second_param = curr_func_params.get(1);
            var attrsId = builder.make_identifier(std::string_view("attrs"), second_param, false, location);
            var nullVal = builder.make_null_value(location);
            var checkAttrsNull = builder.make_expression_value(attrsId, nullVal, Operation.IsEqual, builder.make_bool_type(), location);
            var no_ssr_html = builder.make_varinit_stmt(true, false, std::string_view("no_ssr_html"), null, checkAttrsNull, AccessSpecifier.Internal, funcNode, location)
            converter.vec.push(no_ssr_html);

            // generating:
            // !require_component(hash)
            var checkRequired = make_require_component_call_static(builder, support, selfHash, location)
            const checkNotRequired = builder.make_not_value(checkRequired, location)

            // generating:
            // no_ssr_html && !require_component(hash)
            const no_ssr_html_id = builder.make_identifier("no_ssr_html", no_ssr_html, false, location);
            const condition = builder.make_expression_value(no_ssr_html_id, checkNotRequired, Operation.LogicalAND, builder.make_bool_type(), location)

            // generating:
            // if(...) { return; }
            var ifNullStmt = builder.make_if_stmt(condition, converter.parent, location);
            var returnStmt = builder.make_return_stmt(null, converter.parent, location);
            ifNullStmt.get_body().push(returnStmt);
            converter.vec.push(ifNullStmt);

            // 2. Reset attributes if they are NULL
            // if(no_ssr_html) {
            //  attrs = &SsrAttributeList { data : null, size : 0 }
            // }
            var ifNoAttrs = builder.make_if_stmt(no_ssr_html_id, converter.parent, location);
            const ifNoAttrsBody = ifNoAttrs.get_body();
            const listStruct = builder.make_struct_value(converter.support.ssrAttributeListNode, location);
            listStruct.add_value(std::string_view("data"), builder.make_null_value(location));
            listStruct.add_value(std::string_view("size"), builder.make_ubigint_value(0, location));
            const addrOf = builder.make_addr_of_value(listStruct, location)
            const assign = builder.make_assignment_stmt(attrsId, addrOf, Operation.Assignment, ifNoAttrs, location)
            ifNoAttrsBody.push(assign)
            converter.vec.push(ifNoAttrs)

            // 2. Record html index
            // var ssr_html_index = page.get_html_size()
            var pageId = builder.make_identifier(std::string_view("page"), support.pageNode, false, location)
            var getHtmlSize = builder.make_identifier(std::string_view("get_html_size"), support.getHtmlSizeFn, false, location)
            const getHtmlSizeId = builder.make_access_chain(std::span<*mut Value>([ pageId, getHtmlSize ]), location)
            const getHtmlSizeCall = builder.make_function_call_value(getHtmlSizeId, location)
            var ssr_html_index = builder.make_varinit_stmt(true, false, std::string_view("ssr_html_index"), null, getHtmlSizeCall, AccessSpecifier.Internal, funcNode, location)
            converter.vec.push(ssr_html_index)
            var ssr_html_index_id = builder.make_identifier("ssr_html_index", ssr_html_index, false, location);

            // 3. HTML emission
            // Emit the actual component content, going into html buffer
            converter.target = BufferType.HTML;
            converter.convertJsNode(returned);
            converter.put_chain_in();

            // 4. JS definition emission
            // append the html into the js bundle (for template creation)
            // page.pageJs.append_with_len(page.pageHtml.data() + startIdx, page.pageHtml.size() - startIdx)
            const pageJsId = builder.make_identifier(std::string_view("pageJs"), converter.support.pageJsNode, false, location)
            const appendWithLenId = builder.make_identifier(std::string_view("append_with_len"), converter.support.appendWithLenFn, false, location)
            var pageJsAppendAccess = builder.make_access_chain(std::span<*mut Value>([ pageId, pageJsId, appendWithLenId ]), location);
            const appendCall = builder.make_function_call_node(pageJsAppendAccess, funcNode, location)
            var pageHtmlAccess = builder.make_access_chain(std::span<*mut Value>([ pageId, builder.make_identifier(std::string_view("pageHtml"), converter.support.pageHtmlNode, false, location) ]), location);
            var dataCall = builder.make_function_call_value(
                builder.make_access_chain(std::span<*mut Value>([ pageHtmlAccess as *mut Value, builder.make_identifier(view("data"), converter.support.dataFn, false, location) ]), location),
                location
            );
            var sizeCall = builder.make_function_call_value(
                builder.make_access_chain(std::span<*mut Value>([ pageHtmlAccess as *mut Value, builder.make_identifier(view("size"), converter.support.sizeFn, false, location) ]), location),
                location
            );
            appendCall.get_args().push(builder.make_expression_value(dataCall, ssr_html_index_id, Operation.Addition, dataCall.getType(), location));
            appendCall.get_args().push(builder.make_expression_value(sizeCall, ssr_html_index_id, Operation.Subtraction, sizeCall.getType(), location));
            converter.target = BufferType.JavaScript;
            append_universal_component_js(converter, root, appendCall);
            converter.put_chain_in();

            // 5. Remove html if no_ssr_html
            // if(no_ssr_html) {
            //  page.truncate_html(ssr_html_index)
            // }
            var ifNoSsr = builder.make_if_stmt(no_ssr_html_id, converter.parent, location);
            const ifNoSsrBody = ifNoSsr.get_body();
            // the truncate call
            var truncateHtml = builder.make_identifier(std::string_view("truncate_html"), support.truncateHtmlFn, false, location)
            const truncateCallId = builder.make_access_chain(std::span<*mut Value>([ pageId,truncateHtml  ]), location)
            const truncateCall = builder.make_function_call_node(truncateCallId, ifNoSsr, location)
            truncateCall.get_args().push(ssr_html_index_id)
            ifNoSsrBody.push(truncateCall);

            converter.vec.push(ifNoSsr);

        }
    }

    var scope = builder.make_scope(root.signature.functionNode.getParent(), location);
    var scope_nodes = scope.getNodes();
    scope_nodes.push(root.signature.functionNode );
    return scope;
}
