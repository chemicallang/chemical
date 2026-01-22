
struct JsConverter {
    var builder : *mut ASTBuilder
    var support : *mut SymResSupport
    var vec : *mut VecRef<ASTNode>
    var parent : *mut ASTNode
    var str : std::string
    var jsx_parent : std::string_view
    var t_counter : int = 0
}

func (converter : &mut JsConverter) next_t() : std::string {
    converter.t_counter++
    var res = std::string()
    res.append_view("$c_t")
    res.append_integer(converter.t_counter as bigint)
    return res
}

func (converter : &mut JsConverter) make_require_component_call(hash : size_t) : *mut FunctionCall {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var value = builder.make_ubigint_value(hash, location)
    const support = converter.support;
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("require_component"), support.requireComponentFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func (converter : &mut JsConverter) make_set_component_hash_call(hash : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var value = builder.make_ubigint_value(hash, location)
    return converter.make_value_call_with(value, std::string_view("set_component_hash"), converter.support.setComponentHashFn)
}


func (converter : &mut JsConverter) make_char_chain(value : char) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    
    var name = std::string_view("append_head_js_char")
    var fnPtr = converter.support.appendHeadJsCharFn

    var id = builder.make_identifier(name, fnPtr, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    const char_val = builder.make_char_value(value, location);
    args.push(char_val)
    return call;
}

func (converter : &mut JsConverter) make_value_call_with(value : *mut Value, fn_name : std::string_view, fnPtr : *mut ASTNode) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var id = builder.make_identifier(fn_name, fnPtr, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func (converter : &mut JsConverter) make_char_ptr_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_head_js_char_ptr"), converter.support.appendHeadJsCharPtrFn)
}

func (converter : &mut JsConverter) make_char_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_head_js_char"), converter.support.appendHeadJsCharFn)
}

func (converter : &mut JsConverter) make_integer_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_head_js_integer"), converter.support.appendHeadJsIntFn)
}

func (converter : &mut JsConverter) make_uinteger_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_head_js_uinteger"), converter.support.appendHeadJsUIntFn)
}

func (converter : &mut JsConverter) make_float_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_head_js_float"), converter.support.appendHeadJsFloatFn)
}

func (converter : &mut JsConverter) make_double_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_head_js_double"), converter.support.appendHeadJsDoubleFn)
}

func (converter : &mut JsConverter) make_value_call(value : *mut Value, len : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var name = std::string_view("append_head_js")
    
    var id = builder.make_identifier(name, converter.support.appendHeadJsFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    
    // len arg
    const len_val = builder.make_ubigint_value(len as ubigint, location);
    args.push(len_val)
    
    return call;
}

func (converter : &mut JsConverter) put_chain_in() {
    if(converter.str.empty()) return;
    
    const location = intrinsics::get_raw_location();
    const str_view = converter.builder.allocate_view(converter.str.to_view());
    const val = converter.builder.make_string_value(str_view, location);
    const call = converter.make_value_call(val, converter.str.size());
    converter.vec.push(call as *mut ASTNode);
    converter.str.clear();
}


func (converter : &mut JsConverter) convertChemicalValue(chem : *mut JsChemicalValue) {
    // TODO: this is remaining
}

func (converter : &mut JsConverter) convertJsNode(node : *mut JsNode) {
    if(node == null) return;
    switch(node.kind) {
        JsNodeKind.Literal => {
            var lit = node as *mut JsLiteral
            converter.str.append_view(lit.value);
        }
        JsNodeKind.Identifier => {
            var id = node as *mut JsIdentifier
            converter.str.append_view(id.value);
        }
        JsNodeKind.ChemicalValue => {
            converter.convertChemicalValue(node as *mut JsChemicalValue);
        }
        JsNodeKind.UnaryOp => {
            var unary = node as *mut JsUnaryOp
            if(unary.prefix) {
                converter.str.append_view(unary.operator);
                converter.convertJsNode(unary.operand);
            } else {
                converter.convertJsNode(unary.operand);
                converter.str.append_view(unary.operator);
            }
        }
        JsNodeKind.BinaryOp => {
            var bin = node as *mut JsBinaryOp
            converter.convertJsNode(bin.left);
            converter.str.append_view(" ");
            converter.str.append_view(bin.op);
            converter.str.append_view(" ");
            converter.convertJsNode(bin.right);
        }
        JsNodeKind.Ternary => {
            var tern = node as *mut JsTernary
            converter.convertJsNode(tern.condition);
            converter.str.append_view(" ? ");
            converter.convertJsNode(tern.consequent);
            converter.str.append_view(" : ");
            converter.convertJsNode(tern.alternate);
        }
        JsNodeKind.FunctionCall => {
            var call = node as *mut JsFunctionCall
            if(call.callee.kind == JsNodeKind.Identifier) {
                var id = call.callee as *mut JsIdentifier
                var name = id.value
                switch(fnv1_hash_view(name)) {
                    comptime_fnv1_hash("createSignal"), 
                    comptime_fnv1_hash("createEffect"), 
                    comptime_fnv1_hash("createMemo"), 
                    comptime_fnv1_hash("createResource"), 
                    comptime_fnv1_hash("onMount"), 
                    comptime_fnv1_hash("onCleanup"), 
                    comptime_fnv1_hash("batch"), 
                    comptime_fnv1_hash("untrack") => {
                         converter.str.append_view("$_s.")
                    }
                    default => {}
                }
            }
            converter.convertJsNode(call.callee);
            converter.str.append_view("(");
            for(var i : uint = 0; i < call.args.size(); i++) {
                if(i > 0) converter.str.append_view(", ");
                converter.convertJsNode(call.args.get(i));
            }
            converter.str.append_view(")");
        }
        JsNodeKind.MemberAccess => {
            var mem = node as *mut JsMemberAccess
            converter.convertJsNode(mem.object);
            converter.str.append_view(".");
            converter.str.append_view(mem.property);
        }
        JsNodeKind.IndexAccess => {
            var idx = node as *mut JsIndexAccess
            converter.convertJsNode(idx.object);
            converter.str.append_view("[");
            converter.convertJsNode(idx.index);
            converter.str.append_view("]");
        }
        JsNodeKind.ArrayLiteral, JsNodeKind.ArrayDestructuring => {
            var arr = node as *mut JsArrayLiteral
            converter.str.append_view("[");
            for(var i : uint = 0; i < arr.elements.size(); i++) {
                if(i > 0) converter.str.append_view(", ");
                const el = arr.elements.get(i);
                if(el != null) converter.convertJsNode(el);
            }
            converter.str.append_view("]");
        }
        JsNodeKind.ObjectLiteral => {
            var obj = node as *mut JsObjectLiteral
            converter.str.append_view("{");
            for(var i : uint = 0; i < obj.properties.size(); i++) {
                if(i > 0) converter.str.append_view(", ");
                converter.str.append_view(obj.properties.get(i).key);
                converter.str.append_view(": ");
                converter.convertJsNode(obj.properties.get(i).value);
            }
            converter.str.append_view("}");
        }
        JsNodeKind.ArrowFunction => {
             var arrow = node as *mut JsArrowFunction
             if(arrow.is_async) converter.str.append_view("async ");
             converter.str.append_view("(");
             for(var i : uint = 0; i < arrow.params.size(); i++) {
                 if(i > 0) converter.str.append_view(", ");
                 converter.str.append_view(arrow.params.get(i));
             }
             converter.str.append_view(") => ");
             converter.convertJsNode(arrow.body);
        }
        JsNodeKind.Block => {
             var block = node as *mut JsBlock
             converter.str.append_view("{ ");
             for(var i : uint = 0; i < block.statements.size(); i++) {
                 converter.convertJsNode(block.statements.get(i));
                 converter.str.append_view(" ");
             }
             converter.str.append_view("}");
        }
        JsNodeKind.ExpressionStatement => {
             var expr = node as *mut JsExpressionStatement
             converter.convertJsNode(expr.expression);
             converter.str.append_view(";");
        }
        JsNodeKind.VarDecl => {
             var decl = node as *mut JsVarDecl
             converter.str.append_view(decl.keyword);
             converter.str.append_view(" ");
             if(decl.pattern != null) {
                 converter.convertJsNode(decl.pattern);
             } else {
                 converter.str.append_view(decl.name);
             }
             if(decl.value != null) {
                 converter.str.append_view(" = ");
                 converter.convertJsNode(decl.value);
             }
             converter.str.append_view(";");
        }
        JsNodeKind.If => {
             var ifStmt = node as *mut JsIf
             converter.str.append_view("if(");
             converter.convertJsNode(ifStmt.condition);
             converter.str.append_view(") ");
             converter.convertJsNode(ifStmt.thenBlock);
             if(ifStmt.elseBlock != null) {
                 converter.str.append_view(" else ");
                 converter.convertJsNode(ifStmt.elseBlock);
             }
        }
        JsNodeKind.Return => {
             var ret = node as *mut JsReturn
             converter.str.append_view("return ");
             if(ret.value != null) converter.convertJsNode(ret.value);
             converter.str.append_view(";");
        }
        JsNodeKind.For => {
             var f = node as *mut JsFor
             converter.str.append_view("for(");
             if(f.init != null) converter.convertJsNode(f.init); else converter.str.append_view(";");
             if(f.condition != null) converter.convertJsNode(f.condition);
             converter.str.append_view(";");
             if(f.update != null) converter.convertJsNode(f.update);
             converter.str.append_view(") ");
             converter.convertJsNode(f.body);
        }
        JsNodeKind.ForIn => {
             var f = node as *mut JsForIn
             converter.str.append_view("for(");
             converter.convertJsNode(f.left);
             converter.str.append_view(" in ");
             converter.convertJsNode(f.right);
             converter.str.append_view(") ");
             converter.convertJsNode(f.body);
        }
        JsNodeKind.JSXElement => {
             converter.convertJSXElement(node as *mut JsJSXElement);
        }
        JsNodeKind.JSXFragment => {
             converter.convertJSXFragment(node as *mut JsJSXFragment);
        }
        JsNodeKind.JSXExpressionContainer => {
             var container = node as *mut JsJSXExpressionContainer
             // In Solid HyperScript, reactive expressions should be wrapped in functions.
             // We'll wrap all expressions in getters for simplicity/safety in this CBI,
             // UNLESS they are already functions (ArrowFunction).
             if(container.expression.kind != JsNodeKind.ArrowFunction && container.expression.kind != JsNodeKind.FunctionDecl) {
                 converter.str.append_view("() => ");
             }
             converter.convertJsNode(container.expression);
        }
        JsNodeKind.JSXText => {
             var text = node as *mut JsJSXText
             converter.str.append_view("` "); // Use backticks to support multi-line strings
             converter.str.append_view(text.value);
             converter.str.append_view(" `");
        }
        default => {
        }
    }
}

func (converter : &mut JsConverter) convertAttributeValue(attr : *mut JsJSXAttribute, isComponent : bool) {
    if(attr.value != null) {
         if(attr.value.kind == JsNodeKind.JSXExpressionContainer) {
             var container = attr.value as *mut JsJSXExpressionContainer
             // Check if it's an event handler (starts with 'on')
             var isEventHandler = attr.name.size() > 2 && attr.name.get(0) == 'o' && attr.name.get(1) == 'n' && attr.name.get(2) >= 'A' && attr.name.get(2) <= 'Z';
             
             if(isEventHandler) {
                 // For event handlers, we don't want the getter wrap from convertJsNode.
                 // So we convert the inner expression directly.
                 converter.convertJsNode(container.expression);
                 return;
             }
         }
         converter.convertJsNode(attr.value);
    } else {
         converter.str.append_view("true");
    }
}

func (converter : &mut JsConverter) convertJSXComponent(element : *mut JsJSXElement, tagName : std::string_view, tagNameNode : *mut JsNode) {
    converter.str.append_view("$_s.createComponent(");
    
    if(tagNameNode.kind == JsNodeKind.Identifier) {
        converter.str.append_view(tagName);
    } else {
        converter.convertJsNode(tagNameNode);
    }
    
    var hasSpread = false;
    for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
        if(element.opening.attributes.get(i).kind == JsNodeKind.JSXSpreadAttribute) {
            hasSpread = true;
            break;
        }
    }

    converter.str.append_view(", ");
    if(hasSpread) {
        converter.str.append_view("$_s.mergeProps(");
    }
    
    converter.str.append_view("{");
    
    var attrCount = 0;
    for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
        const attrNode = element.opening.attributes.get(i)
        if(attrNode.kind == JsNodeKind.JSXAttribute) {
            if(attrCount > 0) converter.str.append_view(", ");
            const attr = attrNode as *mut JsJSXAttribute
            converter.str.append_view("\"");
            converter.str.append_view(attr.name);
            converter.str.append_view("\": ");
            converter.convertAttributeValue(attr, true);
            attrCount++;
        } else if (attrNode.kind == JsNodeKind.JSXSpreadAttribute) {
            // Close current object, add spread argument, start next object
            converter.str.append_view("}, ");
            const spread = attrNode as *mut JsJSXSpreadAttribute
            converter.convertJsNode(spread.argument);
            converter.str.append_view(", {");
            attrCount = 0;
        }
    }
    
    converter.str.append_view("}");
    if(hasSpread) {
        converter.str.append_view(")");
    }
    
    if(!element.children.empty()) {
        for(var i : uint = 0; i < element.children.size(); i++) {
             converter.str.append_view(", ");
             converter.convertJsNode(element.children.get(i));
        }
    }
    
    converter.str.append_view(")");
}

func (converter : &mut JsConverter) convertJSXNativeElement(element : *mut JsJSXElement, tagName : std::string_view) {
    converter.str.append_view("$_sh(\"");
    converter.str.append_view(tagName);
    converter.str.append_view("\", ");
    
    var hasSpread = false;
    for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
        if(element.opening.attributes.get(i).kind == JsNodeKind.JSXSpreadAttribute) {
            hasSpread = true;
            break;
        }
    }

    if(hasSpread) {
        converter.str.append_view("$_s.mergeProps(");
    }
    
    converter.str.append_view("{");
    
    var attrCount = 0;
    for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
        const attrNode = element.opening.attributes.get(i)
        if(attrNode.kind == JsNodeKind.JSXAttribute) {
            if(attrCount > 0) converter.str.append_view(", ");
            const attr = attrNode as *mut JsJSXAttribute
            converter.str.append_view("\"");
            converter.str.append_view(attr.name);
            converter.str.append_view("\": ");
            converter.convertAttributeValue(attr, false);
            attrCount++;
        } else if (attrNode.kind == JsNodeKind.JSXSpreadAttribute) {
            converter.str.append_view("}, ");
            const spread = attrNode as *mut JsJSXSpreadAttribute
            converter.convertJsNode(spread.argument);
            converter.str.append_view(", {");
            attrCount = 0;
        }
    }
    
    converter.str.append_view("}");
    if(hasSpread) {
        converter.str.append_view(")");
    }
    
    if(!element.children.empty()) {
        for(var i : uint = 0; i < element.children.size(); i++) {
             converter.str.append_view(", ");
             converter.convertJsNode(element.children.get(i));
        }
    }
    
    converter.str.append_view(")");
}

func (converter : &mut JsConverter) convertJSXElement(element : *mut JsJSXElement) {
    const tagNameNode = element.opening.tagName as *mut JsNode
    var isComponent = false
    var tagName = std::string_view()

    if(tagNameNode.kind == JsNodeKind.Identifier) {
        const jsId = tagNameNode as *mut JsIdentifier
        tagName = jsId.value
        if(element.componentSignature != null || (tagName.size() > 0 && tagName.get(0) >= 'A' && tagName.get(0) <= 'Z')) {
            isComponent = true
        }
    } else if(tagNameNode.kind == JsNodeKind.JSXExpressionContainer) {
        isComponent = true
    }

    if(isComponent) {
        converter.convertJSXComponent(element, tagName, tagNameNode)
    } else {
        converter.convertJSXNativeElement(element, tagName)
    }
}

func (converter : &mut JsConverter) convertJSXFragment(fragment : *mut JsJSXFragment) {
    converter.str.append_view("[");
    
    for(var i : uint = 0; i < fragment.children.size(); i++) {
         if(i > 0) converter.str.append_view(", ");
         converter.convertJsNode(fragment.children.get(i));
    }
    
    converter.str.append_view("]");
}
