// Copyright (c) Chemical Language Foundation 2025.

#include "CBI.h"
#include "SourceProviderCBI.h"
#include "BuildContextCBI.h"
#include "BatchAllocatorCBI.h"
#include "PtrVecCBI.h"
#include "std/chem_string.h"
#include "ASTBuilderCBI.h"
#include "ASTCBI.h"
#include "SymbolResolverCBI.h"
#include "LexerCBI.h"
#include "ParserCBI.h"
#include "ASTDiagnoserCBI.h"

#ifdef LSP_BUILD
#include "compiler/cbi/bindings/lsp/LSPHooks.h"
#endif

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
        { "lab_BuildContextnew_module", (void*) BuildContextnew_module },
        { "lab_BuildContextget_cached", (void*) BuildContextget_cached },
        { "lab_BuildContextset_cached", (void*) BuildContextset_cached },
        { "lab_BuildContextadd_path", (void*) BuildContextadd_path },
        { "lab_BuildContextadd_module", (void*) BuildContextadd_module },
        { "lab_BuildContextfiles_module", (void*) BuildContextfiles_module },
        { "lab_BuildContextchemical_dir_module", (void*) BuildContextchemical_dir_module },
        { "lab_BuildContextc_file_module", (void*) BuildContextc_file_module },
        { "lab_BuildContextcpp_file_module", (void*) BuildContextcpp_file_module },
        { "lab_BuildContextobject_module", (void*) BuildContextobject_module },
        { "lab_BuildContextput_job_before", (void*) BuildContextput_job_before },
        { "lab_BuildContextlink_system_lib", (void*) BuildContextlink_system_lib },
        { "lab_BuildContextadd_compiler_interface", (void*) BuildContextadd_compiler_interface },
        { "lab_BuildContextresolve_import_path", (void*) BuildContextresolve_import_path },
        { "lab_BuildContextresolve_native_lib_path", (void*) BuildContextresolve_native_lib_path },
        { "lab_BuildContextinclude_header", (void*) BuildContextinclude_header },
        { "lab_BuildContexttranslate_to_chemical", (void*) BuildContexttranslate_to_chemical },
        { "lab_BuildContexttranslate_to_c", (void*) BuildContexttranslate_to_c },
        { "lab_BuildContextbuild_exe", (void*) BuildContextbuild_exe },
        { "lab_BuildContextrun_jit_exe", (void*) BuildContextrun_jit_exe },
        { "lab_BuildContextbuild_dynamic_lib", (void*) BuildContextbuild_dynamic_lib },
        { "lab_BuildContextbuild_cbi", (void*) BuildContextbuild_cbi },
        { "lab_BuildContextset_environment_testing", (void*) BuildContextset_environment_testing },
        { "lab_BuildContextindex_cbi_fn", (void*) BuildContextindex_cbi_fn },
        { "lab_BuildContextadd_object", (void*) BuildContextadd_object },
        { "lab_BuildContextdeclare_alias", (void*) BuildContextdeclare_alias },
        { "lab_BuildContextbuild_path", (void*) BuildContextbuild_path },
        { "lab_BuildContexthas_arg", (void*) BuildContexthas_arg },
        { "lab_BuildContextget_arg", (void*) BuildContextget_arg },
        { "lab_BuildContextremove_arg", (void*) BuildContextremove_arg },
        { "lab_BuildContextdefine", (void*) BuildContextdefine },
        { "lab_BuildContextundefine", (void*) BuildContextundefine },
        { "lab_AppBuildContextlaunch_executable", (void*) AppBuildContextlaunch_executable },
        { "lab_AppBuildContexton_finished", (void*) AppBuildContexton_finished },
        { "lab_BuildContextinvoke_dlltool", (void*) BuildContextinvoke_dlltool },
        { "lab_BuildContextinvoke_ranlib", (void*) BuildContextinvoke_ranlib },
        { "lab_BuildContextinvoke_lib", (void*) BuildContextinvoke_lib },
        { "lab_BuildContextinvoke_ar", (void*) BuildContextinvoke_ar }
};

const std::pair<chem::string_view, void*> BatchAllocatorSymMap[] = {
        { "compiler_BatchAllocatorallocate_size", (void*) BatchAllocatorallocate_size },
};

const std::pair<chem::string_view, void*> SourceProviderSymMap[] = {
        { "compiler_SourceProviderincrement", (void*) SourceProviderincrement },
        { "compiler_SourceProviderreadCharacter", (void*) SourceProviderreadCharacter },
        { "compiler_SourceProviderreadCodePoint", (void*) SourceProviderreadCodePoint },
        { "compiler_SourceProviderutf8_decode_peek", (void*) SourceProviderutf8_decode_peek },
        { "compiler_SourceProviderincrementCodepoint", (void*) SourceProviderincrementCodepoint },
        { "compiler_SourceProvidereof", (void*) SourceProvidereof },
        { "compiler_SourceProviderpeek", (void*) SourceProviderpeek },
        { "compiler_SourceProviderincrement_char", (void*) SourceProviderincrement_char },
        { "compiler_SourceProvidergetLineNumber", (void*) SourceProvidergetLineNumber },
        { "compiler_SourceProvidergetLineCharNumber", (void*) SourceProvidergetLineCharNumber },
        { "compiler_SourceProviderreadWhitespaces", (void*) SourceProviderreadWhitespaces },
        { "compiler_SourceProviderhasNewLine", (void*) SourceProviderhasNewLine },
        { "compiler_SourceProviderreadNewLineChars", (void*) SourceProviderreadNewLineChars },
        { "compiler_SourceProviderreadWhitespacesAndNewLines", (void*) SourceProviderreadWhitespacesAndNewLines },
};

const std::pair<chem::string_view, void*> LexerSymMap[] = {
        {"compiler_LexergetFileAllocator",    (void*) LexergetFileAllocator },
        {"compiler_LexersetUserLexer",    (void*) LexersetUserLexer },
        {"compiler_LexerunsetUserLexer",    (void*) LexerunsetUserLexer },
        {"compiler_LexergetEmbeddedToken",    (void*) LexergetEmbeddedToken }
};

const std::pair<chem::string_view, void*> ParserSymMap[] = {
        {"compiler_ParsergetTokenPtr",    (void*) ParsergetTokenPtr },
        {"compiler_ParsergetGlobalBuilder",    (void*) ParsergetGlobalBuilder },
        {"compiler_ParsergetModuleBuilder",    (void*) ParsergetModuleBuilder },
        {"compiler_ParsergetIs64Bit",    (void*) ParsergetIs64Bit },
        {"compiler_ParsergetParentNodePtr",    (void*) ParsergetParentNodePtr },
        {"compiler_ParsergetCurrentFilePath",    (void*) ParsergetCurrentFilePath },
        {"compiler_ParserparseExpression",    (void*) ParserparseExpression },
        {"compiler_Parsererror_at",    (void*) Parsererror_at },
};

const std::pair<chem::string_view, void*> PtrVecSymMap[] = {
        { "compiler_PtrVec_get", (void*) PtrVec_get },
        { "compiler_PtrVec_size", (void*) PtrVec_size },
        { "compiler_PtrVec_set", (void*) PtrVec_set },
        { "compiler_PtrVec_erase", (void*) PtrVec_erase },
        { "compiler_PtrVec_push", (void*) PtrVec_push },
};

const std::pair<chem::string_view, void*> ASTBuilderSymMap[] = {
        { "compiler_ASTBuilderallocate_with_cleanup", (void*) ASTBuilderallocate_with_cleanup },
        { "compiler_ASTBuilderstore_cleanup", (void*) ASTBuilderstore_cleanup },
        { "compiler_ASTBuildermake_embedded_node", (void*) ASTBuildermake_embedded_node },
        { "compiler_ASTBuildermake_embedded_value", (void*) ASTBuildermake_embedded_value },
        { "compiler_ASTBuildermake_any_type", (void*) ASTBuildermake_any_type },
        { "compiler_ASTBuildermake_array_type", (void*) ASTBuildermake_array_type },
        { "compiler_ASTBuildermake_bool_type", (void*) ASTBuildermake_bool_type },


        // chemical integer types
        { "compiler_ASTBuilderget_i8_type", (void*) ASTBuilderget_i8_type },
        { "compiler_ASTBuilderget_i16_type", (void*) ASTBuilderget_i16_type },
        { "compiler_ASTBuilderget_i32_type", (void*) ASTBuilderget_i32_type },
        { "compiler_ASTBuilderget_i64_type", (void*) ASTBuilderget_i64_type },
        { "compiler_ASTBuilderget_i128_type", (void*) ASTBuilderget_i128_type },

        // chemical integer types (unsigned)
        { "compiler_ASTBuilderget_u8_type", (void*) ASTBuilderget_u8_type },
        { "compiler_ASTBuilderget_u16_type", (void*) ASTBuilderget_u16_type },
        { "compiler_ASTBuilderget_u32_type", (void*) ASTBuilderget_u32_type },
        { "compiler_ASTBuilderget_u64_type", (void*) ASTBuilderget_u64_type },
        { "compiler_ASTBuilderget_u128_type", (void*) ASTBuilderget_u128_type },

        // c like integer types
        { "compiler_ASTBuilderget_char_type", (void*) ASTBuilderget_char_type },
        { "compiler_ASTBuilderget_short_type", (void*) ASTBuilderget_short_type },
        { "compiler_ASTBuilderget_int_type", (void*) ASTBuilderget_int_type },
        { "compiler_ASTBuilderget_long_type", (void*) ASTBuilderget_long_type },
        { "compiler_ASTBuilderget_longlong_type", (void*) ASTBuilderget_longlong_type },

        // c like integer types (unsigned)
        { "compiler_ASTBuilderget_uchar_type", (void*) ASTBuilderget_uchar_type },
        { "compiler_ASTBuilderget_ushort_type", (void*) ASTBuilderget_ushort_type },
        { "compiler_ASTBuilderget_uint_type", (void*) ASTBuilderget_uint_type },
        { "compiler_ASTBuilderget_ulong_type", (void*) ASTBuilderget_ulong_type },
        { "compiler_ASTBuilderget_ulonglong_type", (void*) ASTBuilderget_ulonglong_type },

        { "compiler_ASTBuildermake_double_type", (void*) ASTBuildermake_double_type },
        { "compiler_ASTBuildermake_dynamic_type", (void*) ASTBuildermake_dynamic_type },
        { "compiler_ASTBuildermake_float_type", (void*) ASTBuildermake_float_type },
        { "compiler_ASTBuildermake_func_type", (void*) ASTBuildermake_func_type },
        { "compiler_ASTBuildermake_generic_type", (void*) ASTBuildermake_generic_type },
        { "compiler_ASTBuildermake_linked_type", (void*) ASTBuildermake_linked_type },
        { "compiler_ASTBuildermake_linked_value_type", (void*) ASTBuildermake_linked_value_type },
        { "compiler_ASTBuildermake_literal_type", (void*) ASTBuildermake_literal_type },
        { "compiler_ASTBuildermake_ptr_type", (void*) ASTBuildermake_ptr_type },
        { "compiler_ASTBuildermake_reference_type", (void*) ASTBuildermake_reference_type },
        { "compiler_ASTBuildermake_string_type", (void*) ASTBuildermake_string_type },
        { "compiler_ASTBuildermake_void_type", (void*) ASTBuildermake_void_type },
        { "compiler_ASTBuildermake_access_chain", (void*) ASTBuildermake_access_chain },
        { "compiler_ASTBuildermake_value_wrapper", (void*) ASTBuildermake_value_wrapper },
        { "compiler_ASTBuildermake_addr_of_value", (void*) ASTBuildermake_addr_of_value },
        { "compiler_ASTBuildermake_array_value", (void*) ASTBuildermake_array_value },
        { "compiler_ASTBuildermake_bigint_value", (void*) ASTBuildermake_bigint_value },
        { "compiler_ASTBuildermake_bool_value", (void*) ASTBuildermake_bool_value },
        { "compiler_ASTBuildermake_casted_value", (void*) ASTBuildermake_casted_value },
        { "compiler_ASTBuildermake_char_value", (void*) ASTBuildermake_char_value },
        { "compiler_ASTBuildermake_dereference_value", (void*) ASTBuildermake_dereference_value },
        { "compiler_ASTBuildermake_double_value", (void*) ASTBuildermake_double_value },
        { "compiler_ASTBuildermake_expression_value", (void*) ASTBuildermake_expression_value },
        { "compiler_ASTBuildermake_float_value", (void*) ASTBuildermake_float_value },
        { "compiler_ASTBuildermake_function_call_value", (void*) ASTBuildermake_function_call_value },
        { "compiler_ASTBuildermake_function_call_node", (void*) ASTBuildermake_function_call_node },
        { "compiler_ASTBuildermake_index_op_value", (void*) ASTBuildermake_index_op_value },
        { "compiler_ASTBuildermake_int128_value", (void*) ASTBuildermake_int128_value },
        { "compiler_ASTBuildermake_int_value", (void*) ASTBuildermake_int_value },
        { "compiler_ASTBuildermake_is_value", (void*) ASTBuildermake_is_value },
        { "compiler_ASTBuildermake_lambda_function", (void*) ASTBuildermake_lambda_function },
        { "compiler_ASTBuildermake_captured_variable", (void*) ASTBuildermake_captured_variable },
        { "compiler_ASTBuildermake_long_value", (void*) ASTBuildermake_long_value },
        { "compiler_ASTBuildermake_negative_value", (void*) ASTBuildermake_negative_value },
        { "compiler_ASTBuildermake_not_value", (void*) ASTBuildermake_not_value },
        { "compiler_ASTBuildermake_null_value", (void*) ASTBuildermake_null_value },
        { "compiler_ASTBuildermake_number_value", (void*) ASTBuildermake_number_value },
        { "compiler_ASTBuildermake_short_value", (void*) ASTBuildermake_short_value },
        { "compiler_ASTBuildermake_sizeof_value", (void*) ASTBuildermake_sizeof_value },
        { "compiler_ASTBuildermake_string_value", (void*) ASTBuildermake_string_value },
        { "compiler_ASTBuildermake_struct_value", (void*) ASTBuildermake_struct_value },
        { "compiler_ASTBuildermake_ubigint_value", (void*) ASTBuildermake_ubigint_value },
        { "compiler_ASTBuildermake_uchar_value", (void*) ASTBuildermake_uchar_value },
        { "compiler_ASTBuildermake_uint128_value", (void*) ASTBuildermake_uint128_value },
        { "compiler_ASTBuildermake_uint_value", (void*) ASTBuildermake_uint_value },
        { "compiler_ASTBuildermake_ulong_value", (void*) ASTBuildermake_ulong_value },
        { "compiler_ASTBuildermake_ushort_value", (void*) ASTBuildermake_ushort_value },
        { "compiler_ASTBuildermake_block_value", (void*) ASTBuildermake_block_value },
        { "compiler_ASTBuildermake_value_node", (void*) ASTBuildermake_value_node },
        { "compiler_ASTBuildermake_identifier", (void*) ASTBuildermake_identifier },
        { "compiler_ASTBuildermake_variant_case", (void*) ASTBuildermake_variant_case },
        { "compiler_ASTBuildermake_variant_case_variable", (void*) ASTBuildermake_variant_case_variable },
        { "compiler_ASTBuildermake_assignment_stmt", (void*) ASTBuildermake_assignment_stmt },
        { "compiler_ASTBuildermake_break_stmt", (void*) ASTBuildermake_break_stmt },
        { "compiler_ASTBuildermake_continue_stmt", (void*) ASTBuildermake_continue_stmt },
        { "compiler_ASTBuildermake_destruct_stmt", (void*) ASTBuildermake_destruct_stmt },
        { "compiler_ASTBuildermake_return_stmt", (void*) ASTBuildermake_return_stmt },
        { "compiler_ASTBuildermake_typealias_stmt", (void*) ASTBuildermake_typealias_stmt },
        { "compiler_ASTBuildermake_using_stmt", (void*) ASTBuildermake_using_stmt },
        { "compiler_ASTBuildermake_varinit_stmt", (void*) ASTBuildermake_varinit_stmt },
        { "compiler_ASTBuildermake_scope", (void*) ASTBuildermake_scope },
        { "compiler_ASTBuildermake_do_while_loop", (void*) ASTBuildermake_do_while_loop },
        { "compiler_ASTBuildermake_enum_decl", (void*) ASTBuildermake_enum_decl },
        { "compiler_ASTBuildermake_enum_member", (void*) ASTBuildermake_enum_member },
        { "compiler_ASTBuildermake_for_loop", (void*) ASTBuildermake_for_loop },
        { "compiler_ASTBuildermake_function", (void*) ASTBuildermake_function },
        { "compiler_ASTBuildermake_function_param", (void*) ASTBuildermake_function_param },
        { "compiler_ASTBuildermake_generic_param", (void*) ASTBuildermake_generic_param },
        { "compiler_ASTBuildermake_if_stmt", (void*) ASTBuildermake_if_stmt },
        { "compiler_ASTBuildermake_impl_def", (void*) ASTBuildermake_impl_def },
        { "compiler_ASTBuildermake_init_block", (void*) ASTBuildermake_init_block },
        { "compiler_ASTBuildermake_interface_def", (void*) ASTBuildermake_interface_def },
        { "compiler_ASTBuildermake_namespace", (void*) ASTBuildermake_namespace },
        { "compiler_ASTBuildermake_struct_def", (void*) ASTBuildermake_struct_def },
        { "compiler_ASTBuildermake_struct_member", (void*) ASTBuildermake_struct_member },
        { "compiler_ASTBuildermake_union_def", (void*) ASTBuildermake_union_def },
        { "compiler_ASTBuildermake_unsafe_block", (void*) ASTBuildermake_unsafe_block },
        { "compiler_ASTBuildermake_while_loop", (void*) ASTBuildermake_while_loop },
        { "compiler_ASTBuildermake_variant_def", (void*) ASTBuildermake_variant_def },
        { "compiler_ASTBuildermake_variant_member", (void*) ASTBuildermake_variant_member },
        { "compiler_ASTBuildermake_variant_member_param", (void*) ASTBuildermake_variant_member_param },
        { "compiler_ASTAnygetAnyKind", (void*) ASTAnygetAnyKind },
        { "compiler_ValuegetEncodedLocation", (void*) ValuegetEncodedLocation },
        { "compiler_ValuegetKind", (void*) ValuegetKind },
        { "compiler_ValuegetType", (void*) ValuegetType },
        { "compiler_ASTNodegetEncodedLocation", (void*) ASTNodegetEncodedLocation },
        { "compiler_ASTNodegetKind", (void*) ASTNodegetKind },
        { "compiler_ASTNodechild", (void*) ASTNodechild },
        { "compiler_BaseTypegetKind", (void*) BaseTypegetKind },
        { "compiler_IntNTypeget_intn_type_kind", (void*) IntNTypeget_intn_type_kind },
        { "compiler_LinkedTypegetLinkedNode", (void*) LinkedTypegetLinkedNode },
        { "compiler_GenericTypegetLinkedType", (void*) GenericTypegetLinkedType },
        { "compiler_TypealiasStatementgetActualType", (void*) TypealiasStatementgetActualType },
        { "compiler_FunctionTypeget_params", (void*) FunctionTypeget_params },
        { "compiler_AccessChainget_values", (void*) AccessChainget_values },
        { "compiler_AccessChainas_value", (void*) AccessChainas_value },
        { "compiler_ArrayValueget_values", (void*) ArrayValueget_values },
        { "compiler_FunctionCallget_args", (void*) FunctionCallget_args },
        { "compiler_FunctionCallNodeget_args", (void*) FunctionCallNodeget_args },
        { "compiler_IndexOperatorget_idx_ptr", (void*) IndexOperatorget_idx_ptr },
        { "compiler_LambdaFunctionget_params", (void*) LambdaFunctionget_params },
        { "compiler_LambdaFunctionget_capture_list", (void*) LambdaFunctionget_capture_list },
        { "compiler_LambdaFunctionget_body", (void*) LambdaFunctionget_body },
        { "compiler_StructValueadd_value", (void*) StructValueadd_value },
        { "compiler_VariantCaseadd_variable", (void*) VariantCaseadd_variable },
        { "compiler_ScopegetNodes", (void*) ScopegetNodes },
        { "compiler_DoWhileLoopget_body", (void*) DoWhileLoopget_body },
        { "compiler_WhileLoopget_body", (void*) WhileLoopget_body },
        { "compiler_ForLoopget_body", (void*) ForLoopget_body },
        { "compiler_EnumDeclarationadd_member", (void*) EnumDeclarationadd_member },
        { "compiler_FunctionDeclarationget_params", (void*) FunctionDeclarationget_params },
        { "compiler_FunctionDeclarationadd_body", (void*) FunctionDeclarationadd_body },
        { "compiler_IfStatementget_body", (void*) IfStatementget_body },
        { "compiler_IfStatementadd_else_body", (void*) IfStatementadd_else_body },
        { "compiler_IfStatementadd_else_if", (void*) IfStatementadd_else_if },
        { "compiler_ImplDefinitionadd_function", (void*) ImplDefinitionadd_function },
        { "compiler_StructDefinitionadd_member", (void*) StructDefinitionadd_member },
        { "compiler_StructDefinitionadd_function", (void*) StructDefinitionadd_function },
        { "compiler_InterfaceDefinitionadd_function", (void*) InterfaceDefinitionadd_function },
        { "compiler_Namespaceget_children", (void*) Namespaceget_body },
        { "compiler_UnsafeBlockget_children", (void*) UnsafeBlockget_body },
        { "compiler_BlockValueget_body", (void*) BlockValueget_body },
        { "compiler_BlockValuesetCalculatedValue", (void*) BlockValuesetCalculatedValue },
        { "compiler_UnionDefinitionadd_member", (void*) UnionDefinitionadd_member },
        { "compiler_UnionDefinitionadd_function", (void*) UnionDefinitionadd_function },
        { "compiler_VariantDefinitionadd_member", (void*) VariantDefinitionadd_member },
        { "compiler_VariantMemberadd_param", (void*) VariantMemberadd_param },
        { "compiler_InitBlockadd_initializer", (void*) InitBlockadd_initializer },
        { "compiler_EmbeddedNodegetDataPtr", (void*) EmbeddedNodegetDataPtr },
        { "compiler_EmbeddedValuegetDataPtr", (void*) EmbeddedValuegetDataPtr },
};

const std::pair<chem::string_view, void*> SymbolResolverSymMap[] = {
        {"compiler_SymbolResolverfind", (void*) SymbolResolverfind},
};

const std::pair<chem::string_view, void*> ASTDiagnoserSymMap[] = {
        {"compiler_ASTDiagnosererror", (void*) ASTDiagnosererror},
};

#ifdef LSP_BUILD
const std::pair<chem::string_view, void*> LSPAnalyzersMap[] = {
        {"ide_SemanticTokensAnalyzerputAuto", (void*) SemanticTokensAnalyzerputAuto},
        {"ide_SemanticTokensAnalyzerput", (void*) SemanticTokensAnalyzerput},
        {"ide_SemanticTokensAnalyzerputToken", (void*) SemanticTokensAnalyzerputToken},
        {"ide_FoldingRangeAnalyzerput", (void*) FoldingRangeAnalyzerput},
        {"ide_FoldingRangeAnalyzerstackPush", (void*) FoldingRangeAnalyzerstackPush},
        {"ide_FoldingRangeAnalyzerstackEmpty", (void*) FoldingRangeAnalyzerstackEmpty},
        {"ide_FoldingRangeAnalyzerstackPop", (void*) FoldingRangeAnalyzerstackPop},
};
#endif

void prepare_cbi_maps(std::unordered_map<chem::string_view, std::span<const std::pair<chem::string_view, void*>>>& interface_maps) {
    interface_maps.reserve(9);
    interface_maps.emplace("SourceProvider", SourceProviderSymMap);
    interface_maps.emplace("BatchAllocator", BatchAllocatorSymMap);
    interface_maps.emplace("Lexer", LexerSymMap);
    interface_maps.emplace("Parser", ParserSymMap);
    interface_maps.emplace("BuildContext", BuildContextSymMap);
    interface_maps.emplace("ASTBuilder", ASTBuilderSymMap);
    interface_maps.emplace("PtrVec", PtrVecSymMap);
    interface_maps.emplace("SymbolResolver", SymbolResolverSymMap);
    interface_maps.emplace("ASTDiagnoser", ASTDiagnoserSymMap);
#ifdef LSP_BUILD
    interface_maps.emplace("LSPAnalyzers", LSPAnalyzersMap);
#endif
}