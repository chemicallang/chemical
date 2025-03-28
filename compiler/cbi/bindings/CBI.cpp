// Copyright (c) Chemical Language Foundation 2025.

#include "CBI.h"
#include "SourceProviderCBI.h"
#include "BuildContextCBI.h"
#include "BatchAllocatorCBI.h"
#include "PtrVecCBI.h"
#include "std/chem_string.h"
#include "ASTBuilderCBI.h"
#include "ASTCBI.h"
#include "SerialStrAllocatorCBI.h"
#include "LexerCBI.h"
#include "ParserCBI.h"

dispose_string::~dispose_string(){
    ptr->~string();
}

chem::string* init_chem_string(chem::string* str) {
    str->storage.constant.data = "";
    str->storage.constant.length = 0;
    str->state = '0';
    return str;
}

const std::pair<chem::string_view, void*> BuildContextSymMap[] = {
        { "BuildContextmodule_from_directory", (void*) BuildContextmodule_from_directory },
        { "BuildContextfiles_module", (void*) BuildContextfiles_module },
        { "BuildContextchemical_files_module", (void*) BuildContextchemical_files_module },
        { "BuildContextchemical_dir_module", (void*) BuildContextchemical_dir_module },
        { "BuildContextc_file_module", (void*) BuildContextc_file_module },
        { "BuildContextcpp_file_module", (void*) BuildContextcpp_file_module },
        { "BuildContextobject_module", (void*) BuildContextobject_module },
        { "BuildContextadd_compiler_interface", (void*) BuildContextadd_compiler_interface },
        { "BuildContextresolve_import_path", (void*) BuildContextresolve_import_path },
        { "BuildContextresolve_native_lib_path", (void*) BuildContextresolve_native_lib_path },
        { "BuildContextinclude_header", (void*) BuildContextinclude_header },
        { "BuildContexttranslate_to_chemical", (void*) BuildContexttranslate_to_chemical },
        { "BuildContexttranslate_to_c", (void*) BuildContexttranslate_to_c },
        { "BuildContextbuild_exe", (void*) BuildContextbuild_exe },
        { "BuildContextbuild_dynamic_lib", (void*) BuildContextbuild_dynamic_lib },
        { "BuildContextbuild_cbi", (void*) BuildContextbuild_cbi },
        { "BuildContextadd_cbi_type", (void*) BuildContextadd_cbi_type },
        { "BuildContextadd_object", (void*) BuildContextadd_object },
        { "BuildContextdeclare_alias", (void*) BuildContextdeclare_alias },
        { "BuildContextbuild_path", (void*) BuildContextbuild_path },
        { "BuildContexthas_arg", (void*) BuildContexthas_arg },
        { "BuildContextget_arg", (void*) BuildContextget_arg },
        { "BuildContextremove_arg", (void*) BuildContextremove_arg },
        { "BuildContextdefine", (void*) BuildContextdefine },
        { "BuildContextundefine", (void*) BuildContextundefine },
        { "AppBuildContextlaunch_executable", (void*) AppBuildContextlaunch_executable },
        { "AppBuildContexton_finished", (void*) AppBuildContexton_finished },
        { "BuildContextlink_objects", (void*) BuildContextlink_objects },
        { "BuildContextinvoke_dlltool", (void*) BuildContextinvoke_dlltool },
        { "BuildContextinvoke_ranlib", (void*) BuildContextinvoke_ranlib },
        { "BuildContextinvoke_lib", (void*) BuildContextinvoke_lib },
        { "BuildContextinvoke_ar", (void*) BuildContextinvoke_ar }
};

const std::pair<chem::string_view, void*> BatchAllocatorSymMap[] = {
        { "BatchAllocatorallocate_size", (void*) BatchAllocatorallocate_size },
};

const std::pair<chem::string_view, void*> SerialStrAllocatorSymMap[] = {
        {"SerialStrAllocatordeallocate", (void*) SerialStrAllocatordeallocate },
        {"SerialStrAllocatorcurrent_view", (void*) SerialStrAllocatorcurrent_view },
        {"SerialStrAllocatorfinalize_view", (void*) SerialStrAllocatorfinalize_view },
        {"SerialStrAllocatorappend", (void*) SerialStrAllocatorappend }
};

const std::pair<chem::string_view, void*> SourceProviderSymMap[] = {
        { "SourceProviderreadCharacter", (void*) SourceProviderreadCharacter },
        { "SourceProvidereof", (void*) SourceProvidereof },
        { "SourceProviderpeek", (void*) SourceProviderpeek },
        { "SourceProviderincrement_char", (void*) SourceProviderincrement_char },
        { "SourceProvidergetLineNumber", (void*) SourceProvidergetLineNumber },
        { "SourceProvidergetLineCharNumber", (void*) SourceProvidergetLineCharNumber },
        { "SourceProviderreadWhitespaces", (void*) SourceProviderreadWhitespaces },
        { "SourceProviderhasNewLine", (void*) SourceProviderhasNewLine },
        { "SourceProviderreadNewLineChars", (void*) SourceProviderreadNewLineChars },
        { "SourceProviderreadWhitespacesAndNewLines", (void*) SourceProviderreadWhitespacesAndNewLines },
};

const std::pair<chem::string_view, void*> LexerSymMap[] = {
        {"LexergetFileAllocator",    (void*) LexergetFileAllocator },
        {"LexersetUserLexer",    (void*) LexersetUserLexer },
        {"LexerunsetUserLexer",    (void*) LexerunsetUserLexer },
        {"LexergetEmbeddedToken",    (void*) LexergetEmbeddedToken }
};

const std::pair<chem::string_view, void*> ParserSymMap[] = {
        {"ParsergetTokenPtr",    (void*) ParsergetTokenPtr },
        {"ParsergetGlobalBuilder",    (void*) ParsergetGlobalBuilder },
        {"ParsergetModuleBuilder",    (void*) ParsergetModuleBuilder },
        {"ParsergetIs64Bit",    (void*) ParsergetIs64Bit },
        {"ParsergetParentNodePtr",    (void*) ParsergetParentNodePtr },
        {"ParsergetCurrentFilePath",    (void*) ParsergetCurrentFilePath },
        {"ParserparseExpression",    (void*) ParserparseExpression },
        {"Parsererror_at",    (void*) Parsererror_at },
};

const std::pair<chem::string_view, void*> PtrVecSymMap[] = {
        { "PtrVec_get", (void*) PtrVec_get },
        { "PtrVec_size", (void*) PtrVec_size },
        { "PtrVec_set", (void*) PtrVec_set },
        { "PtrVec_erase", (void*) PtrVec_erase },
        { "PtrVec_push", (void*) PtrVec_push },
};

const std::pair<chem::string_view, void*> ASTBuilderSymMap[] = {
        { "ASTBuilderallocate_with_cleanup", (void*) ASTBuilderallocate_with_cleanup },
        { "ASTBuildercreateType", (void*) ASTBuildercreateType },
        { "ASTBuildermake_sym_res_node", (void*) ASTBuildermake_sym_res_node },
        { "ASTBuildermake_sym_res_value", (void*) ASTBuildermake_sym_res_value },
        { "ASTBuildermake_any_type", (void*) ASTBuildermake_any_type },
        { "ASTBuildermake_array_type", (void*) ASTBuildermake_array_type },
        { "ASTBuildermake_bigint_type", (void*) ASTBuildermake_bigint_type },
        { "ASTBuildermake_bool_type", (void*) ASTBuildermake_bool_type },
        { "ASTBuildermake_char_type", (void*) ASTBuildermake_char_type },
        { "ASTBuildermake_double_type", (void*) ASTBuildermake_double_type },
        { "ASTBuildermake_dynamic_type", (void*) ASTBuildermake_dynamic_type },
        { "ASTBuildermake_float_type", (void*) ASTBuildermake_float_type },
        { "ASTBuildermake_func_type", (void*) ASTBuildermake_func_type },
        { "ASTBuildermake_generic_type", (void*) ASTBuildermake_generic_type },
        { "ASTBuildermake_int128_type", (void*) ASTBuildermake_int128_type },
        { "ASTBuildermake_int_type", (void*) ASTBuildermake_int_type },
        { "ASTBuildermake_linked_type", (void*) ASTBuildermake_linked_type },
        { "ASTBuildermake_linked_value_type", (void*) ASTBuildermake_linked_value_type },
        { "ASTBuildermake_literal_type", (void*) ASTBuildermake_literal_type },
        { "ASTBuildermake_long_type", (void*) ASTBuildermake_long_type },
        { "ASTBuildermake_ptr_type", (void*) ASTBuildermake_ptr_type },
        { "ASTBuildermake_reference_type", (void*) ASTBuildermake_reference_type },
        { "ASTBuildermake_short_type", (void*) ASTBuildermake_short_type },
        { "ASTBuildermake_string_type", (void*) ASTBuildermake_string_type },
        { "ASTBuildermake_ubigint_type", (void*) ASTBuildermake_ubigint_type },
        { "ASTBuildermake_uchar_type", (void*) ASTBuildermake_uchar_type },
        { "ASTBuildermake_uint128_type", (void*) ASTBuildermake_uint128_type },
        { "ASTBuildermake_uint_type", (void*) ASTBuildermake_uint_type },
        { "ASTBuildermake_ulong_type", (void*) ASTBuildermake_ulong_type },
        { "ASTBuildermake_ushort_type", (void*) ASTBuildermake_ushort_type },
        { "ASTBuildermake_void_type", (void*) ASTBuildermake_void_type },
        { "ASTBuildermake_access_chain", (void*) ASTBuildermake_access_chain },
        { "ASTBuildermake_value_wrapper", (void*) ASTBuildermake_value_wrapper },
        { "ASTBuildermake_addr_of_value", (void*) ASTBuildermake_addr_of_value },
        { "ASTBuildermake_array_value", (void*) ASTBuildermake_array_value },
        { "ASTBuildermake_bigint_value", (void*) ASTBuildermake_bigint_value },
        { "ASTBuildermake_bool_value", (void*) ASTBuildermake_bool_value },
        { "ASTBuildermake_casted_value", (void*) ASTBuildermake_casted_value },
        { "ASTBuildermake_char_value", (void*) ASTBuildermake_char_value },
        { "ASTBuildermake_dereference_value", (void*) ASTBuildermake_dereference_value },
        { "ASTBuildermake_double_value", (void*) ASTBuildermake_double_value },
        { "ASTBuildermake_expression_value", (void*) ASTBuildermake_expression_value },
        { "ASTBuildermake_float_value", (void*) ASTBuildermake_float_value },
        { "ASTBuildermake_function_call_value", (void*) ASTBuildermake_function_call_value },
        { "ASTBuildermake_index_op_value", (void*) ASTBuildermake_index_op_value },
        { "ASTBuildermake_int128_value", (void*) ASTBuildermake_int128_value },
        { "ASTBuildermake_int_value", (void*) ASTBuildermake_int_value },
        { "ASTBuildermake_is_value", (void*) ASTBuildermake_is_value },
        { "ASTBuildermake_lambda_function", (void*) ASTBuildermake_lambda_function },
        { "ASTBuildermake_captured_variable", (void*) ASTBuildermake_captured_variable },
        { "ASTBuildermake_long_value", (void*) ASTBuildermake_long_value },
        { "ASTBuildermake_negative_value", (void*) ASTBuildermake_negative_value },
        { "ASTBuildermake_not_value", (void*) ASTBuildermake_not_value },
        { "ASTBuildermake_null_value", (void*) ASTBuildermake_null_value },
        { "ASTBuildermake_number_value", (void*) ASTBuildermake_number_value },
        { "ASTBuildermake_short_value", (void*) ASTBuildermake_short_value },
        { "ASTBuildermake_sizeof_value", (void*) ASTBuildermake_sizeof_value },
        { "ASTBuildermake_string_value", (void*) ASTBuildermake_string_value },
        { "ASTBuildermake_struct_value", (void*) ASTBuildermake_struct_value },
        { "ASTBuildermake_ubigint_value", (void*) ASTBuildermake_ubigint_value },
        { "ASTBuildermake_uchar_value", (void*) ASTBuildermake_uchar_value },
        { "ASTBuildermake_uint128_value", (void*) ASTBuildermake_uint128_value },
        { "ASTBuildermake_uint_value", (void*) ASTBuildermake_uint_value },
        { "ASTBuildermake_ulong_value", (void*) ASTBuildermake_ulong_value },
        { "ASTBuildermake_ushort_value", (void*) ASTBuildermake_ushort_value },
        { "ASTBuildermake_block_value", (void*) ASTBuildermake_block_value },
        { "ASTBuildermake_value_node", (void*) ASTBuildermake_value_node },
        { "ASTBuildermake_identifier", (void*) ASTBuildermake_identifier },
        { "ASTBuildermake_variant_case", (void*) ASTBuildermake_variant_case },
        { "ASTBuildermake_variant_case_variable", (void*) ASTBuildermake_variant_case_variable },
        { "ASTBuildermake_assignment_stmt", (void*) ASTBuildermake_assignment_stmt },
        { "ASTBuildermake_break_stmt", (void*) ASTBuildermake_break_stmt },
        { "ASTBuildermake_continue_stmt", (void*) ASTBuildermake_continue_stmt },
        { "ASTBuildermake_destruct_stmt", (void*) ASTBuildermake_destruct_stmt },
        { "ASTBuildermake_return_stmt", (void*) ASTBuildermake_return_stmt },
        { "ASTBuildermake_typealias_stmt", (void*) ASTBuildermake_typealias_stmt },
        { "ASTBuildermake_using_stmt", (void*) ASTBuildermake_using_stmt },
        { "ASTBuildermake_varinit_stmt", (void*) ASTBuildermake_varinit_stmt },
        { "ASTBuildermake_scope", (void*) ASTBuildermake_scope },
        { "ASTBuildermake_do_while_loop", (void*) ASTBuildermake_do_while_loop },
        { "ASTBuildermake_enum_decl", (void*) ASTBuildermake_enum_decl },
        { "ASTBuildermake_enum_member", (void*) ASTBuildermake_enum_member },
        { "ASTBuildermake_for_loop", (void*) ASTBuildermake_for_loop },
        { "ASTBuildermake_function", (void*) ASTBuildermake_function },
        { "ASTBuildermake_function_param", (void*) ASTBuildermake_function_param },
        { "ASTBuildermake_generic_param", (void*) ASTBuildermake_generic_param },
        { "ASTBuildermake_if_stmt", (void*) ASTBuildermake_if_stmt },
        { "ASTBuildermake_impl_def", (void*) ASTBuildermake_impl_def },
        { "ASTBuildermake_init_block", (void*) ASTBuildermake_init_block },
        { "ASTBuildermake_interface_def", (void*) ASTBuildermake_interface_def },
        { "ASTBuildermake_namespace", (void*) ASTBuildermake_namespace },
        { "ASTBuildermake_struct_def", (void*) ASTBuildermake_struct_def },
        { "ASTBuildermake_struct_member", (void*) ASTBuildermake_struct_member },
        { "ASTBuildermake_union_def", (void*) ASTBuildermake_union_def },
        { "ASTBuildermake_unsafe_block", (void*) ASTBuildermake_unsafe_block },
        { "ASTBuildermake_while_loop", (void*) ASTBuildermake_while_loop },
        { "ASTBuildermake_variant_def", (void*) ASTBuildermake_variant_def },
        { "ASTBuildermake_variant_member", (void*) ASTBuildermake_variant_member },
        { "ASTBuildermake_variant_member_param", (void*) ASTBuildermake_variant_member_param },
        { "compiler_ValuegetKind", (void*) ValuegetKind },
        { "compiler_Valuelink", (void*) Valuelink },
        { "ValuegetLinkedNode", (void*) ValuegetLinkedNode },
        { "ASTNodegetKind", (void*) ASTNodegetKind },
        { "ASTNodedeclare_top_level", (void*) ASTNodedeclare_top_level },
        { "compiler_ASTNodedeclare_and_link", (void*) ASTNodedeclare_and_link },
        { "compiler_BaseTypegetKind", (void*) BaseTypegetKind },
        { "BaseTypelink", (void*) BaseTypelink },
        { "BaseTypegetLinkedNode", (void*) BaseTypegetLinkedNode },
        { "FunctionTypeget_params", (void*) FunctionTypeget_params },
        { "GenericTypeget_types", (void*) GenericTypeget_types },
        { "compiler_AccessChainget_values", (void*) AccessChainget_values },
        { "AccessChainas_value", (void*) AccessChainas_value },
        { "ArrayValueget_values", (void*) ArrayValueget_values },
        { "ArrayValueadd_size", (void*) ArrayValueadd_size },
        { "compiler_FunctionCallget_args", (void*) FunctionCallget_args },
        { "IndexOperatorget_values", (void*) IndexOperatorget_values },
        { "LambdaFunctionget_params", (void*) LambdaFunctionget_params },
        { "LambdaFunctionget_capture_list", (void*) LambdaFunctionget_capture_list },
        { "LambdaFunctionget_body", (void*) LambdaFunctionget_body },
        { "StructValueadd_value", (void*) StructValueadd_value },
        { "VariantCaseadd_variable", (void*) VariantCaseadd_variable },
        { "compiler_ScopegetNodes", (void*) ScopegetNodes },
        { "Scopelink_sequentially", (void*) Scopelink_sequentially },
        { "Scopelink_asynchronously", (void*) Scopelink_asynchronously },
        { "DoWhileLoopget_body", (void*) DoWhileLoopget_body },
        { "WhileLoopget_body", (void*) WhileLoopget_body },
        { "ForLoopget_body", (void*) ForLoopget_body },
        { "EnumDeclarationadd_member", (void*) EnumDeclarationadd_member },
        { "FunctionDeclarationget_params", (void*) FunctionDeclarationget_params },
        { "FunctionDeclarationadd_body", (void*) FunctionDeclarationadd_body },
        { "IfStatementget_body", (void*) IfStatementget_body },
        { "IfStatementadd_else_body", (void*) IfStatementadd_else_body },
        { "IfStatementadd_else_if", (void*) IfStatementadd_else_if },
        { "ImplDefinitionadd_function", (void*) ImplDefinitionadd_function },
        { "StructDefinitionadd_member", (void*) StructDefinitionadd_member },
        { "StructDefinitionadd_function", (void*) StructDefinitionadd_function },
        { "InterfaceDefinitionadd_function", (void*) InterfaceDefinitionadd_function },
        { "Namespaceget_children", (void*) Namespaceget_body },
        { "UnsafeBlockget_children", (void*) UnsafeBlockget_body },
        { "BlockValueget_body", (void*) BlockValueget_body },
        { "UnionDefinitionadd_member", (void*) UnionDefinitionadd_member },
        { "UnionDefinitionadd_function", (void*) UnionDefinitionadd_function },
        { "VariantDefinitionadd_member", (void*) VariantDefinitionadd_member },
        { "VariantMemberadd_param", (void*) VariantMemberadd_param },
        { "InitBlockadd_initializer", (void*) InitBlockadd_initializer },
};

void prepare_cbi_maps(std::unordered_map<chem::string_view, std::span<const std::pair<chem::string_view, void*>>>& interface_maps) {
    interface_maps.reserve(9);
    interface_maps.emplace("SourceProvider", SourceProviderSymMap);
    interface_maps.emplace("BatchAllocator", BatchAllocatorSymMap);
    interface_maps.emplace("SerialStrAllocator", SerialStrAllocatorSymMap);
    interface_maps.emplace("Lexer", LexerSymMap);
    interface_maps.emplace("Parser", ParserSymMap);
    interface_maps.emplace("BuildContext", BuildContextSymMap);
    interface_maps.emplace("ASTBuilder", ASTBuilderSymMap);
    interface_maps.emplace("PtrVec", PtrVecSymMap);
    // couldn't create a constant empty array
    interface_maps["SymbolResolver"] = {};
}