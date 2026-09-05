struct SerializableInfo {
    var struct_name : std::string_view
    var struct_node : *mut ASTNode = null
    var field_names : std::vector<std::string_view>
    var field_types : std::vector<*mut BaseType>
    var json_encoder_node : *mut ASTNode = null
    var json_value_node : *mut ASTNode = null
    var serializer_node : *mut ASTNode = null
    var deserializer_node : *mut ASTNode = null
    var result_node : *mut ASTNode = null
    var result_ok_member : *mut ASTNode = null
    var result_err_member : *mut ASTNode = null
    var unit_node : *mut ASTNode = null
    var se_repl_node : *mut ASTNode = null
    var serialization_error_node : *mut ASTNode = null
    // json-lib take_ok helper (moves an Ok payload out of a Result<T, SerializationError>,
    // leaving the result in an Err state so the payload can be moved onward into an
    // aggregate literal - used by generated deserialize for destructible fields)
    var take_ok_node : *mut ASTNode = null
    var encoder_object_method : *mut ASTNode = null
    var object_encoder_field_method : *mut ASTNode = null
    var json_decoder_node : *mut ASTNode = null
    var decoder_decode_i64_method : *mut ASTNode = null
    var decoder_decode_u64_method : *mut ASTNode = null
    var decoder_decode_double_method : *mut ASTNode = null
    var decoder_decode_float_method : *mut ASTNode = null
    var decoder_decode_str_method : *mut ASTNode = null
    var decoder_decode_bool_method : *mut ASTNode = null
    var decoder_decode_char_method : *mut ASTNode = null
    // object navigation: JsonDecoder.object() returns the JsonObjectDecoder
    // (whose item_decoder() yields each key/value pair); each item's `.second`
    // is a fresh JsonDecoder for that field's value. decoded like the handwritten
    // json.ch reference: object() first, then one item_decoder() per field.
    var json_object_decoder_node : *mut ASTNode = null
    var decoder_object_method : *mut ASTNode = null
    var decoder_decode_generic_method : *mut ASTNode = null
    var object_decoder_item_method : *mut ASTNode = null
    // std::pair's `second` member decl (item_decoder returns pair<string_view, JsonDecoder>)
    var pair_second_member : *mut ASTNode = null
    var type_decoder_node : *mut ASTNode = null
    // per-field Result<field_type, SerializationError> type (only for struct-typed
    // fields, decoded via decode<T>); pre-instantiated in symres so the generated
    // decode result var + pattern matches carry a concrete type instead of the
    // generic master Result while the decode<T> instantiation is in flight
    var field_result_types : std::vector<*mut GenericType>
    // generic types built & visited during symres (instantiated there),
    // then reused as-is during codegen replacement
    var serializer_iface_type : *mut GenericType = null
    var deserializer_iface_type : *mut GenericType = null
    var type_decoder_type : *mut GenericType = null
    var result_type : *mut GenericType = null
    var ser_result_type : *mut GenericType = null
    var serialize_fn : *mut FunctionDeclaration = null
    var deserialize_fn : *mut FunctionDeclaration = null
    // impl defs + scope built & symres-linked during json_symResNode; returned
    // by the codegen replacement hooks (declare + body passes share these nodes)
    var replacement_scope : *mut Scope = null
}

public func known_type_fn(value : *EmbeddedNode) : *BaseType { return null }
public func child_res_fn(value : *EmbeddedNode, name : &std::string_view) : *ASTNode { return null }
public func cross_mod_sym_decl_proxy_fn(obj : *mut void, node : *mut EmbeddedNode, fn : CrossModuleSymbolDeclarerFn, at_least_spec : AccessSpecifier) { }