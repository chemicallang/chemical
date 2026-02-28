// Copyright (c) Chemical Language Foundation 2025.

#include "CBI.h"
#include "SourceProviderCBI.h"
#include "BuildContextCBI.h"
#include "LabCBIAddons.h"
#include "BatchAllocatorCBI.h"
#include "PtrVecCBI.h"
#include "std/chem_string.h"
#include "ASTBuilderCBI.h"
#include "ASTCBI.h"
#include "SymbolResolverCBI.h"
#include "LexerCBI.h"
#include "ParserCBI.h"
#include "ASTDiagnoserCBI.h"
#include "AnnotationControllerCBI.h"
#include "TransformerContextCBI.h"

#ifdef LSP_BUILD
#include "compiler/cbi/bindings/lsp/LSPHooks.h"
#endif

const std::pair<chem::string_view, void*> BuildContextSymMap[] = {
        { "lab_BuildContextgetAnnotationController", (void*) BuildContextgetAnnotationController },
        { "lab_BuildContextnew_package", (void*) BuildContextnew_package },
        { "lab_BuildContextset_module_symbol_info", (void*) BuildContextset_module_symbol_info },
        { "lab_BuildContextget_cached", (void*) BuildContextget_cached },
        { "lab_BuildContextset_cached", (void*) BuildContextset_cached },
        { "lab_BuildContextadd_path", (void*) BuildContextadd_path },
        { "lab_BuildContextadd_module", (void*) BuildContextadd_module },
        { "lab_BuildContextadd_dependency", (void*) BuildContextadd_dependency },
        { "lab_BuildContextput_job_before", (void*) BuildContextput_job_before },
        { "lab_BuildContextlink_system_lib", (void*) BuildContextlink_system_lib },
        { "lab_BuildContextadd_compiler_interface", (void*) BuildContextadd_compiler_interface },
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
        { "lab_BuildContextinvoke_ar", (void*) BuildContextinvoke_ar },
        { "lab_BuildContextset_conflict_resolution_strategy", (void*) BuildContextset_conflict_resolution_strategy },
        { "lab_BuildContextfetch_job_dependency", (void*) BuildContextfetch_job_dependency },
        { "lab_BuildContextfetch_mod_dependency", (void*) BuildContextfetch_mod_dependency },


        // Module functions
        { "lab_ModulegetType", (void*) ModulegetType },
        { "lab_ModulegetScopeName", (void*) ModulegetScopeName },
        { "lab_ModulegetName", (void*) ModulegetName },
        { "lab_ModulegetBitcodePath", (void*) ModulegetBitcodePath },
        { "lab_ModulesetBitcodePath", (void*) ModulesetBitcodePath },
        { "lab_ModulegetObjectPath", (void*) ModulegetObjectPath },
        { "lab_ModulesetObjectPath", (void*) ModulesetObjectPath },
        { "lab_ModulegetLlvmIrPath", (void*) ModulegetLlvmIrPath },
        { "lab_ModulesetLlvmIrPath", (void*) ModulesetLlvmIrPath },
        { "lab_ModulegetAsmPath", (void*) ModulegetAsmPath },
        { "lab_ModulesetAsmPath", (void*) ModulesetAsmPath },

        // LabJob functions
        { "lab_LabJobgetType", (void*) LabJobgetType },
        { "lab_LabJobgetName", (void*) LabJobgetName },
        { "lab_LabJobgetAbsPath", (void*) LabJobgetAbsPath },
        { "lab_LabJobgetBuildDir", (void*) LabJobgetBuildDir },
        { "lab_LabJobgetStatus", (void*) LabJobgetStatus },
        { "lab_LabJobgetTargetTriple", (void*) LabJobgetTargetTriple },
        { "lab_LabJobgetMode", (void*) LabJobgetMode },
        { "lab_LabJobgetTarget", (void*) LabJobgetTarget }
};

const std::pair<chem::string_view, void*> BatchAllocatorSymMap[] = {
        { "compiler_BatchAllocatorallocate_size", (void*) BatchAllocatorallocate_size },
};

const std::pair<chem::string_view, void*> AnnotationControllerSymMap[] = {
        { "lab_AnnotationControllercreateSingleMarkerAnnotation", (void*) AnnotationControllercreateSingleMarkerAnnotation },
        { "lab_AnnotationControllercreateMarkerAnnotation", (void*) AnnotationControllercreateMarkerAnnotation },
        { "lab_AnnotationControllercreateCollectorAnnotation", (void*) AnnotationControllercreateCollectorAnnotation },
        { "lab_AnnotationControllercreateMarkerAndCollectorAnnotation", (void*) AnnotationControllercreateMarkerAndCollectorAnnotation },
        { "compiler_AnnotationControllergetDefinition", (void*) AnnotationControllergetDefinition },
        { "compiler_AnnotationControllermarkSingle", (void*) AnnotationControllermarkSingle },
        { "compiler_AnnotationControllermark", (void*) AnnotationControllermark },
        { "compiler_AnnotationControllercollect", (void*) AnnotationControllercollect },
        { "compiler_AnnotationControllermarkAndCollect", (void*) AnnotationControllermarkAndCollect },
        { "compiler_AnnotationControllerhandleAnnotation", (void*) AnnotationControllerhandleAnnotation },
        { "compiler_AnnotationControllerisMarked", (void*) AnnotationControllerisMarked },

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
        {"compiler_ParsergetEncodedLocation",    (void*) ParsergetEncodedLocation },
        {"compiler_ParsergetAnnotationController",    (void*) ParsergetAnnotationController },
        {"compiler_ParsergetIs64Bit",    (void*) ParsergetIs64Bit },
        {"compiler_ParsergetParentNodePtr",    (void*) ParsergetParentNodePtr },
        {"compiler_ParsergetCurrentFilePath",    (void*) ParsergetCurrentFilePath },
        {"compiler_ParserparseExpression",    (void*) ParserparseExpression },
        {"compiler_ParserparseNestedLevelStatement",    (void*) ParserparseNestedLevelStatement },
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
        { "compiler_PointerTypegetChildType", (void*) PointerTypegetChildType },
        { "compiler_ReferenceTypegetChildType", (void*) ReferenceTypegetChildType },
        { "compiler_TypealiasStatementgetActualType", (void*) TypealiasStatementgetActualType },
        { "compiler_FunctionTypeget_params", (void*) FunctionTypeget_params },
        { "compiler_FunctionTypegetReturnType", (void*) FunctionTypegetReturnType },
        { "compiler_AccessChainget_values", (void*) AccessChainget_values },
        { "compiler_AccessChainas_value", (void*) AccessChainas_value },
        { "compiler_ArrayValueget_values", (void*) ArrayValueget_values },
        { "compiler_ExpressiveStringgetValues", (void*) ExpressiveStringgetValues },
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
        { "compiler_EmbeddedNodegetDataPtr", (void*) EmbeddedNodegetDataPtr },
        { "compiler_EmbeddedValuegetDataPtr", (void*) EmbeddedValuegetDataPtr },
        { "compiler_FunctionDeclarationgetReturnType", (void*) FunctionDeclarationgetReturnType },
        { "compiler_FunctionParamgetName", (void*) FunctionParamgetName },
        { "compiler_FunctionParamgetType", (void*) FunctionParamgetType },
        { "compiler_BaseDefMembergetName", (void*) BaseDefMembergetName },
        { "compiler_BaseDefMembergetType", (void*) BaseDefMembergetType },
        { "compiler_StructDefinitiongetMembers", (void*) StructDefinitiongetMembers },
        { "compiler_StructDefinitiongetFunctions", (void*) StructDefinitiongetFunctions },
        { "compiler_InterfaceDefinitiongetFunctions", (void*) InterfaceDefinitiongetFunctions },
        { "compiler_EnumDeclarationgetMembers", (void*) EnumDeclarationgetMembers },
        { "compiler_VariantDefinitiongetMembers", (void*) VariantDefinitiongetMembers },
        { "compiler_FunctionDeclarationgetName", (void*) FunctionDeclarationgetName },
        { "compiler_StructDefinitiongetName", (void*) StructDefinitiongetName },
        { "compiler_InterfaceDefinitiongetName", (void*) InterfaceDefinitiongetName },
        { "compiler_NamespacegetName", (void*) NamespacegetName },
        { "compiler_EnumDeclarationgetName", (void*) EnumDeclarationgetName },
        { "compiler_EnumMembergetName", (void*) EnumMembergetName },
        { "compiler_VariantDefinitiongetName", (void*) VariantDefinitiongetName },
        { "compiler_UnionDefinitiongetName", (void*) UnionDefinitiongetName },
        { "compiler_UnionDefgetName", (void*) UnionDefinitiongetName },
        { "compiler_GenericTypegetArgumentCount", (void*) GenericTypegetArgumentCount },
        { "compiler_GenericTypegetArgumentType", (void*) GenericTypegetArgumentType },
        { "compiler_GenericTypegetArgumentLocation", (void*) GenericTypegetArgumentLocation },
        { "compiler_FileScopegetBody", (void*) FileScopegetBody },
        { "compiler_Namespaceget_body", (void*) Namespaceget_body },
        { "compiler_StructDefinitiongetMembers", (void*) StructDefinitiongetMembers },
        { "compiler_StructDefinitiongetFunctions", (void*) StructDefinitiongetFunctions },
        { "compiler_InterfaceDefinitiongetFunctions", (void*) InterfaceDefinitiongetFunctions },
        { "compiler_EnumDeclarationgetMembers", (void*) EnumDeclarationgetMembers },
        { "compiler_VariantDefinitiongetMembers", (void*) VariantDefinitiongetMembers },
        { "compiler_UnionDefgetMembers", (void*) UnionDefinitiongetMembers },
        { "compiler_UnionDefgetFunctions", (void*) UnionDefinitiongetFunctions },
        { "compiler_FunctionDeclarationgetAttributes", (void*) FunctionDeclarationgetAttributes },
        { "compiler_InterfaceDefinitiongetAttributes", (void*) InterfaceDefinitiongetAttributes },
        { "compiler_TypealiasStatementgetAttributes", (void*) TypealiasStatementgetAttributes },
        { "compiler_ASTNodegetAccessSpecifier", (void*) ASTNodegetAccessSpecifier },
        { "compiler_TypealiasStatementgetName", (void*) TypealiasStatementgetName },
        { "compiler_VariablesContainergetInheritedCount", (void*) VariablesContainergetInheritedCount },
        { "compiler_VariablesContainergetInheritedType", (void*) VariablesContainergetInheritedType },

        { "compiler_BaseGenericDeclgetGenericParams", (void*) BaseGenericDeclgetGenericParams },
        { "compiler_GenericStructDeclgetMasterImpl", (void*) GenericStructDeclgetMasterImpl },
        { "compiler_GenericFuncDeclgetMasterImpl", (void*) GenericFuncDeclgetMasterImpl },
        { "compiler_GenericVariantDeclgetMasterImpl", (void*) GenericVariantDeclgetMasterImpl },
        { "compiler_GenericUnionDeclgetMasterImpl", (void*) GenericUnionDeclgetMasterImpl },
        { "compiler_GenericInterfaceDeclgetMasterImpl", (void*) GenericInterfaceDeclgetMasterImpl },
        { "compiler_GenericTypeParametergetName", (void*) GenericTypeParametergetName },
        { "compiler_GenericTypeParametergetDefaultType", (void*) GenericTypeParametergetDefaultType },
        { "compiler_FunctionDeclarationisExtensionFn", (void*) FunctionDeclarationisExtensionFn },
        { "compiler_ArrayTypegetElementType", (void*) ArrayTypegetElementType },
        { "compiler_ArrayTypegetArraySize", (void*) ArrayTypegetArraySize },
        { "compiler_DynamicTypegetChildType", (void*) DynamicTypegetChildType },
        { "compiler_LiteralTypegetChildType", (void*) LiteralTypegetChildType },
};

const std::pair<chem::string_view, void*> SymbolResolverSymMap[] = {
        {"compiler_SymbolResolvergetAnnotationController", (void*) SymbolResolvergetAnnotationController},
        {"compiler_SymbolResolverfind", (void*) SymbolResolverfind},
        {"compiler_SymbolResolverdeclare", (void*) SymbolResolverdeclare},
};

const std::pair<chem::string_view, void*> TransformerContextSymMap[] = {
        {"transformer_TransformerContextgetTargetJob",        (void*) TransformerContextgetTargetJob},
        {"transformer_TransformerContextparseTarget",         (void*) TransformerContextparseTarget},
        {"transformer_TransformerContextanalyzeTarget",       (void*) TransformerContextanalyzeTarget},
        {"transformer_TransformerContextgetFlattenedModules", (void*) TransformerContextgetFlattenedModules},
        {"transformer_TransformerContextgetFileTokens",       (void*) TransformerContextgetFileTokens},
        {"transformer_TransformerContextdecodeLocation",      (void*) TransformerContextdecodeLocation},


        {"transformer_ASTFileMetaDatagetFileId",              (void*) FileMetaDatagetFileId},
        {"transformer_ASTFileMetaDatagetAbsPath",             (void*) FileMetaDatagetAbsPath},
        {"transformer_ASTFileMetaDatagetFileScope",           (void*) FileMetaDatagetFileScope},

        {"transformer_TransformerModulegetFiles",             (void*) ModulegetFiles},
        {"transformer_TransformerModulegetFileCount",         (void*) ModulegetFileCount},
        {"transformer_TransformerModulegetFile",              (void*) ModulegetFile},
        {"transformer_TransformerFileScopegetBody",           (void*) FileScopegetBody},
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
    interface_maps.emplace("AnnotationController", AnnotationControllerSymMap);
    interface_maps.emplace("Lexer", LexerSymMap);
    interface_maps.emplace("Parser", ParserSymMap);
    interface_maps.emplace("BuildContext", BuildContextSymMap);
    interface_maps.emplace("ASTBuilder", ASTBuilderSymMap);
    interface_maps.emplace("PtrVec", PtrVecSymMap);
    interface_maps.emplace("SymbolResolver", SymbolResolverSymMap);
    interface_maps.emplace("ASTDiagnoser", ASTDiagnoserSymMap);
    interface_maps.emplace("TransformerContext", TransformerContextSymMap);
#ifdef LSP_BUILD
    interface_maps.emplace("LSPAnalyzers", LSPAnalyzersMap);
#endif
}