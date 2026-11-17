func is_event_attribute_name(name : std::string_view) : bool {
    return name.size() > 2 && name.at(0) == 'o' && name.at(1) == 'n';
}

func has_non_ssr_attr_value(attrValue : *AttributeValue) : bool {
        unsafe {
        if(attrValue == null) return false;
        switch(attrValue.kind) {
            AttributeValueKind.Chemical => {
                const chem = attrValue as *mut ChemicalAttributeValue;
                return chem.value != null && chem.value.getKind() == ValueKind.LambdaFunc;
            }
            AttributeValueKind.ChemicalValues => {
                const chem = attrValue as *mut ChemicalAttributeValues;
                for(var i : uint = 0; i < chem.values.size(); i++) {
                    const value = chem.values.at(i);
                    if(value != null && value.getKind() == ValueKind.LambdaFunc) return true;
                }
                return false;
            }
            default => return false
        }

        }
    }

// Builds an SsrAttributeList struct value for a component invocation. When `withClass` is
// non-empty, an additional `class` attribute carrying the generated class name is appended
// (renderHtmlAttrs merges it with any caller-provided class).
func (converter : &mut ASTConverter) build_ssr_attrs(element : *mut HtmlElement, withClass : std::string_view) : *mut Value {
        unsafe {
        const builder = converter.builder
        const location = intrinsics::get_raw_location()

        const ssrAttrLinkedNode = converter.support.ssrAttrLinkedNode
        const ssrTextLinkedNode = converter.support.ssrTextLinkedNode
        const ssrAttributeValueNode = converter.support.ssrAttributeValueNode
        const multipleAttributeValueNode = converter.support.multipleAttributeValueNode
        const ssrAttributeListNode = converter.support.ssrAttributeListNode

        const structValue = builder.make_struct_value(ssrAttributeListNode, location)

        const hasClass = withClass.size() > 0
        const hasAttrs = !element.attributes.empty()

        if(!hasAttrs && !hasClass) {
            structValue.add_value(std::string_view("data"), builder.make_null_value(location))
            structValue.add_value(std::string_view("size"), builder.make_ubigint_value(0, location))
            return structValue;
        }

        const ssrAttrLinkedType = builder.make_linked_type(std::string_view("SsrAttribute"), ssrAttrLinkedNode, location)
        const arrayValue = builder.make_array_value(ssrAttrLinkedType, location)
        const attrValues = arrayValue.get_values()

        var attrValConv = AttrValueConverter {
            pageNode : converter.support.pageNode,
            ssrTextNode : converter.support.ssrTextLinkedNode,
            ssrAttributeValueNode : converter.support.ssrAttributeValueNode,
            multipleAttributeValueNode : converter.support.multipleAttributeValueNode,
            parent : converter.parent
        }

        var pushedCount : ubigint = 0;
        for(var i = 0u; i < element.attributes.size(); i++) {
            const attr = element.attributes.at(i)
            if(is_event_attribute_name(attr.name) || has_non_ssr_attr_value(attr.value)) continue;
            // skip if a later attribute with same name exists
            var is_duplicate : bool = false;
            for(var j = i + 1u; j < element.attributes.size(); j++) {
                const nextAttr = element.attributes.at(j)
                if(nextAttr.name.equals(&attr.name)) {
                    is_duplicate = true;
                    break;
                }
            }
            if(is_duplicate) continue;

            const attrStructVal = builder.make_struct_value(ssrAttrLinkedNode, location)

            const nameStructVal = builder.make_struct_value(ssrTextLinkedNode, location)
            nameStructVal.add_value(std::string_view("data"), builder.make_string_value(&attr.name, location))
            nameStructVal.add_value(std::string_view("size"), builder.make_ubigint_value(attr.name.size(), location))
            attrStructVal.add_value(std::string_view("name"), nameStructVal)

            const attrValue = attr.value;
            if(attrValue == null) {
                const textStructVal = builder.make_struct_value(ssrTextLinkedNode, location);
                textStructVal.add_value(std::string_view("data"), builder.make_string_value(builder.allocate_view(std::string_view("true")), location));
                textStructVal.add_value(std::string_view("size"), builder.make_ubigint_value(4, location));
                attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, "Text", textStructVal));
            } else if(attrValue.kind == AttributeValueKind.Chemical) {
                var chemAttrValue = attrValue as *mut ChemicalAttributeValue
                var attrValueVal = attrValConv.convert_to_attr_value(builder, chemAttrValue.value.getType(), chemAttrValue.value)
                attrStructVal.add_value(std::string_view("value"), attrValueVal);
            } else if(attrValue.kind == AttributeValueKind.ChemicalValues) {
                var chemAttrValue = attrValue as *mut ChemicalAttributeValues
                const multiVal = attrValConv.convert_multiple_attr_values(builder, chemAttrValue.values.data(), chemAttrValue.values.size())
                attrStructVal.add_value(std::string_view("value"), multiVal);
            } else {
                var chemAttrValue = attrValue as *mut TextAttributeValue
                const textStructVal = builder.make_struct_value(ssrTextLinkedNode, location)
                // Store the raw text; escaping happens once at render time
                // (appendHtmlEscaped/appendJsEscaped). Pre-escaping here double-escaped
                // values when rendered as child text (e.g. "→" -> "&#8594;" -> "&amp;#8594;").
                var stripped = strip_js_string_quotes(chemAttrValue.text);
                textStructVal.add_value(std::string_view("data"), builder.make_string_value(builder.allocate_view(&stripped), location))
                textStructVal.add_value(std::string_view("size"), builder.make_ubigint_value(stripped.size(), location))
                attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, "Text", textStructVal));
            }

            attrValues.push(attrStructVal)
            pushedCount++;
        }

        if(hasClass) {
            const classAttrStruct = builder.make_struct_value(ssrAttrLinkedNode, location)
            const nameStruct = builder.make_struct_value(ssrTextLinkedNode, location)
            nameStruct.add_value(std::string_view("data"), builder.make_string_value(builder.allocate_view(std::string_view("class")), location))
            nameStruct.add_value(std::string_view("size"), builder.make_ubigint_value(5, location))
            classAttrStruct.add_value(std::string_view("name"), nameStruct)
            const valueStruct = builder.make_struct_value(ssrTextLinkedNode, location)
            valueStruct.add_value(std::string_view("data"), builder.make_string_value(&withClass, location))
            valueStruct.add_value(std::string_view("size"), builder.make_ubigint_value(withClass.size(), location))
            classAttrStruct.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, "Text", valueStruct))
            attrValues.push(classAttrStruct)
            pushedCount++;
        }

        structValue.add_value(std::string_view("data"), arrayValue)
        structValue.add_value(std::string_view("size"), builder.make_ubigint_value(pushedCount, location))
        return structValue;

        }
    }

// Renders the children of a component invocation into an SsrText struct value by capturing
// the html buffer between two points and truncating.
func (converter : &mut ASTConverter) build_ssr_children(element : *mut HtmlElement, idLoc : ubigint) : *mut Value {
        unsafe {
        const builder = converter.builder
        const location = intrinsics::get_raw_location()

        if(element.children.empty()) {
            const ssrTextStructVal = builder.make_struct_value(converter.support.ssrTextLinkedNode, location);
            ssrTextStructVal.add_value(std::string_view("data"), builder.make_null_value(location));
            ssrTextStructVal.add_value(std::string_view("size"), builder.make_ubigint_value(0, location));
            return ssrTextStructVal;
        }

        // 1. Capture current HTML size
        var pageId2 = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
        var getHtmlSizeId = builder.make_identifier(std::string_view("get_html_size"), converter.support.getHtmlSizeFn, false, location)
        var getSizeCall = builder.make_function_call_value(
            builder.make_access_chain(&std::span<*mut Value>([ pageId2 as *mut Value, getHtmlSizeId ]), location),
            location
        );

        var startIdxNameStr = std::string();
        startIdxNameStr.append_view("startIdx_");
        startIdxNameStr.append_uinteger(idLoc);
        var startIdxName = builder.allocate_view(startIdxNameStr.to_view());

        var startIdxVar = builder.make_varinit_stmt(false, false, &startIdxName, builder.get_u64_type(), getSizeCall, AccessSpecifier.Internal, converter.parent, location);
        converter.vec.push(startIdxVar as *mut ASTNode);

        // 2. Render children. When the parent will emit these children as client
        // vnodes (see emit_universal_queue), suppress the children's own hydration
        // dispatches so they are not mounted twice.
        const childrenAreVnodes = children_are_static(&element.children);
        const prevSuppress = converter.suppress_child_dispatch;
        if(childrenAreVnodes) converter.suppress_child_dispatch = true;
        for(var i : uint = 0; i < element.children.size(); i++) {
             converter.convertHtmlChild(element.children.at(i));
        }
        converter.suppress_child_dispatch = prevSuppress;

        converter.put_chain_in()

        // 3. Extract range and truncate
        var pageId3 = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
        var pageHtmlId = builder.make_identifier(std::string_view("pageHtml"), converter.support.pageHtmlNode, false, location)
        var pageHtmlAccess = builder.make_access_chain(&std::span<*mut Value>([ pageId3, pageHtmlId ]), location);

        var childrenHtmlNameStr = std::string();
        childrenHtmlNameStr.append_view("childrenHtml_");
        childrenHtmlNameStr.append_uinteger(idLoc);
        var childrenHtmlName = builder.allocate_view(childrenHtmlNameStr.to_view());

        var childrenHtmlVar = builder.make_varinit_stmt(false, false, &childrenHtmlName, null,
            builder.make_function_call_value(builder.make_identifier(std::string_view("std::string"), converter.support.stringNodeMake, false, location), location),
            AccessSpecifier.Internal, converter.parent, location);
        converter.vec.push(childrenHtmlVar as *mut ASTNode);

        var childrenHtmlId = builder.make_identifier(&childrenHtmlName, childrenHtmlVar as *mut ASTNode, false, location);
        var appendWithLenId = builder.make_identifier(std::string_view("append_with_len"), converter.support.appendWithLenFn, false, location)
        var appendCall = builder.make_function_call_node(
            builder.make_access_chain(&std::span<*mut Value>([ childrenHtmlId, appendWithLenId ]), location),
            converter.parent,
            location
        );
        var startIdxId = builder.make_identifier(&startIdxName, startIdxVar as *mut ASTNode, false, location);
        var dataId = builder.make_identifier(std::string_view("data"), converter.support.dataFn, false, location)
        var dataCall = builder.make_function_call_value(
            builder.make_access_chain(&std::span<*mut Value>([ pageHtmlAccess as *mut Value, dataId ]), location),
            location
        );
        var sizeId = builder.make_identifier(std::string_view("size"), converter.support.sizeFn, false, location)
        var sizeCall = builder.make_function_call_value(
            builder.make_access_chain(&std::span<*mut Value>([ pageHtmlAccess as *mut Value, sizeId ]), location),
            location
        );

        appendCall.get_args().push(builder.make_expression_value(dataCall as *mut Value, startIdxId as *mut Value, Operation.Addition, dataCall.getType(), location));
        appendCall.get_args().push(builder.make_expression_value(sizeCall as *mut Value, startIdxId as *mut Value, Operation.Subtraction, sizeCall.getType(), location));
        converter.vec.push(appendCall as *mut ASTNode);

        var pageId4 = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
        var truncateHtmlId = builder.make_identifier(std::string_view("truncate_html"), converter.support.truncateHtmlFn, false, location)
        var truncateCall = builder.make_function_call_node(
            builder.make_access_chain(&std::span<*mut Value>([ pageId4 as *mut Value, truncateHtmlId ]), location),
            converter.parent,
            location
        );
        truncateCall.get_args().push(builder.make_identifier(&startIdxName, startIdxVar as *mut ASTNode, false, location));
        converter.vec.push(truncateCall as *mut ASTNode);

        // 4. Construct SsrText
        const ssrTextStructVal = builder.make_struct_value(converter.support.ssrTextLinkedNode, location);
        var dataCall2 = builder.make_function_call_value(
            builder.make_access_chain(&std::span<*mut Value>([ childrenHtmlId, dataId ]), location),
            location
        );
        var sizeCall2 = builder.make_function_call_value(
            builder.make_access_chain(&std::span<*mut Value>([ childrenHtmlId as *mut Value, sizeId ]), location),
            location
        );

        ssrTextStructVal.add_value(std::string_view("data"), dataCall2);
        ssrTextStructVal.add_value(std::string_view("size"), sizeCall2);
        return ssrTextStructVal;

        }
    }

// Attribute names every component accepts regardless of its declared props:
// the universal conventions plus the entities any JSX element may carry.
func is_implicit_prop_name(name : std::string_view) : bool {
    if(name.equals(std::string_view("children"))) return true
    if(name.equals(std::string_view("class"))) return true
    if(name.equals(std::string_view("className"))) return true
    if(name.equals(std::string_view("key"))) return true
    if(name.equals(std::string_view("ref"))) return true
    if(name.equals(std::string_view("style"))) return true
    return is_event_attribute_name(name)
}

// Validates a component invocation's attribute names against its declared props
// struct (`#universal Card(props : CardProps)`). This is the check that stops a
// prop typo from becoming `undefined` in the browser, which is the error class
// the JSX stack used to leave to the browser console.
func (converter : &mut ASTConverter) validate_props(element : *mut HtmlElement, signature : *mut ComponentSignature) {
        unsafe {
        const propsStruct = signature.propsStruct
        if(propsStruct == null) return
        if(converter.diagnoser == null) return
        const members = propsStruct.getMembers()
        const memberCount : uint = if(members == null) 0u else members.size()

        for(var i : uint = 0; i < element.attributes.size(); i++) {
            const attr = element.attributes.at(i)
            if(is_implicit_prop_name(attr.name)) continue
            var member : *mut BaseDefMember = null
            for(var j : uint = 0; j < memberCount; j++) {
                const candidate : *mut BaseDefMember = members.get(j)
                if(candidate.getName().equals(&attr.name)) { member = candidate; break }
            }
            if(member == null) {
                var msg = std::string("unknown prop '")
                msg.append_view(&attr.name)
                msg.append_view("' for component '")
                msg.append_view(&signature.name)
                msg.append_view("'")
                if(memberCount > 0) {
                    msg.append_view("; declared props: ")
                    for(var j : uint = 0; j < memberCount; j++) {
                        if(j > 0) { msg.append_view(", ") }
                        const candidate : *mut BaseDefMember = members.get(j)
                        msg.append_view(&candidate.getName())
                    }
                } else {
                    msg.append_view("; the props struct declares no fields")
                }
                converter.diagnoser.error(msg.to_view(), element.loc)
                continue
            }
            // Declared prop: check the value against the field's type using the
            // type checker's own rule, so implicit conversions behave exactly as
            // they do for a field assignment.
            if(attr.value == null) continue
            if(attr.value.kind != AttributeValueKind.Chemical) continue
            const chemVal = attr.value as *mut ChemicalAttributeValue
            if(chemVal.value == null) continue
            const fieldType = member.getType()
            if(fieldType == null) continue
            if(fieldType.satisfies_value(chemVal.value, true)) continue

            var expected : char[96]
            var got : char[96]
            const expLen = unsafe(fieldType.representation(unsafe(&raw mut expected[0]), 96))
            var gotLen : ubigint = 0
            const valType = chemVal.value.getType()
            if(valType != null) {
                gotLen = unsafe(valType.representation(unsafe(&raw mut got[0]), 96))
            }
            var msg = std::string("prop '")
            msg.append_view(&attr.name)
            msg.append_view("' of component '")
            msg.append_view(&signature.name)
            msg.append_view("' expects '")
            msg.append_with_len(unsafe(&raw mut expected[0]), expLen)
            msg.append_view("' but got '")
            if(gotLen == 0) {
                msg.append_view("an unknown type")
            } else {
                msg.append_with_len(unsafe(&raw mut got[0]), gotLen)
            }
            msg.append('\'')
            converter.diagnoser.error(msg.to_view(), element.loc)
        }

        // Required props: a field without an initializer must be supplied. (The
        // `#html` attribute model has no spread, so every field is checkable here;
        // the JSX path handles spreads separately.)
        for(var j : uint = 0; j < memberCount; j++) {
            const member : *mut BaseDefMember = members.get(j)
            // A field with a default may be omitted at the call site. The value is
            // injected as a real attribute, so the SSR prop pass and the client
            // props payload both carry it — injecting into one alone would make
            // hydration disagree with SSR (§3.15a). The JSX path does the same
            // through `ResolvedAttr.defaultValue`.
            const defaultValue = member.getDefaultValue()
            if(defaultValue != null) {
                const attrValue = converter.builder.allocate<ChemicalAttributeValue>()
                attrValue.kind = AttributeValueKind.Chemical
                attrValue.value = defaultValue
                const injected = converter.builder.allocate<HtmlAttribute>()
                injected.name = member.getName()
                injected.value = unsafe(attrValue as *AttributeValue)
                injected.loc = element.loc
                element.attributes.push(injected)
                continue
            }
            var present : bool = false
            for(var i : uint = 0; i < element.attributes.size(); i++) {
                if(element.attributes.at(i).name.equals(&member.getName())) { present = true; break }
            }
            if(present) continue
            var msg = std::string("missing required prop '")
            msg.append_view(&member.getName())
            msg.append_view("' for component '")
            msg.append_view(&signature.name)
            msg.append_view("'")
            converter.diagnoser.error(msg.to_view(), element.loc)
        }

        }
}

func (converter : &mut ASTConverter) convertHtmlComponent(element : *mut HtmlElement) {
        unsafe {
        // 0. Flush any pending HTML
        converter.put_chain_in()

        const builder = converter.builder
        const location = intrinsics::get_raw_location()
        const signature = element.componentSignature

        // 0. Typed props: reject attribute names the component does not declare.
        converter.validate_props(element, signature)

        // 1. Generate the hash based on component name
        const hash = signature.functionNode.getEncodedLocation()

        // Styled components: render <tag class="hash">children</tag> (or forward to a wrapped
        // component) by calling the generated SSR function with the attribute list + children.
        // No hydration <span> / JS queue is emitted (pure SSR + injected CSS).
        if(signature.mountStrategy == MountStrategy.Styled) {
            const attrsVal = converter.build_ssr_attrs(element, signature.className)
            const childrenVal = converter.build_ssr_children(element, element.loc)

            var compBase = builder.make_identifier(&signature.name, signature.functionNode as *mut ASTNode, false, location)
            var compCall = builder.make_function_call_node(compBase as *mut Value, converter.parent, location)
            compCall.get_args().push(builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location) as *mut Value)
            compCall.get_args().push(builder.make_addr_of_value(attrsVal, true, location) as *mut Value)
            compCall.get_args().push(childrenVal as *mut Value)
            converter.vec.push(compCall as *mut ASTNode)
            converter.put_chain_in();
            return;
        }

        // 2. Generate the if(page.require_component(hash)) block to emit component JS.
        if(signature.mountStrategy != MountStrategy.Universal) {
            var base = builder.make_identifier(&signature.name, signature.functionNode as *mut ASTNode, false, location)
            var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
            var call = builder.make_function_call_node(base as *mut Value, converter.parent, location)
            call.get_args().push(pageId as *mut Value)
            converter.vec.push(call)
        }

        // special case for universal component rendering
        // it just requires a single function call with the props
        if(signature.mountStrategy == MountStrategy.Universal) {

            const idLoc = element.loc

            // Allocate this instance's hydration id once, here: the marker (emitted
            // after the children) and the client dispatch (emitted now) must carry
            // the same value, and nested components allocate ids in between.
            var hidNameStr = std::string("hid_");
            hidNameStr.append_uinteger(idLoc);
            var hidName = builder.allocate_view(hidNameStr.to_view());

            var hidPageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
            const nextHidFn = converter.support.pageNode.child(std::string_view("next_hydration_id"));
            var hidNextId = builder.make_identifier(std::string_view("next_hydration_id"), nextHidFn, false, location);
            const nextHidCall = builder.make_function_call_value(
                builder.make_access_chain(&std::span<*mut Value>([ hidPageId as *mut Value, hidNextId ]), location),
                location
            );
            var hidVar = builder.make_varinit_stmt(false, false, &hidName, builder.get_u64_type(), nextHidCall, AccessSpecifier.Internal, converter.parent, location);
            converter.vec.push(hidVar as *mut ASTNode);
            var hidIdent = builder.make_identifier(&hidName, hidVar as *mut ASTNode, false, location);

            // Emit the hydration dispatch BEFORE compiling the children. The
            // dispatch is a client-side statement, and child components nested in
            // this element's children are compiled (and get their own dispatch)
            // while build_ssr_children runs below. Emitting the parent first makes
            // it mount before its independently-dispatched children, so scoped
            // context (useContext) resolves down the tree and disposal follows it.
            //
            // Inside table structure a wrapper element is invalid: the browser
            // foster-parents it and destroys the table. There we emit a comment
            // boundary and let the client hydrate the component's own root element.
            const markerBoundary = converter.in_table_context;

            if(!converter.suppress_child_dispatch) {
                converter.emit_universal_queue(element, signature, hidIdent, markerBoundary);
            }

            // `signature.className` carries the generated class name for a styled
            // wrap over a universal component (set by css_cbi during symres). For
            // plain universal components it is empty, so this is a no-op there.
            const attrsVal = converter.build_ssr_attrs(element, signature.className)
            const childrenVal = converter.build_ssr_children(element, idLoc)

            var compBase = builder.make_identifier(&signature.name, signature.functionNode as *mut ASTNode, false, location)
            var compPageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
            var compCall = builder.make_function_call_node(compBase as *mut Value, converter.parent, location)
            compCall.get_args().push(compPageId as *mut Value)
            compCall.get_args().push(builder.make_addr_of_value(attrsVal, true, location) as *mut Value)
            compCall.get_args().push(childrenVal as *mut Value)

            if(markerBoundary) {
                // Comment boundary: valid in any HTML context (including <tr>).
                converter.put_chain_in();
                converter.emit_page_stmt_with_value(std::string_view("begin_hydration_comment"), hidIdent);
                converter.vec.push(compCall as *mut ASTNode)
                converter.put_chain_in();
            } else {
                // 2. Emit <span id="u<id>" data-chx-i>  (inline-safe boundary)
                converter.put_chain_in();
                converter.emit_page_stmt_with_value(std::string_view("begin_hydration_marker"), hidIdent);

                converter.vec.push(compCall as *mut ASTNode)
                converter.put_chain_in(); // Flush any pending HTML before </div>

                // 5. Emit </span>
                converter.str.append_view("</span>");
                converter.put_chain_in();
            }

            // 6. Hydration dispatch was emitted before the children were compiled
            // (see the top of this branch) so mount order is parent-before-child.
            return;
        }

        var s = &mut converter.str

        s.append_view("<script>")

        if(signature.mountStrategy == MountStrategy.Universal) {
            s.append_view("$_um(document.currentScript, ")
            get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, &mut *s)
            s.append_view(", {")
        } else {
            s.append_view("$_dm(document.currentScript, ")
            get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, &mut *s)
            s.append_view(", {")
        }

        const attrs = element.attributes.size()
        for (var i : uint = 0; i < attrs; i++) {
            if (i > 0) s.append_view(", ")
            const attr = element.attributes.at(i)
            s.append_view(&attr.name)
            s.append_view(": ")

            if (attr.value != null) {
                switch(attr.value.kind) {
                    AttributeValueKind.Text, AttributeValueKind.Number => {
                        const val = attr.value as *mut TextAttributeValue
                        s.append_view(&val.text)
                    }
                    AttributeValueKind.Chemical => {
                        const val = attr.value as *mut ChemicalAttributeValue
                        const type = val.value.getType()
                        const is_str = converter.is_string_type(type)

                        if(is_str) {
                             s.append('"')
                        }

                        converter.emit_append_html_from_str(&mut *s)

                        converter.put_chemical_value_in(val.value)

                        if(is_str) {
                            converter.put_char_chain('"')
                        }
                    }
                    AttributeValueKind.ChemicalValues => {
                        converter.emit_append_html_from_str(&mut *s)
                        const valuesNode = attr.value as *mut ChemicalAttributeValues
                        converter.emit_append_html_call(builder.make_string_value(builder.allocate_view(std::string_view("\"")), location), 1)
                        for (var j : uint = 0; j < valuesNode.values.size(); j++) {
                            if (j > 0) converter.emit_append_html_call(builder.make_string_value(builder.allocate_view(std::string_view(" ")), location), 1)
                            converter.put_chemical_value_in(valuesNode.values.at(j))
                        }
                        converter.emit_append_html_call(builder.make_string_value(builder.allocate_view(std::string_view("\"")), location), 1)
                    }
                }
            } else {
                s.append_view("true")
            }
        }

        s.append_view("});")

        s.append_view("</script>")
        converter.emit_append_html_from_str(&mut *s)

        }
    }
