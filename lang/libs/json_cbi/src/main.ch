// ===== Parse Phase =====

@no_mangle
public func json_parseMacroTopLevelNode(parser : *mut Parser, builder : *mut ASTBuilder, spec : int) : *mut ASTNode {
    const location = parser.getEncodedLocation(parser.getToken())

    if(!parser.increment_if(ChemicalTokenType.LParen as int)) {
        parser.error(std::string_view("expected '(' after #json"))
        return null
    }

    // type path: `Type` or `ns::Type` / `a::b::Type` (namespaced types)
    var segments = std::vector<std::string_view>()
    if(parser.getToken().type == ChemicalTokenType.Identifier as int) {
        segments.push(builder.allocate_view(&parser.getToken().value))
        parser.increment()
    } else {
        parser.error(std::string_view("expected type name in #json"))
        return null
    }
    while(parser.increment_if(ChemicalTokenType.DoubleColonSym as int)) {
        if(parser.getToken().type != ChemicalTokenType.Identifier as int) {
            parser.error(std::string_view("expected identifier after '::'"))
            return null
        }
        segments.push(builder.allocate_view(&parser.getToken().value))
        parser.increment()
    }

    if(!parser.increment_if(ChemicalTokenType.RParen as int)) {
        parser.error(std::string_view("expected ')'"))
        return null
    }

    // join the segments into one "a::b::Type" path on the AST arena
    var total : size_t = 0
    for(var si = 0u; si < segments.size(); si++) {
        if(si > 0u) { total += 2 } // "::"
        total += segments.get(si).size()
    }
    var path_buf = builder.allocate_str_size(total + 1)
    var off = 0
    for(var sj = 0u; sj < segments.size(); sj++) {
        if(sj > 0u) {
            path_buf[off] = ':'
            path_buf[off + 1] = ':'
            off += 2
        }
        var seg = segments.get(sj)
        for(var c = 0u; c < seg.size(); c++) {
            path_buf[off] = seg.data()[c]
            off++
        }
    }
    path_buf[total] = '\0'
    var struct_name = std::string_view(path_buf, total as size_t)

    var info = builder.allocate<SerializableInfo>()
    new (info) SerializableInfo {
        struct_name : struct_name
    }

    const nodes_arr : []*mut ASTNode = []
    const values_arr : []*mut Value = []

    const node = builder.make_top_level_embedded_node(
        spec as AccessSpecifier,
        std::string_view("json"),
        info,
        known_type_fn,
        child_res_fn,
        cross_mod_sym_decl_proxy_fn,
        std::span<*mut ASTNode>(nodes_arr),
        std::span<*mut Value>(values_arr),
        parser.getParentNode(),
        location
    )

    return node
}

// ===== Declare Phase =====

@no_mangle
public func json_symResDeclareNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
}

// ===== Link Signature Phase =====

@no_mangle
public func json_symResLinkSigNode(visitor : *mut SymResLinkSignature, node : *mut EmbeddedNode) {
}

// ===== Link Body Phase =====

func resolve_types(resolver : *mut SymbolResolver, info : *mut SerializableInfo, loc : ubigint) : bool {
    var std_node = resolver.resolve(std::string_view("std"))
    if(std_node == null) { resolver.error(std::string_view("std not found"), loc); return false }

    info.serializer_node = std_node.child(&std::string_view("Serializer"))
    if(info.serializer_node == null) { resolver.error(std::string_view("std::Serializer not found"), loc); return false }

    info.deserializer_node = std_node.child(&std::string_view("Deserializer"))
    if(info.deserializer_node == null) { resolver.error(std::string_view("std::Deserializer not found"), loc); return false }

    info.result_node = std_node.child(&std::string_view("Result"))
    if(info.result_node == null) { resolver.error(std::string_view("std::Result not found"), loc); return false }

    info.result_ok_member = info.result_node.child(&std::string_view("Ok"))
    info.result_err_member = info.result_node.child(&std::string_view("Err"))
    if(info.result_ok_member == null || info.result_err_member == null) {
        resolver.error(std::string_view("std::Result.Ok/Err not found"), loc); return false
    }

    info.unit_node = std_node.child(&std::string_view("Unit"))
    if(info.unit_node == null) { resolver.error(std::string_view("std::Unit not found"), loc); return false }

    info.serialization_error_node = std_node.child(&std::string_view("SerializationError"))
    if(info.serialization_error_node == null) { resolver.error(std::string_view("std::SerializationError not found"), loc); return false }

    // non-generic SerializationError swap helper living in the json library; used
    // by the generated deserialize error returns instead of generic std::replace,
    // because a generic call from generated code can register a bogus
    // instantiation while the nested decode<T> instantiation is in flight
    info.se_repl_node = resolver.resolve(&std::string_view("__non_gen_se_repl"))
    if(info.se_repl_node == null) { resolver.error(std::string_view("__non_gen_se_repl not found"), loc); return false }

    // take_ok: moves an Ok payload out of a Result<T, SerializationError> (leaving
    // an Err state), giving generated deserialize an owned local it can move into
    // the struct literal - pattern-bound Ok values cannot be moved onward
    info.take_ok_node = resolver.resolve(&std::string_view("take_ok"))
    if(info.take_ok_node == null) { resolver.error(std::string_view("take_ok not found"), loc); return false }

    info.se_err_node = resolver.resolve(&std::string_view("se_err"))
    if(info.se_err_node == null) { resolver.error(std::string_view("se_err not found"), loc); return false }

    info.std_string_node = std_node.child(&std::string_view("string"))
    if(info.std_string_node == null) { resolver.error(std::string_view("std::string not found"), loc); return false }

    info.fnv1_hash_view_node = resolver.resolve(&std::string_view("fnv1_hash_view"))
    if(info.fnv1_hash_view_node == null) { resolver.error(std::string_view("fnv1_hash_view not found"), loc); return false }
    info.fnv1_hash_view_ret_type = (info.fnv1_hash_view_node as *mut FunctionDeclaration).getReturnType()

    var encoder_iface = std_node.child(&std::string_view("Encoder"))
    if(encoder_iface == null) { resolver.error(std::string_view("std::Encoder not found"), loc); return false }
    info.encoder_object_method = encoder_iface.child(&std::string_view("object"))
    if(info.encoder_object_method == null) { resolver.error(std::string_view("std::Encoder.object not found"), loc); return false }

    var object_encoder_iface = std_node.child(&std::string_view("ObjectEncoder"))
    if(object_encoder_iface == null) { resolver.error(std::string_view("std::ObjectEncoder not found"), loc); return false }
    info.object_encoder_field_method = object_encoder_iface.child(&std::string_view("field"))
    if(info.object_encoder_field_method == null) { resolver.error(std::string_view("std::ObjectEncoder.field not found"), loc); return false }

    info.json_decoder_node = resolver.resolve(&std::string_view("JsonDecoder"))
    if(info.json_decoder_node == null) { resolver.error(std::string_view("JsonDecoder not found"), loc); return false }

    info.json_object_decoder_node = resolver.resolve(&std::string_view("JsonObjectDecoder"))
    if(info.json_object_decoder_node == null) { resolver.error(std::string_view("JsonObjectDecoder not found"), loc); return false }

    info.decoder_decode_i64_method = info.json_decoder_node.child(&std::string_view("decode_i64"))
    info.decoder_decode_u64_method = info.json_decoder_node.child(&std::string_view("decode_u64"))
    info.decoder_decode_double_method = info.json_decoder_node.child(&std::string_view("decode_double"))
    info.decoder_decode_float_method = info.json_decoder_node.child(&std::string_view("decode_float"))
    info.decoder_decode_str_method = info.json_decoder_node.child(&std::string_view("decode_str"))
    info.decoder_decode_bool_method = info.json_decoder_node.child(&std::string_view("decode_bool"))
    info.decoder_decode_char_method = info.json_decoder_node.child(&std::string_view("decode_char"))
    info.decoder_object_method = info.json_decoder_node.child(&std::string_view("object"))
    info.decoder_decode_generic_method = info.json_decoder_node.child(&std::string_view("decode"))
    info.object_decoder_item_method = info.json_object_decoder_node.child(&std::string_view("item_decoder"))

    var pair_node = std_node.child(&std::string_view("pair"))
    if(pair_node == null) { resolver.error(std::string_view("std::pair not found"), loc); return false }
    info.pair_second_member = pair_node.child(&std::string_view("second"))
    if(info.pair_second_member == null) { resolver.error(std::string_view("std::pair.second not found"), loc); return false }

    info.json_value_node = resolver.resolve(&std::string_view("JsonValue"))
    if(info.json_value_node == null) { resolver.error(std::string_view("JsonValue not found"), loc); return false }

    info.json_encoder_node = resolver.resolve(&std::string_view("JsonEncoder"))
    if(info.json_encoder_node == null) { resolver.error(std::string_view("JsonEncoder not found"), loc); return false }

    info.type_decoder_node = resolver.resolve(&std::string_view("TypeDecoder"))
    if(info.type_decoder_node == null) { resolver.error(std::string_view("TypeDecoder not found"), loc); return false }

    return true
}

func find_decoder_method(info : *mut SerializableInfo, field_type : *mut BaseType) : *mut ASTNode {
    if(field_type == null) { return null }
    var kind = field_type.getKind()
    if(kind == BaseTypeKind.IntN) {
        var intn = field_type as *IntNType
        var intn_kind = intn.get_intn_type_kind()
        if(intn_kind == IntNTypeKind.Int || intn_kind == IntNTypeKind.Long ||
           intn_kind == IntNTypeKind.I64 || intn_kind == IntNTypeKind.I32 ||
           intn_kind == IntNTypeKind.Short || intn_kind == IntNTypeKind.I16 ||
           intn_kind == IntNTypeKind.I8) {
            return info.decoder_decode_i64_method
        }
        if(intn_kind == IntNTypeKind.UInt || intn_kind == IntNTypeKind.ULong ||
           intn_kind == IntNTypeKind.U64 || intn_kind == IntNTypeKind.U32 ||
           intn_kind == IntNTypeKind.UShort || intn_kind == IntNTypeKind.U16 ||
           intn_kind == IntNTypeKind.U8) {
            return info.decoder_decode_u64_method
        }
        if(intn_kind == IntNTypeKind.UChar || intn_kind == IntNTypeKind.Char) {
            // chars encode as single-char JSON strings (encode_char), so they
            // must decode through decode_char too - decode_u64 would demand a
            // number and reject the encoded form
            return info.decoder_decode_char_method
        }
    }
    if(kind == BaseTypeKind.Float) { return info.decoder_decode_float_method }
    if(kind == BaseTypeKind.Double) { return info.decoder_decode_double_method }
    if(kind == BaseTypeKind.Bool) { return info.decoder_decode_bool_method }
    // NOTE: std::string / std::string_view members are NOT decoded via decode_str
    // here: owning strings must be constructed (or taken) rather than bitcast from
    // a view, and string_view fields flow through the same generic decode<T>
    // machinery as struct fields. Both return null here on purpose.
    return null
}

// ===== Enum field support =====
// Enum fields serialize as a JSON string holding the member name (e.g.
// Color.Red -> "Red"). Encode resolves the name from the enum's backing int
// with an if/else chain (see build_serialize_fn); decode hashes the decoded
// name with std::fnv1_hash_view and dispatches over precomputed name hashes
// back to the enum value (erroring on unknown names), so lookup never compares
// strings linearly.

// returns the EnumDeclaration node a field type refers to (null otherwise)
func field_enum_node(field_type : *mut BaseType) : *mut ASTNode {
    if(field_type == null) { return null }
    if(field_type.getKind() != BaseTypeKind.Linked) { return null }
    var linked = (field_type as *mut LinkedType).getLinkedNode()
    if(linked == null) { return null }
    if(linked.getKind() == ASTNodeKind.EnumDecl) { return linked }
    return null
}

// returns a reason string_view for field types #json cannot serialize/
// deserialize (empty = supported). Called at the #json site so unsupported
// shapes produce a clean error there instead of corrupting the json library's
// own symres. Literal (static) views only - resolver.error must not receive a
// view into a stack-local string, and no per-field name is interpolated.
func unsupported_field_reason(mtype : *mut BaseType) : std::string_view {
    if(mtype == null) { return std::string_view("") }
    var k = mtype.getKind()
    if(k == BaseTypeKind.IntN || k == BaseTypeKind.Bool || k == BaseTypeKind.Float ||
       k == BaseTypeKind.Double || k == BaseTypeKind.String) {
        return std::string_view("")
    }
    if(k == BaseTypeKind.Generic) {
        return std::string_view("generic container field types (std::vector / std::ordered_map / Option) are not supported yet")
    }
    if(k == BaseTypeKind.Array) {
        return std::string_view("an array field type (not supported)")
    }
    if(k == BaseTypeKind.Pointer) {
        return std::string_view("a pointer field type (not supported)")
    }
    if(k == BaseTypeKind.Reference) {
        return std::string_view("a reference field type (not supported)")
    }
    if(k == BaseTypeKind.Union) {
        return std::string_view("a union field type (not supported)")
    }
    if(k == BaseTypeKind.Linked) {
        var linked = (mtype as *mut LinkedType).getLinkedNode()
        if(linked == null) { return std::string_view("") }
        var lk = linked.getKind()
        if(lk == ASTNodeKind.StructDecl || lk == ASTNodeKind.EnumDecl) {
            // structs/enums are handled (structs need their own #json/impl)
            return std::string_view("")
        }
        if(lk == ASTNodeKind.VariantDecl) {
            return std::string_view("a variant field type (#json variant support is not implemented yet)")
        }
        if(lk == ASTNodeKind.UnionDecl) {
            return std::string_view("a union field type (not supported)")
        }
        return std::string_view("")
    }
    if(k == BaseTypeKind.Dynamic || k == BaseTypeKind.Function || k == BaseTypeKind.CapturingFunction ||
       k == BaseTypeKind.Void || k == BaseTypeKind.Complex || k == BaseTypeKind.LongDouble) {
        return std::string_view("a field type with no Serializer/Deserializer implementation")
    }
    // unknown kinds fall through to the generic decode<T> path (status quo);
    // only shapes known to lack a Serializer/Deserializer are rejected above
    return std::string_view("")
}

// fnv1a-64 (same algorithm as std::fnv1_hash_view) computed in the plugin so
// generated decode dispatch can compare against constant name hashes
func fnv1a64(name : std::string_view) : u64 {
    var hash : u64 = 0xcbf29ce484222325 as u64
    for(var i = 0u; i < name.size(); i++) {
        hash = hash ^ (name.data()[i] as u64)
        hash = hash * (0x100000001b3 as u64)
    }
    return hash
}

// resolve a (possibly namespaced) type path "Type" / "ns::Type" / "a::b::Type":
// the first segment is resolved through the symbol table (a namespace resolves
// to its root scope), remaining segments via .child() walking. Returns null if
// any segment fails to resolve.
func resolve_type_path(resolver : *mut SymbolResolver, path : &std::string_view) : *mut ASTNode {
    var seg_start : size_t = 0
    var first = true
    var node : *mut ASTNode = null
    loop {
        var sep = path.size()
        for(var i = seg_start; i < path.size(); i++) {
            if(path.data()[i] == ':' && i + 1 < path.size() && path.data()[i + 1] == ':') {
                sep = i
                break
            }
        }
        var seg = std::string_view(path.data() + seg_start, sep - seg_start)
        if(first) {
            node = resolver.resolve(&seg)
            first = false
        } else {
            if(node == null) { return null }
            node = node.child(&seg)
        }
        if(sep == path.size()) { break }
        seg_start = sep + 2
    }
    return node
}

// ===== Serialize function generation =====
// func serialize(&self, encoder : &JsonEncoder) : Result<Unit, SerializationError> {
//     var obj = encoder.object()
//     obj.field<T0>("x0", &self.x0)
//     obj.field<T1>("x1", &self.x1)
// }
//
// ObjectEncoder.field takes its value BY REFERENCE (std::ObjectEncoder.field
// signature is `value : &V`), so owning/destructible members are encoded without
// moving them out of &self or copying them.

func build_serialize_fn(builder : *mut ASTBuilder, info : *mut SerializableInfo, members : *mut VecRef<BaseDefMember>, parent_node : *mut ASTNode, loc : ubigint) : *mut FunctionDeclaration {
    var struct_type = builder.make_linked_type(&info.struct_name, info.struct_node, loc)
    // parent must be the impl def (contained function) so the mangler emits the
    // impl-context name that vtable generation expects, like parsed impl code
    var serialize_fn = builder.make_function(std::string_view("serialize"), info.ser_result_type as *mut BaseType, false, parent_node, loc)
    // parsed impl-contained functions are Public (see parseContainerMembersInto)
    serialize_fn.setAccessSpecifier(AccessSpecifier.Public)
    info.serialize_fn = serialize_fn

    var self_ref_type = builder.make_reference_type(struct_type as *mut BaseType, false, loc)
    var self_param = builder.make_function_param(std::string_view("self"), self_ref_type as *mut BaseType, 0, null, true, serialize_fn, loc)
    serialize_fn.get_params().push(self_param)

    var encoder_linked = builder.make_linked_type(&std::string_view("JsonEncoder"), info.json_encoder_node, loc)
    var encoder_ref_type = builder.make_reference_type(encoder_linked as *mut BaseType, false, loc)
    var encoder_param = builder.make_function_param(std::string_view("encoder"), encoder_ref_type as *mut BaseType, 1, null, false, serialize_fn, loc)
    serialize_fn.get_params().push(encoder_param)

    var body = serialize_fn.add_body()

    // var obj = encoder.object()
    var encoder_id = builder.make_identifier(&std::string_view("encoder"), encoder_param as *mut ASTNode, false, loc)
    var object_method_id = builder.make_identifier(&std::string_view("object"), info.encoder_object_method, false, loc)
    var object_chain = builder.make_access_chain(&std::span<*mut Value>([ encoder_id as *mut Value, object_method_id as *mut Value ]), loc)
    var object_call = builder.make_function_call_value(object_chain as *mut Value, loc)
    var obj_name = builder.allocate_view(&std::string_view("obj"))
    var obj_var = builder.make_varinit_stmt(
        false, false, &obj_name, null, object_call as *mut Value,
        AccessSpecifier.Internal, serialize_fn as *mut ASTNode, loc
    )
    body.push(obj_var as *mut ASTNode)

    // obj.field<T_i>("name_i", &self.name_i) for each field. Enum fields first
    // resolve their member NAME to a std::string (if/else chain over the backing
    // int) and emit obj.field<std::string>("name", &__en<i>); scalar and
    // composite fields emit obj.field<T>("name", &self.name) directly.
    var str_linked_type : *mut LinkedType = null
    for(var i = 0u; i < info.field_names.size(); i++) {
        var fname = info.field_names.get(i)
        var ftype_i = info.field_types.get(i)
        var enum_node = field_enum_node(ftype_i)

        // explicit generic arg: the field's concrete type (std::string for enums)
        var field_generic_type : *mut BaseType = ftype_i
        // arg 2 value: &self.field_name for scalar/composite members
        // (ObjectEncoder.field takes V BY REFERENCE, so passing the member itself
        // would try to MOVE it out of &self - illegal for destructible members)
        var field_arg : *mut Value = null

        if(enum_node != null) {
            // var __en<i> : std::string = std::string()
            var en_name = alloc_indexed_view(builder, "__en%d", i)
            str_linked_type = builder.make_linked_type(&std::string_view("string"), info.std_string_node, loc)
            field_generic_type = str_linked_type as *mut BaseType
            var empty_ctor = builder.make_function_call_value(
                builder.make_identifier(&std::string_view("string"), info.std_string_node, false, loc) as *mut Value, loc
            )
            var en_var = builder.make_varinit_stmt(
                false, false, &en_name, field_generic_type, empty_ctor as *mut Value,
                AccessSpecifier.Internal, serialize_fn as *mut ASTNode, loc
            )
            body.push(en_var as *mut ASTNode)

            // resolve the member name for each enum value via the backing int:
            //   if((self.field as int) == 0) { __en<i> = std::string("Red") }
            //   else if(...) { ... }
            //   else { __en<i> = std::string("") }
            var enum_members = (enum_node as *mut EnumDeclaration).getMembers()
            var first_if : *mut IfStatement = null
            for(var j = 0u; j < enum_members.size(); j++) {
                var mem_name = enum_members.get(j).getName()
                var self_id = builder.make_identifier(&std::string_view("self"), self_param as *mut ASTNode, false, loc)
                var member_id = builder.make_identifier(&fname, members.get(i) as *mut ASTNode, false, loc)
                var member_chain = builder.make_access_chain(&std::span<*mut Value>([ self_id as *mut Value, member_id as *mut Value ]), loc)
                var cond = builder.make_expression_value(
                    builder.make_casted_value(member_chain as *mut Value, builder.get_int_type(), loc) as *mut Value,
                    builder.make_int_value(j as int, loc) as *mut Value,
                    Operation.IsEqual, builder.make_bool_type(), loc
                )
                var mem_name_alloc = builder.allocate_view(&mem_name)
                var name_ctor = builder.make_function_call_value(
                    builder.make_identifier(&std::string_view("string"), info.std_string_node, false, loc) as *mut Value, loc
                )
                name_ctor.get_args().push(builder.make_string_value(&mem_name_alloc, loc))
                var assign = builder.make_assignment_stmt(
                    builder.make_identifier(&en_name, en_var as *mut ASTNode, false, loc) as *mut Value,
                    name_ctor as *mut Value, Operation.Assignment, serialize_fn as *mut ASTNode, loc
                )
                if(j == 0u) {
                    first_if = builder.make_if_stmt(cond as *mut Value, serialize_fn as *mut ASTNode, loc)
                    first_if.get_body().push(assign as *mut ASTNode)
                } else {
                    var else_if_body = first_if.add_else_if(cond as *mut Value)
                    else_if_body.push(assign as *mut ASTNode)
                }
            }
            // trailing else: unreachable for a valid enum, keeps every path assigned
            var empty_name_alloc = builder.allocate_view(&std::string_view(""))
            var empty_ctor2 = builder.make_function_call_value(
                builder.make_identifier(&std::string_view("string"), info.std_string_node, false, loc) as *mut Value, loc
            )
            empty_ctor2.get_args().push(builder.make_string_value(&empty_name_alloc, loc))
            var else_assign = builder.make_assignment_stmt(
                builder.make_identifier(&en_name, en_var as *mut ASTNode, false, loc) as *mut Value,
                empty_ctor2 as *mut Value, Operation.Assignment, serialize_fn as *mut ASTNode, loc
            )
            first_if.add_else_body().push(else_assign as *mut ASTNode)
            body.push(first_if as *mut ASTNode)

            // arg 2 becomes &__en<i> (the computed name), not &self.field
            field_arg = builder.make_reference_of_value(
                builder.make_identifier(&en_name, en_var as *mut ASTNode, false, loc) as *mut Value, false, loc
            )
        } else {
            var self_id = builder.make_identifier(&std::string_view("self"), self_param as *mut ASTNode, false, loc)
            var member_id = builder.make_identifier(&fname, members.get(i) as *mut ASTNode, false, loc)
            var member_chain = builder.make_access_chain(&std::span<*mut Value>([ self_id as *mut Value, member_id as *mut Value ]), loc)
            field_arg = builder.make_reference_of_value(member_chain as *mut Value, false, loc)
        }

        var obj_id = builder.make_identifier(&std::string_view("obj"), obj_var as *mut ASTNode, false, loc)
        var field_method_id = builder.make_identifier(&std::string_view("field"), info.object_encoder_field_method, false, loc)
        var field_chain = builder.make_access_chain(&std::span<*mut Value>([ obj_id as *mut Value, field_method_id as *mut Value ]), loc)

        // Build the statement node from the method CHAIN — mirrors the parser's
        // bare-call statement (AccessChainNode whose single chain value is the
        // FunctionCall). IMPORTANT: never pass a pre-built FunctionCall here,
        // make_function_call_node would wrap it in a SECOND call whose parent is
        // that value (i.e. calling the first call's result as a function).
        var field_stmt = builder.make_function_call_node(field_chain as *mut Value, serialize_fn as *mut ASTNode, loc)
        field_stmt.add_generic_arg(field_generic_type, loc)

        // arg 1: field name string literal
        var fname_alloc = builder.allocate_view(&fname)
        var fname_val = builder.make_string_value(&fname_alloc, loc)
        field_stmt.get_args().push(fname_val)

        field_stmt.get_args().push(field_arg)

        body.push(field_stmt as *mut ASTNode)
    }

    // return std::Result.Ok<Unit, SerializationError>(std::Unit())
    var unit_call = builder.make_function_call_value(
        builder.make_identifier(&std::string_view("Unit"), info.unit_node, false, loc) as *mut Value, loc
    )
    var ser_ok_call = builder.make_function_call_value(
        builder.make_identifier(&std::string_view("Ok"), info.result_ok_member, false, loc) as *mut Value, loc
    )
    ser_ok_call.add_generic_arg(info.ser_result_type.getArgumentType(0), loc)
    ser_ok_call.add_generic_arg(info.ser_result_type.getArgumentType(1), loc)
    ser_ok_call.get_args().push(unit_call as *mut Value)
    var ser_ok_return = builder.make_return_stmt(ser_ok_call as *mut Value, serialize_fn as *mut ASTNode, loc)
    body.push(ser_ok_return as *mut ASTNode)

    return serialize_fn
}

// ===== Deserialize function generation =====
// func deserialize(&self) : Result<Point, SerializationError> {
//     var dr = self.decoder.object()                    // Result<JsonObjectDecoder, SE>
//     if(dr is std::Result.Err) { ... return Err<Point, SerializationError>(...) }
//     var Ok(obj) = dr else unreachable
//     // per field: consume one key/value pair and decode the value
//     var q0 = obj.item_decoder()                       // scalar fields
//     ...
//     var Ok(p0) = q0 else unreachable
//     var v0 = p0.second.decode_i64()
//     ...
//     var Ok(x) = v0 else unreachable                   // scalar: Ok-pattern bind
//
//     var q1 = obj.item_decoder()                       // composite fields
//     ...
//     var v1 = p1.second.decode<T>()                    // strings + nested structs
//     ...
//     var f1 : T = take_ok(&mut v1)                     // move payload out of the
//                                                       // Result (v1 -> Err state)
//     ...
//     return std::Result.Ok<Point, SerializationError>(Point { x: x, child: f1 })
// }
//
// Composite values are decoded via decode<T> and moved out of their Result with
// json's take_ok helper (like std::Option.take): a pattern-bound Ok payload
// cannot be moved onward into the aggregate literal ("cannot move this value
// without re-initializing memory"), but take_ok leaves an OWNED local that can
// be moved - required for destructible/owning members (strings, nested structs)
// and avoids copies entirely. The final literal only casts scalar values; a
// CastedValue around a destructible value would bitwise-copy it and still
// destruct the local at scope end (double free).

// allocates a "__name<idx>" identifier view on the AST arena
func alloc_indexed_view(builder : *mut ASTBuilder, fmt : *char, idx : uint) : std::string_view {
    var buf = builder.allocate_str_size(24)
    var len = snprintf(buf, 24, fmt, idx as int)
    buf[len] = '\0'
    return std::string_view(buf, len as size_t)
}

// appends to `body`:
//   if(<rname> is std::Result.Err) {
//       var Err(__e) = <rname> else unreachable
//       return std::Result.Err<T, SerializationError>(__non_gen_se_repl(&mut __e, SerializationError()))
//   }
// this mirrors the negative-check style of the handwritten json.ch impls
func append_err_check(builder : *mut ASTBuilder, info : *mut SerializableInfo, body : *mut VecRef<ASTNode>, parent_fn : *mut ASTNode, rname : std::string_view, rnode : *mut ASTNode, loc : ubigint) {
    var err_linked = builder.make_linked_type(&std::string_view("Err"), info.result_err_member, loc)
    var is_err = builder.make_is_value(
        builder.make_identifier(&rname, rnode, false, loc) as *mut Value,
        err_linked as *mut BaseType, false, loc
    )
    var err_if = builder.make_if_stmt(is_err as *mut Value, parent_fn, loc)
    body.push(err_if as *mut ASTNode)
    var err_body = err_if.get_body()

    // var Err(__e) = <rname> else unreachable
    var e_name = builder.allocate_view(&std::string_view("__e"))
    var err_pm = builder.make_pattern_match_expr(false, false, &std::string_view("Err"), loc)
    err_pm.set_expression(builder.make_identifier(&rname, rnode, false, loc) as *mut Value)
    err_pm.set_else_unreachable()
    var err_pm_id = err_pm.add_param_name(builder, &e_name, loc)
    err_body.push(builder.make_pattern_match_node(err_pm, parent_fn, loc) as *mut ASTNode)

    // return std::Result.Err<T, SerializationError>(std::replace(&mut e, SerializationError()))
    var e_id = builder.make_identifier(&e_name, err_pm_id as *mut ASTNode, false, loc)
    // &mut (reference of value), matching the parsed `std::replace(&mut e, ...)`
    // in the handwritten impls — an AddrOfValue here would take the address of
    // the destructured error through an extra temp during call codegen
    var addr_e = builder.make_reference_of_value(e_id as *mut Value, true, loc)
    var se_repl_call = builder.make_function_call_value(
        builder.make_identifier(&std::string_view("__non_gen_se_repl"), info.se_repl_node, false, loc) as *mut Value, loc
    )
    var se_call = builder.make_function_call_value(
        builder.make_identifier(&std::string_view("SerializationError"), info.serialization_error_node, false, loc) as *mut Value, loc
    )
    se_repl_call.get_args().push(addr_e as *mut Value)
    se_repl_call.get_args().push(se_call as *mut Value)
    var err_call = builder.make_function_call_value(
        builder.make_identifier(&std::string_view("Err"), info.result_err_member, false, loc) as *mut Value, loc
    )
    err_call.add_generic_arg(info.result_type.getArgumentType(0), loc)
    err_call.add_generic_arg(info.result_type.getArgumentType(1), loc)
    err_call.get_args().push(se_repl_call as *mut Value)
    err_body.push(builder.make_return_stmt(err_call as *mut Value, parent_fn, loc) as *mut ASTNode)
}

func build_deserialize_fn(builder : *mut ASTBuilder, info : *mut SerializableInfo, members : *mut VecRef<BaseDefMember>, parent_node : *mut ASTNode, loc : ubigint) : *mut FunctionDeclaration {
    var struct_type = builder.make_linked_type(&info.struct_name, info.struct_node, loc)
    // parent must be the impl def (contained function) so the mangler emits the
    // impl-context name that vtable generation expects, like parsed impl code
    var deserialize_fn = builder.make_function(std::string_view("deserialize"), info.result_type as *mut BaseType, false, parent_node, loc)
    // parsed impl-contained functions are Public (see parseContainerMembersInto)
    deserialize_fn.setAccessSpecifier(AccessSpecifier.Public)
    info.deserialize_fn = deserialize_fn

    // self : &TypeDecoder<Point>
    var self_ref_type = builder.make_reference_type(info.type_decoder_type as *mut BaseType, false, loc)
    var self_param = builder.make_function_param(std::string_view("self"), self_ref_type as *mut BaseType, 0, null, true, deserialize_fn, loc)
    deserialize_fn.get_params().push(self_param)

    var body = deserialize_fn.add_body()

    // the .decoder member of TypeDecoder<T> (type &JsonDecoder)
    var decoder_member = info.type_decoder_node.child(&std::string_view("decoder"))

    // chain identifiers, one set shared by all statements (pre-links are a hint;
    // symres relinks access-chain members against the concrete receiver types)
    var self_id = builder.make_identifier(&std::string_view("self"), self_param as *mut ASTNode, false, loc)
    var decoder_member_id = builder.make_identifier(&std::string_view("decoder"), decoder_member, false, loc)
    var object_method_id = builder.make_identifier(&std::string_view("object"), info.decoder_object_method, false, loc)

    // var __dr = self.decoder.object()
    var dr_name = builder.allocate_view(&std::string_view("__dr"))
    var obj_chain = builder.make_access_chain(&std::span<*mut Value>([ self_id as *mut Value, decoder_member_id as *mut Value, object_method_id as *mut Value ]), loc)
    var obj_call = builder.make_function_call_value(obj_chain as *mut Value, loc)
    var dr_var = builder.make_varinit_stmt(
        false, false, &dr_name, null, obj_call as *mut Value,
        AccessSpecifier.Internal, deserialize_fn as *mut ASTNode, loc
    )
    body.push(dr_var as *mut ASTNode)

    // if(__dr is std::Result.Err) { var Err(__e) = __dr else unreachable; return Err<...>(...) }
    append_err_check(builder, info, body, deserialize_fn as *mut ASTNode, dr_name, dr_var as *mut ASTNode, loc)

    // var Ok(__obj) = __dr else unreachable
    var obj_name = builder.allocate_view(&std::string_view("__obj"))
    var dr_ok_pm = builder.make_pattern_match_expr(false, false, &std::string_view("Ok"), loc)
    dr_ok_pm.set_expression(builder.make_identifier(&dr_name, dr_var as *mut ASTNode, false, loc) as *mut Value)
    dr_ok_pm.set_else_unreachable()
    var obj_pm_id = dr_ok_pm.add_param_name(builder, &obj_name, loc)
    body.push(builder.make_pattern_match_node(dr_ok_pm, deserialize_fn as *mut ASTNode, loc) as *mut ASTNode)

    var item_decoder_method_id = builder.make_identifier(&std::string_view("item_decoder"), info.object_decoder_item_method, false, loc)
    var second_id = builder.make_identifier(&std::string_view("second"), info.pair_second_member, false, loc)

    // per-field value holders + their names, for the final struct construction.
    // Scalar fields bind the Ok payload via a pattern match (PatternMatchId);
    // composite/destructible fields move the Ok payload out with take_ok into a
    // plain local var (VarInitStatement). Both entries are the AST node the final
    // struct-init identifier links to.
    var field_val_names = std::vector<std::string_view>()
    var field_val_nodes = std::vector<*mut ASTNode>()
    // scalar fields are wrapped in a CastedValue in the struct init (decode
    // produces i64/u64/... but the member needs a cast to its exact type);
    // composite/string fields are already the exact member type and must be
    // passed as plain identifiers so the value MOVES into the literal (a cast
    // would bitwise-copy and leave the local destructed later -> double free)
    var field_val_is_cast = std::vector<bool>()

    for(var i = 0u; i < info.field_names.size(); i++) {
        var field_type = info.field_types.get(i)
        var enum_node = field_enum_node(field_type)
        var is_enum = enum_node != null
        var decode_method = find_decoder_method(info, field_type)
        var decode_is_generic = false
        if(decode_method == null) {
            if(is_enum) {
                // enum fields decode their member NAME through decode_str, then
                // dispatch on the name hash (handled after the __v<i> err check)
                decode_method = info.decoder_decode_str_method
            } else {
                // nested struct / composite field: decode via the generic JsonDecoder.decode<T>()
                decode_method = info.decoder_decode_generic_method
                if(decode_method == null) {
                    // unsupported field type: leave its slot undecoded
                    continue
                }
                decode_is_generic = true
            }
        }

        var q_name = alloc_indexed_view(builder, "__q%d", i)
        var p_name = alloc_indexed_view(builder, "__p%d", i)
        var v_name = alloc_indexed_view(builder, "__v%d", i)
        var f_name = alloc_indexed_view(builder, "__f%d", i)

        // var __q<i> = __obj.item_decoder()
        var obj_id = builder.make_identifier(&obj_name, obj_pm_id as *mut ASTNode, false, loc)
        var q_chain = builder.make_access_chain(&std::span<*mut Value>([ obj_id as *mut Value, item_decoder_method_id as *mut Value ]), loc)
        var q_call = builder.make_function_call_value(q_chain as *mut Value, loc)
        var q_var = builder.make_varinit_stmt(
            false, false, &q_name, null, q_call as *mut Value,
            AccessSpecifier.Internal, deserialize_fn as *mut ASTNode, loc
        )
        body.push(q_var as *mut ASTNode)
        append_err_check(builder, info, body, deserialize_fn as *mut ASTNode, q_name, q_var as *mut ASTNode, loc)

        // var Ok(__p<i>) = __q<i> else unreachable
        var q_ok_pm = builder.make_pattern_match_expr(false, false, &std::string_view("Ok"), loc)
        q_ok_pm.set_expression(builder.make_identifier(&q_name, q_var as *mut ASTNode, false, loc) as *mut Value)
        q_ok_pm.set_else_unreachable()
        var p_pm_id = q_ok_pm.add_param_name(builder, &p_name, loc)
        body.push(builder.make_pattern_match_node(q_ok_pm, deserialize_fn as *mut ASTNode, loc) as *mut ASTNode)

        // var __v<i> = __p<i>.second.decode_XXX()    (or .decode<T>() for composites)
        var decode_method_id : *mut VariableIdentifier
        if(decode_is_generic) {
            decode_method_id = builder.make_identifier(&std::string_view("decode"), decode_method, false, loc)
        } else {
            var decode_fn = decode_method as *FunctionDeclaration
            decode_method_id = builder.make_identifier(&decode_fn.getName(), decode_method, false, loc)
        }
        var p_id = builder.make_identifier(&p_name, p_pm_id as *mut ASTNode, false, loc)
        var v_chain = builder.make_access_chain(&std::span<*mut Value>([ p_id as *mut Value, second_id as *mut Value, decode_method_id as *mut Value ]), loc)
        var v_call = builder.make_function_call_value(v_chain as *mut Value, loc)
        if(decode_is_generic) {
            // explicit generic arg: the concrete field type T
            v_call.add_generic_arg(field_type, loc)
        }
        // struct-typed (decode<T>) fields get an explicit concrete result type so
        // the var + its pattern-match identifiers are never left at the generic
        // master Result type while the decode<T> instantiation finalizes
        var v_var_type : *mut BaseType = null
        if(decode_is_generic) {
            v_var_type = info.field_result_types.get(i) as *mut BaseType
        }
        var v_var = builder.make_varinit_stmt(
            false, false, &v_name, v_var_type, v_call as *mut Value,
            AccessSpecifier.Internal, deserialize_fn as *mut ASTNode, loc
        )
        body.push(v_var as *mut ASTNode)
        append_err_check(builder, info, body, deserialize_fn as *mut ASTNode, v_name, v_var as *mut ASTNode, loc)

        if(is_enum) {
            // Enum field: __v<i> holds Result<string_view, SE> (decode_str). Bind
            // the name, hash it and dispatch over the precomputed name hashes:
            //
            //   var Ok(__s<i>) = __v<i> else unreachable
            //   const __h<i> = std::fnv1_hash_view(&__s<i>)
            //   var __f<i> : EnumType                      // uninitialized
            //   if(__h<i> == <hash "Red">) { __f<i> = <0 as EnumType> }
            //   else if(__h<i> == <hash "Green">) { ... }
            //   else { return Err<Struct, SE>(se_err(...)) }  // unknown name

            // var Ok(__s<i>) = __v<i> else unreachable
            var s_name = alloc_indexed_view(builder, "__s%d", i)
            var v_ok_pm = builder.make_pattern_match_expr(false, false, &std::string_view("Ok"), loc)
            v_ok_pm.set_expression(builder.make_identifier(&v_name, v_var as *mut ASTNode, false, loc) as *mut Value)
            v_ok_pm.set_else_unreachable()
            var s_pm_id = v_ok_pm.add_param_name(builder, &s_name, loc)
            body.push(builder.make_pattern_match_node(v_ok_pm, deserialize_fn as *mut ASTNode, loc) as *mut ASTNode)

            // const __h<i> = std::fnv1_hash_view(&__s<i>)
            var h_name = alloc_indexed_view(builder, "__h%d", i)
            var s_id = builder.make_identifier(&s_name, s_pm_id as *mut ASTNode, false, loc)
            var s_ref = builder.make_reference_of_value(s_id as *mut Value, false, loc)
            var hash_call = builder.make_function_call_value(
                builder.make_identifier(&std::string_view("fnv1_hash_view"), info.fnv1_hash_view_node, false, loc) as *mut Value, loc
            )
            hash_call.get_args().push(s_ref as *mut Value)
            var h_var = builder.make_varinit_stmt(
                true, false, &h_name, info.fnv1_hash_view_ret_type, hash_call as *mut Value,
                AccessSpecifier.Internal, deserialize_fn as *mut ASTNode, loc
            )
            body.push(h_var as *mut ASTNode)

            // var __f<i> : EnumType (uninitialized; every non-error path assigns)
            var f_var = builder.make_varinit_stmt(
                false, false, &f_name, field_type, null,
                AccessSpecifier.Internal, deserialize_fn as *mut ASTNode, loc
            )
            body.push(f_var as *mut ASTNode)

            // if/else-if dispatch on the name hash; final else returns an error
            var enum_members = (enum_node as *mut EnumDeclaration).getMembers()
            var first_if : *mut IfStatement = null
            for(var j = 0u; j < enum_members.size(); j++) {
                var mem_name = enum_members.get(j).getName()
                var mem_hash_lit = builder.make_casted_value(
                    builder.make_ubigint_value(fnv1a64(mem_name) as ubigint, loc) as *mut Value,
                    info.fnv1_hash_view_ret_type, loc
                )
                var cond = builder.make_expression_value(
                    builder.make_identifier(&h_name, h_var as *mut ASTNode, false, loc) as *mut Value,
                    mem_hash_lit as *mut Value,
                    Operation.IsEqual, builder.make_bool_type(), loc
                )
                // __f<i> = <j as EnumType>
                var enum_val = builder.make_casted_value(
                    builder.make_int_value(j as int, loc) as *mut Value,
                    field_type, loc
                )
                var assign = builder.make_assignment_stmt(
                    builder.make_identifier(&f_name, f_var as *mut ASTNode, false, loc) as *mut Value,
                    enum_val as *mut Value, Operation.Assignment, deserialize_fn as *mut ASTNode, loc
                )
                if(j == 0u) {
                    first_if = builder.make_if_stmt(cond as *mut Value, deserialize_fn as *mut ASTNode, loc)
                    first_if.get_body().push(assign as *mut ASTNode)
                } else {
                    var else_if_body = first_if.add_else_if(cond as *mut Value)
                    else_if_body.push(assign as *mut ASTNode)
                }
            }

            // unknown member name: return Err<Struct, SE>(se_err("..."))
            var unknown_alloc = builder.allocate_view(&std::string_view("unknown enum member name"))
            var se_err_call = builder.make_function_call_value(
                builder.make_identifier(&std::string_view("se_err"), info.se_err_node, false, loc) as *mut Value, loc
            )
            se_err_call.get_args().push(builder.make_string_value(&unknown_alloc, loc))
            var err_call = builder.make_function_call_value(
                builder.make_identifier(&std::string_view("Err"), info.result_err_member, false, loc) as *mut Value, loc
            )
            err_call.add_generic_arg(info.result_type.getArgumentType(0), loc)
            err_call.add_generic_arg(info.result_type.getArgumentType(1), loc)
            err_call.get_args().push(se_err_call as *mut Value)
            first_if.add_else_body().push(builder.make_return_stmt(err_call as *mut Value, deserialize_fn as *mut ASTNode, loc) as *mut ASTNode)
            body.push(first_if as *mut ASTNode)

            field_val_names.push(f_name)
            field_val_nodes.push(f_var as *mut ASTNode)
            field_val_is_cast.push(false)
        } else if(decode_is_generic) {
            // Composite/destructible field: move the Ok payload out of __v<i> with
            // take_ok (leaves __v<i> in an Err state). Pattern-bound Ok values
            // cannot be moved onward into the struct literal ("cannot move this
            // value without re-initializing memory"), but an owned local CAN.
            // var __f<i> = take_ok<FieldType>(&mut __v<i>)
            var v_id = builder.make_identifier(&v_name, v_var as *mut ASTNode, false, loc)
            var addr_v = builder.make_reference_of_value(v_id as *mut Value, true, loc)
            var take_ok_call = builder.make_function_call_value(
                builder.make_identifier(&std::string_view("take_ok"), info.take_ok_node, false, loc) as *mut Value, loc
            )
            take_ok_call.add_generic_arg(field_type, loc)
            take_ok_call.get_args().push(addr_v as *mut Value)
            var f_var = builder.make_varinit_stmt(
                false, false, &f_name, field_type, take_ok_call as *mut Value,
                AccessSpecifier.Internal, deserialize_fn as *mut ASTNode, loc
            )
            body.push(f_var as *mut ASTNode)
            field_val_names.push(f_name)
            field_val_nodes.push(f_var as *mut ASTNode)
            field_val_is_cast.push(false)
        } else {
            // Scalar field: bind the Ok payload (trivially copyable, no move
            // restrictions). var Ok(__f<i>) = __v<i> else unreachable
            var v_ok_pm = builder.make_pattern_match_expr(false, false, &std::string_view("Ok"), loc)
            v_ok_pm.set_expression(builder.make_identifier(&v_name, v_var as *mut ASTNode, false, loc) as *mut Value)
            v_ok_pm.set_else_unreachable()
            var f_pm_id = v_ok_pm.add_param_name(builder, &f_name, loc)
            body.push(builder.make_pattern_match_node(v_ok_pm, deserialize_fn as *mut ASTNode, loc) as *mut ASTNode)

            field_val_names.push(f_name)
            field_val_nodes.push(f_pm_id as *mut ASTNode)
            field_val_is_cast.push(true)
        }
    }

    // return std::Result.Ok<Point, SerializationError>(Point { x : __f0, y : __f1 })
    var struct_val = builder.make_struct_value(info.struct_node, loc)
    for(var j = 0u; j < field_val_nodes.size(); j++) {
        var fname_j = info.field_names.get(j)
        var ftype_j = info.field_types.get(j)
        var fval_id = builder.make_identifier(&field_val_names.get(j), field_val_nodes.get(j), false, loc)
        if(field_val_is_cast.get(j)) {
            var casted = builder.make_casted_value(fval_id as *mut Value, ftype_j, loc)
            struct_val.add_value(&fname_j, casted as *mut Value)
        } else {
            struct_val.add_value(&fname_j, fval_id as *mut Value)
        }
    }
    var ok_call = builder.make_function_call_value(
        builder.make_identifier(&std::string_view("Ok"), info.result_ok_member, false, loc) as *mut Value, loc
    )
    ok_call.add_generic_arg(info.result_type.getArgumentType(0), loc)
    ok_call.add_generic_arg(info.result_type.getArgumentType(1), loc)
    ok_call.get_args().push(struct_val as *mut Value)
    body.push(builder.make_return_stmt(ok_call as *mut Value, deserialize_fn as *mut ASTNode, loc) as *mut ASTNode)

    return deserialize_fn
}

@no_mangle
public func json_symResNode(visitor : *mut SymResLinkBody, node : *mut EmbeddedNode) {
    const resolver = visitor.getSymbolResolver()
    const info = node.getDataPtr() as *mut SerializableInfo
    const loc = node.getEncodedLocation()
    var builder = resolver.getJobBuilder()

    // resolve the struct (supports namespaced paths like models::Note)
    info.struct_node = resolve_type_path(resolver, &info.struct_name)
    if(info.struct_node == null) { resolver.error(std::string_view("struct not found"), loc); return }

    // read struct members. Field types that have no Serializer/Deserializer
    // support are rejected HERE with a clean error at the #json site - otherwise
    // the unresolved generic dispatch corrupts the json library's own symres
    // with cascade errors reported inside lang/libs/json/src/encode.ch.
    var struct_def = info.struct_node as *StructDefinition
    var members = struct_def.getMembers()
    if(members != null) {
        for(var i = 0u; i < members.size(); i++) {
            var member = members.get(i)
            var mname = member.getName()
            var mtype = member.getType()
            var unsupported = unsupported_field_reason(mtype)
            if(unsupported.size() > 0) {
                resolver.error(&unsupported, loc)
                // abort this #json (no impls built) so the compile fails HERE
                // with the clean message instead of silently dropping the field
                return
            }
            info.field_names.push(builder.allocate_view(&mname))
            info.field_types.push(mtype)
        }
    }

    if(!resolve_types(resolver, info, loc)) { return }

    // === Build & visit generic types (triggers instantiation) ===
    // These must be instantiated during symres so the generic interface decl
    // becomes a concrete interface decl (linked as a pointer). We store them
    // and reuse them as-is during codegen replacement.

    // Serializer<JsonValue, JsonEncoder>
    var serializer_linked = builder.make_linked_type(&std::string_view("Serializer"), info.serializer_node, loc)
    var json_value_linked = builder.make_linked_type(&std::string_view("JsonValue"), info.json_value_node, loc)
    var json_encoder_linked = builder.make_linked_type(&std::string_view("JsonEncoder"), info.json_encoder_node, loc)
    var ser_iface_type = builder.make_generic_type_with_args(
        serializer_linked as *mut LinkedType,
        std::span<*mut BaseType>([ json_value_linked as *mut BaseType, json_encoder_linked as *mut BaseType ]),
        std::span<ubigint>([ loc, loc ])
    )
    visitor.visitType(ser_iface_type as *mut BaseType, loc)
    info.serializer_iface_type = ser_iface_type

    // Result<Point, SerializationError>
    var result_linked = builder.make_linked_type(&std::string_view("Result"), info.result_node, loc)
    var struct_type = builder.make_linked_type(&info.struct_name, info.struct_node, loc)
    var se_linked = builder.make_linked_type(&std::string_view("SerializationError"), info.serialization_error_node, loc)
    var result_type = builder.make_generic_type_with_args(
        result_linked as *mut LinkedType,
        std::span<*mut BaseType>([ struct_type as *mut BaseType, se_linked as *mut BaseType ]),
        std::span<ubigint>([ loc, loc ])
    )
    visitor.visitType(result_type as *mut BaseType, loc)
    info.result_type = result_type

    // Result<Unit, SerializationError> for the serialize return type
    // NOTE: must use FRESH LinkedType instances here — GenericType::instantiate
    // mutates referenced->linked, so sharing the same `result_linked`/`se_linked`
    // with the above Result<Point, SerializationError> would make this alias
    // that already-instantiated type instead of creating its own.
    var result_linked2 = builder.make_linked_type(&std::string_view("Result"), info.result_node, loc)
    var se_linked2 = builder.make_linked_type(&std::string_view("SerializationError"), info.serialization_error_node, loc)
    var unit_linked = builder.make_linked_type(&std::string_view("Unit"), info.unit_node, loc)
    var ser_result_type = builder.make_generic_type_with_args(
        result_linked2 as *mut LinkedType,
        std::span<*mut BaseType>([ unit_linked as *mut BaseType, se_linked2 as *mut BaseType ]),
        std::span<ubigint>([ loc, loc ])
    )
    visitor.visitType(ser_result_type as *mut BaseType, loc)
    info.ser_result_type = ser_result_type

    // Deserializer<Point>
    var deser_linked = builder.make_linked_type(&std::string_view("Deserializer"), info.deserializer_node, loc)
    var deser_iface_type = builder.make_generic_type_with_args(
        deser_linked as *mut LinkedType,
        std::span<*mut BaseType>([ struct_type as *mut BaseType ]),
        std::span<ubigint>([ loc ])
    )
    visitor.visitType(deser_iface_type as *mut BaseType, loc)
    info.deserializer_iface_type = deser_iface_type

    // TypeDecoder<Point>
    var td_linked = builder.make_linked_type(&std::string_view("TypeDecoder"), info.type_decoder_node, loc)
    var type_decoder_type = builder.make_generic_type_with_args(
        td_linked as *mut LinkedType,
        std::span<*mut BaseType>([ struct_type as *mut BaseType ]),
        std::span<ubigint>([ loc ])
    )
    visitor.visitType(type_decoder_type as *mut BaseType, loc)
    info.type_decoder_type = type_decoder_type

    // === Build the replacement scope + impl defs FIRST ===
    // Contained functions must have the IMPL DEF as their AST parent so the
    // mangler emits the impl-context name (std_stdSerializer__cgs__0_Point_serialize)
    // that vtable generation expects — exactly like parsed `impl ... for ...` code.
    var impl_scope = builder.make_scope(info.struct_node as *mut ASTNode, loc)

    // impl std::Serializer<JsonValue, JsonEncoder> for Point
    var struct_linked = builder.make_linked_type(&info.struct_name, info.struct_node, loc)
    var ser_impl = builder.make_impl_def(
        info.serializer_iface_type as *mut BaseType,
        struct_linked as *mut BaseType,
        impl_scope as *mut ASTNode,
        loc
    )
    impl_scope.getNodes().push(ser_impl as *mut ASTNode)

    // impl std::Deserializer<Point> for TypeDecoder<Point>
    var deser_impl = builder.make_impl_def(
        info.deserializer_iface_type as *mut BaseType,
        info.type_decoder_type as *mut BaseType,
        impl_scope as *mut ASTNode,
        loc
    )
    impl_scope.getNodes().push(deser_impl as *mut ASTNode)

    // === Build + link serialize function (contained in ser_impl) ===
    var serialize_fn = build_serialize_fn(&raw mut builder, info, members, ser_impl as *mut ASTNode, loc)
    ser_impl.add_function(&raw mut builder, serialize_fn)
    // register the impl with the interface + adopt its contained funcs into the
    // struct (so `point.serialize(...)` resolves to the override) + store it in
    // implsIndex (so generic dispatch like `decode<T>()` can find it). mirrors
    // what the module's index phase does for parsed impls.
    resolver.index_impl(ser_impl as *mut ImplDefinition)
    visitor.visitNode(ser_impl as *mut ASTNode)

    // Pre-instantiate Result<field_type, SerializationError> for struct-typed
    // fields (the ones decoded through the generic decode<T>). Giving the
    // generated decode-result var an explicit concrete type keeps its pattern
    // matches & identifiers concrete too: without it, the decode<T> call stays
    // typed as the generic master Result while the instantiation finalizes
    // asynchronously, and the pattern-match temp vars emit the master type.
    info.field_result_types.clear()
    for(var i = 0u; i < info.field_types.size(); i++) {
        var ftype = info.field_types.get(i)
        if(find_decoder_method(info, ftype) != null) {
            info.field_result_types.push(null)
            continue
        }
        var fret_linked = builder.make_linked_type(&std::string_view("Result"), info.result_node, loc)
        var fse_linked = builder.make_linked_type(&std::string_view("SerializationError"), info.serialization_error_node, loc)
        var fres_type = builder.make_generic_type_with_args(
            fret_linked as *mut LinkedType,
            std::span<*mut BaseType>([ ftype, fse_linked as *mut BaseType ]),
            std::span<ubigint>([ loc, loc ])
        )
        visitor.visitType(fres_type as *mut BaseType, loc)
        info.field_result_types.push(fres_type)
    }

    // === Build + link deserialize function (contained in deser_impl) ===
    var deserialize_fn = build_deserialize_fn(&raw mut builder, info, members, deser_impl as *mut ASTNode, loc)
    deser_impl.add_function(&raw mut builder, deserialize_fn)
    resolver.index_impl(deser_impl as *mut ImplDefinition)
    visitor.visitNode(deser_impl as *mut ASTNode)

    info.replacement_scope = impl_scope
}

// ===== Codegen Phase =====
// The replacement scope (impl Serializer for the struct + impl Deserializer for
// TypeDecoder<struct>) was already built & symres-linked inside json_symResNode,
// so both the 2c declaration pass and the body pass just return the SAME scope
// from info. Reusing identical nodes across the passes mirrors parsed impls:
// the declare pass writes prototypes/vtables, the body pass writes definitions.

@no_mangle
public func json_replacementNodeDeclare(builder : *mut ASTBuilder, node : *mut EmbeddedNode) : *ASTNode {
    const info = node.getDataPtr() as *mut SerializableInfo
    return info.replacement_scope as *ASTNode
}

@no_mangle
public func json_replacementNode(builder : *mut ASTBuilder, node : *mut EmbeddedNode) : *mut ASTNode {
    const info = node.getDataPtr() as *mut SerializableInfo
    return info.replacement_scope
}