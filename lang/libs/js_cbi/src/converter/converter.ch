
struct JsConverter {
    var builder : *mut ASTBuilder
    var support : *mut SymResSupport
    var vec : *mut VecRef<ASTNode>
    var parent : *mut ASTNode
    var str : std::string
}

func (converter : &mut JsConverter) make_value_call(value : *mut Value, len : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var name = std::string_view("append_js")
    
    var id = builder.make_identifier(name, converter.support.appendJsFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    args.push(builder.make_ubigint_value(len, location));
    return call;
}

func (converter : &mut JsConverter) put_chain_in() {
    if(converter.str.empty()) return;
    
    const location = intrinsics::get_raw_location();
    const builder = converter.builder;
    const value = builder.make_string_value(builder.allocate_view(converter.str.to_view()), location)
    const size = converter.str.size()
    converter.str.clear();
    
    const call = converter.make_value_call(value, size);
    converter.vec.push(call);
}

func (converter : &mut JsConverter) convertJsNode(node : *mut JsNode) {
    switch(node.kind) {
        JsNodeKind.VarDecl => {
            var varDecl = node as *mut JsVarDecl
            converter.str.append_view("var ")
            converter.str.append_view(varDecl.name)
            converter.str.append_view(" = ")
            converter.convertJsNode(varDecl.value)
            converter.str.append_view(";")
        }
        JsNodeKind.Literal => {
            var literal = node as *mut JsLiteral
            converter.str.append_view(literal.value)
        }
    }
}

func (converter : &mut JsConverter) convertJsRoot(root : *mut JsRoot) {
    var i = 0u
    while(i < root.statements.size()) {
        converter.convertJsNode(root.statements.get(i))
        i++
    }
    converter.put_chain_in()
}
