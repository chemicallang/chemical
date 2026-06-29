@no_mangle
public func universal_replacementNodeDeclare(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut JsComponentDecl;
    return root.signature.functionNode ;
}

func make_require_component_call_static(builder : *mut ASTBuilder, support : &mut SymResSupport, hash : size_t, location : ubigint) : *mut FunctionCall {
    var value = builder.make_ubigint_value(hash, location)
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("require_component"), support.requireComponentFn, false, location);
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func make_set_component_hash_call_static(builder : *mut ASTBuilder, support : &mut SymResSupport, hash : size_t, parent : *mut ASTNode, location : ubigint) : *mut FunctionCallNode {
    var value = builder.make_ubigint_value(hash, location)
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("set_component_hash"), support.setComponentHashFn, false, location);
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
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
        support : &raw mut support,
        vec : body,
        parent : funcNode,
        str : std::string(),
        jsx_parent : view(""),
        t_counter : 0,
        state_vars : std::vector<std::string_view>(),
        state_inits : std::vector<JsStateInit>(),
        current_func : funcNode,
        component_props_name : root.signature.propsName
    }

    const nodeLocation = value.getEncodedLocation()
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
            var idx_s = std::string("idx_")
            idx_s.append_uinteger(location)
            const indexName = builder.allocate_view(idx_s.to_view());
            
            var ph_s = std::string("ph_")
            ph_s.append_uinteger(location)
            const prevHoistName = builder.allocate_view(ph_s.to_view());
            
            var pageId = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);

            // Record index and prev_hoist
            var getJsPosId = builder.make_identifier(std::string_view("get_js_pos"), support.getJsPosFn, false, location)
            const getJsPosCall = builder.make_function_call_value(builder.make_access_chain(&std::span<*mut Value>([ pageId, getJsPosId ]), location), location);
            var indexVar = builder.make_varinit_stmt(false, false, &indexName, builder.get_u64_type(), getJsPosCall, AccessSpecifier.Internal, converter.parent, location);
            converter.vec.push(indexVar);

            var jsHoistPosId = builder.make_identifier(std::string_view("js_hoist_pos"), support.js_hoist_pos, false, location)
            const hoistPosAccess = builder.make_access_chain(&std::span<*mut Value>([ pageId, jsHoistPosId ]), location);
            var prevHoistVar = builder.make_varinit_stmt(false, false, &prevHoistName, builder.get_u64_type(), hoistPosAccess, AccessSpecifier.Internal, converter.parent, location);
            converter.vec.push(prevHoistVar);

            var checkRequired = make_require_component_call_static(builder, &mut support, selfHash, location)
            var ifRequiredStmt = builder.make_if_stmt(checkRequired, converter.parent, location);
            const ifRequiredBody = ifRequiredStmt.get_body()
            ifRequiredBody.push(make_set_component_hash_call_static(builder, &mut support, selfHash, converter.parent, location))

            // generating the js into the if required body
            converter.target = BufferType.JavaScript;
            const rootBody = converter.vec
            converter.vec = ifRequiredBody
            append_universal_component_js(&mut converter, root);
            converter.put_chain_in();
            converter.vec = rootBody

            // put the if statement
            converter.vec.push(ifRequiredStmt)

            // Perform hoisting logic
            const currentPosCall = builder.make_function_call_value(builder.make_access_chain(&std::span<*mut Value>([ pageId, getJsPosId ]), location), location);
            
            var fe_s = std::string("fe_")
            fe_s.append_uinteger(location)
            const fromEndName = builder.allocate_view(fe_s.to_view());
            var fromEndVar = builder.make_varinit_stmt(false, false, &fromEndName, builder.get_u64_type(), currentPosCall, AccessSpecifier.Internal, converter.parent, location);
            converter.vec.push(fromEndVar);

            // delta = page->js_hoist_pos - prev_hoist
            const currentHoistPos = builder.make_access_chain(&std::span<*mut Value>([ pageId, jsHoistPosId ]), location);
            const deltaVal = builder.make_expression_value(currentHoistPos, builder.make_identifier(&prevHoistName, prevHoistVar, false, location), Operation.Subtraction, builder.get_u64_type(), location);
            
            var dl_s = std::string("dl_")
            dl_s.append_uinteger(location)
            const deltaName = builder.allocate_view(dl_s.to_view());
            var deltaVar = builder.make_varinit_stmt(false, false, &deltaName, builder.get_u64_type(), deltaVal, AccessSpecifier.Internal, converter.parent, location);
            converter.vec.push(deltaVar);

            // updated_index = index + delta
            const updatedIndexVal = builder.make_expression_value(builder.make_identifier(&indexName, indexVar, false, location), builder.make_identifier(&deltaName, deltaVar, false, location), Operation.Addition, builder.get_u64_type(), location);
            
            var ui_s = std::string("ui_")
            ui_s.append_uinteger(location)
            const updatedIndexName = builder.allocate_view(ui_s.to_view());
            var updatedIndexVar = builder.make_varinit_stmt(false, false, &updatedIndexName, builder.get_u64_type(), updatedIndexVal, AccessSpecifier.Internal, converter.parent, location);
            converter.vec.push(updatedIndexVar);

            var moveJsRangeId = builder.make_identifier(std::string_view("move_js_range"), support.moveJsRangeFn, false, location)

            // move_js_range(updated_index, fromEnd, page->js_hoist_pos)
            var moveCall = builder.make_function_call_node(
                builder.make_access_chain(&std::span<*mut Value>([ pageId, moveJsRangeId ]), location),
                converter.parent,
                location
            );
            moveCall.get_args().push(builder.make_identifier(&updatedIndexName, updatedIndexVar, false, location));
            moveCall.get_args().push(builder.make_identifier(&fromEndName, fromEndVar, false, location));
            moveCall.get_args().push(builder.make_access_chain(&std::span<*mut Value>([ pageId, jsHoistPosId ]), location));
            converter.vec.push(moveCall);

            // page->js_hoist_pos += (fromEnd - updated_index)
            const deltaLenVal = builder.make_expression_value(builder.make_identifier(&fromEndName, fromEndVar, false, location), builder.make_identifier(&updatedIndexName, updatedIndexVar, false, location), Operation.Subtraction, builder.get_u64_type(), location);
            const updateHoistPosVal = builder.make_expression_value(builder.make_access_chain(&std::span<*mut Value>([ pageId, jsHoistPosId ]), location), deltaLenVal, Operation.Addition, builder.get_u64_type(), location);
            converter.vec.push(builder.make_assignment_stmt(builder.make_access_chain(&std::span<*mut Value>([ pageId, jsHoistPosId ]), location), updateHoistPosVal, Operation.Assignment, converter.parent, location));

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
