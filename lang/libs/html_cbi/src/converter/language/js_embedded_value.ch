
func (converter : &mut ASTConverter) emit_append_js_call(value : *mut Value, len : size_t) {
        unsafe {
        const builder = converter.builder
        const location = intrinsics::get_raw_location()
        const support = converter.support

        var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location)
        var id = builder.make_identifier(std::string_view("append_js_raw"), support.appendJsFn, false, location)
        const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
        var call = builder.make_function_call_node(chain, converter.parent, location)

        var args = call.get_args()
        args.push(value)
        args.push(builder.make_ubigint_value(len, location))

        converter.vec.push(call as *mut ASTNode)

        }
    }

func (converter : &mut ASTConverter) emit_append_js_from_str(s : &mut std::string) {
        unsafe {
        if (s.empty()) return;
        const builder = converter.builder
        const location = intrinsics::get_raw_location()
        const size = s.size()
        const value = builder.make_string_value(builder.allocate_view(s.view()), location)
        s.clear()
        converter.emit_append_js_call(value, size)

        }
    }

func (converter : &mut ASTConverter) make_js_value_call_with(value : *mut Value, fn_name : std::string_view, fnPtr : *mut ASTNode) : *mut FunctionCallNode {
        unsafe {
        const builder = converter.builder
        const location = intrinsics::get_raw_location();
        var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
        var id = builder.make_identifier(&fn_name, fnPtr, false, location);
        const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
        var call = builder.make_function_call_node(chain, converter.parent, location)
        var args = call.get_args();
        args.push(value)
        return call;

        }
    }

func (converter : &mut ASTConverter) make_js_char_ptr_value_call(value : *mut Value) : *mut FunctionCallNode {
        unsafe {
        return converter.make_js_value_call_with(value, std::string_view("append_js_char_ptr"), converter.support.appendJsCharPtrFn)

        }
    }

func (converter : &mut ASTConverter) make_js_char_value_call(value : *mut Value) : *mut FunctionCallNode {
        unsafe {
        return converter.make_js_value_call_with(value, std::string_view("append_js_char"), converter.support.appendJsCharFn)

        }
    }

func (converter : &mut ASTConverter) make_js_integer_value_call(value : *mut Value) : *mut FunctionCallNode {
        unsafe {
        return converter.make_js_value_call_with(value, std::string_view("append_js_integer"), converter.support.appendJsIntFn)

        }
    }

func (converter : &mut ASTConverter) make_js_uinteger_value_call(value : *mut Value) : *mut FunctionCallNode {
        unsafe {
        return converter.make_js_value_call_with(value, std::string_view("append_js_uinteger"), converter.support.appendJsUIntFn)

        }
    }

func (converter : &mut ASTConverter) make_js_float_value_call(value : *mut Value) : *mut FunctionCallNode {
        unsafe {
        return converter.make_js_value_call_with(value, std::string_view("append_js_float"), converter.support.appendJsFloatFn)

        }
    }

func (converter : &mut ASTConverter) make_js_double_value_call(value : *mut Value) : *mut FunctionCallNode {
        unsafe {
        return converter.make_js_value_call_with(value, std::string_view("append_js_double"), converter.support.appendJsDoubleFn)

        }
    }

// Report a value that cannot be serialized into the JavaScript bundle through
// the live codegen diagnoser (instead of writing an error string into the
// bundle at runtime, which silently produced broken JS).
func (converter : &mut ASTConverter) js_unsupported(value : *mut Value, detail : &std::string_view) {
        unsafe {
        if(converter.diagnoser == null) {
            // No diagnoser available (non-codegen caller): keep a visible fallback.
            converter.put_js_error(std::string_view("Unsupported value cant be written to js bundle"));
            return;
        }
        var msg = std::string("cannot embed this value in the JavaScript bundle: ")
        msg.append_view(detail)
        var loc = converter.fallback_loc
        if(loc == 0) { loc = intrinsics::get_raw_location() }
        converter.diagnoser.error(msg.to_view(), loc)

        }
    }

func (converter : &mut ASTConverter) put_js_error(val : &std::string_view) {
        unsafe {
        const value = converter.builder.make_string_value(converter.builder.allocate_view(val), intrinsics::get_raw_location())
        converter.vec.push(converter.make_js_char_ptr_value_call(value) as *mut ASTNode);

        }
    }

func (converter : &mut ASTConverter) put_by_node_js(type : *mut BaseType, node : *mut ASTNode, value : *mut Value) {
        unsafe {
        switch(node.getKind()) {
            ASTNodeKind.StructDecl, ASTNodeKind.UnionDecl, ASTNodeKind.VariantDecl => {
                var fnName = std::string_view("writeToPageJs")
                const writeFn = node.child(&fnName)
                if(writeFn == null) {
                    converter.js_unsupported(value, std::string_view("its type has no JS serialization (define writeToPageJs on the type, or pass a primitive/string)"));
                    return;
                }
                if(writeFn.getKind() != ASTNodeKind.FunctionDecl) {
                    converter.js_unsupported(value, std::string_view("'writeToPageJs' is not a function"));
                    return;
                }
                var bundleName = std::string_view("pageJs")
                const fn = writeFn as *mut FunctionDeclaration;
                const chain = converter.make_write_to_page_call(value, fnName, writeFn, &bundleName);
                converter.vec.push(chain)
            }
            ASTNodeKind.TypealiasStmt => {
                // const stmt = node as *mut TypealiasStatement
                // const actual_type = stmt.getActualType();
                // converter.put_by_type_js(actual_type, value);
                // TODO:
            }
            default => {
                converter.js_unsupported(value, std::string_view("this type has no JS serialization (use a supported primitive, or define writeToPageJs on the type)"))
            }
        }

        }
    }

// The struct a declaration type names, or "" when the type is not a struct/class.
// `std::string` and `std::string_view` are structs, so their names are how the
// aggregate path tells "a list of strings" from "a list of something else".
func (converter : &mut ASTConverter) declared_struct_name(type : *mut BaseType) : std::string_view {
        unsafe {
        if(type == null || type.getKind() != BaseTypeKind.Linked) { return std::string_view("") }
        const linked = type as *mut LinkedType
        const node = linked.getLinkedNode()
        if(node == null || node.getKind() != ASTNodeKind.StructDecl) { return std::string_view("") }
        return (node as *mut StructDefinition).getName()

        }
}

// Emits `page.<method>(&value)`: one statement that serializes the aggregate into
// the props payload at render time.
func (converter : &mut ASTConverter) emit_js_aggregate(value : *mut Value, method : std::string_view) {
        unsafe {
        const builder = converter.builder
        const location = intrinsics::get_raw_location()
        const fnPtr = converter.support.pageNode.child(&method)
        if(fnPtr == null) {
            converter.js_unsupported(value, std::string_view("this page library has no writer for aggregate props"))
            return
        }
        var pageIdent = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
        var id = builder.make_identifier(&method, fnPtr, false, location)
        const chain = builder.make_access_chain(&std::span<*mut Value>([ pageIdent, id ]), location)
        var call = builder.make_function_call_node(chain, converter.parent, location)
        const refValue = builder.make_reference_of_value(value, false, location)
        call.get_args().push(refValue as *mut Value)
        converter.vec.push(call as *mut ASTNode)

        }
}

// Aggregate props: a `vector<std::string>` (or `vector<std::string_view>`) prop is
// an aggregate the client can rebuild, so it is written as a JSON array of JS
// strings. Other aggregates keep the "no JS serialization" diagnostic: a prop the
// client cannot reconstruct is a build failure, not a silent `null`.
func (converter : &mut ASTConverter) put_string_vector_js(value : *mut Value, generic : *mut GenericType, node : *mut ASTNode) : bool {
        unsafe {
        if(node == null) { return false }
        // `vector` is an ordinary generic struct, but a declaration reached through
        // a generic instantiation can also arrive as a `GenericStructDecl`; both
        // spellings name the same declaration.
        var declName = std::string_view("")
        if(node.getKind() == ASTNodeKind.StructDecl) {
            declName = (node as *mut StructDefinition).getName()
        } else if(node.getKind() == ASTNodeKind.GenericStructDecl) {
            const master = (node as *mut GenericStructDecl).getMasterImpl()
            if(master == null) { return false }
            declName = master.getName()
        } else {
            return false
        }
        const vectorName = std::string_view("vector")
        if(!declName.equals(&vectorName)) { return false }
        if(generic.getArgumentCount() < 1) { return false }
        const element = generic.getArgumentType(0)
        const elementName = converter.declared_struct_name(element)
        const stringName = std::string_view("string")
        const viewName = std::string_view("string_view")
        if(elementName.equals(&stringName)) {
            converter.emit_js_aggregate(value, std::string_view("append_js_string_array"))
            return true
        }
        if(elementName.equals(&viewName)) {
            converter.emit_js_aggregate(value, std::string_view("append_js_string_view_array"))
            return true
        }
        return false

        }
}

func (converter : &mut ASTConverter) put_js_value_in(value : *mut Value) {
        unsafe {
        const type = value.getType();
        switch(type.getKind()) {
            BaseTypeKind.IntN => {
                const intN = type as *mut IntNType;
                const kind = intN.get_intn_type_kind();
                if(kind == IntNTypeKind.Char || kind == IntNTypeKind.UChar) {
                    converter.vec.push(converter.make_js_char_value_call(value) as *mut ASTNode);
                } else if(kind <= IntNTypeKind.Int128) {
                    converter.vec.push(converter.make_js_integer_value_call(value) as *mut ASTNode);
                } else {
                    converter.vec.push(converter.make_js_uinteger_value_call(value) as *mut ASTNode);
                }
            }
            BaseTypeKind.Float => {
                converter.vec.push(converter.make_js_float_value_call(value) as *mut ASTNode);
            }
            BaseTypeKind.Double => {
                converter.vec.push(converter.make_js_double_value_call(value) as *mut ASTNode);
            }
            BaseTypeKind.Bool => {
                converter.vec.push(converter.make_js_integer_value_call(value) as *mut ASTNode);
            }
            BaseTypeKind.String, BaseTypeKind.Pointer => {
                // Strings are embedded inside JS string literals at the call sites,
                // so they must be escaped for that context (quotes/backslashes/controls).
                var escapedName = std::string_view("append_js_escaped_char_ptr")
                var escapedFn = converter.support.appendJsEscapedCharPtrFn
                if(escapedFn == null) {
                    escapedName = std::string_view("append_js_char_ptr")
                    escapedFn = converter.support.appendJsCharPtrFn
                }
                converter.vec.push(converter.make_js_value_call_with(value, escapedName, escapedFn) as *mut ASTNode);
            }
            BaseTypeKind.Array => {
                // array literal — serialize as a JS array literal: ["One","Two"]
                if(value.getKind() == ValueKind.ArrayValue) {
                    const arrayValue = value as *mut ArrayValue
                    const values = arrayValue.get_values()
                    const size = values.size()

                    var open = std::string("[")
                    converter.emit_append_js_from_str(&mut open)
                    for(var i : uint = 0; i < size; i++) {
                        if(i > 0) {
                            var comma = std::string(",")
                            converter.emit_append_js_from_str(&mut comma)
                        }
                        const ptr = values.get(i)
                        const ptrType = ptr.getType()
                        const is_str = converter.is_string_type(ptrType)
                        if(is_str) {
                            var quote = std::string("\"")
                            converter.emit_append_js_from_str(&mut quote)
                            converter.put_js_value_in(ptr)
                            var quote2 = std::string("\"")
                            converter.emit_append_js_from_str(&mut quote2)
                        } else {
                            converter.put_js_value_in(ptr)
                        }
                    }
                    var close = std::string("]")
                    converter.emit_append_js_from_str(&mut close)
                } else {
                    converter.vec.push(converter.make_js_char_ptr_value_call(value) as *mut ASTNode);
                }
            }
            BaseTypeKind.Linked => {
                const linked = type as *mut LinkedType;
                const node = linked.getLinkedNode();
                converter.put_by_node_js(type, node, value);
            }
            BaseTypeKind.Generic => {
                const generic = type as *mut GenericType;
                const linked = generic.getLinkedType();
                const node = linked.getLinkedNode();
                if(converter.put_string_vector_js(value, generic, node)) { return }
                converter.put_by_node_js(type, node, value);
            }
            default => {
                converter.vec.push(converter.make_js_char_ptr_value_call(value) as *mut ASTNode);
            }
        }

        }
    }