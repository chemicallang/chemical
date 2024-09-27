// Copyright (c) Qinetik 2024.

#include "CBI.h"
#include "SourceProviderCBI.h"
#include "LexerCBI.h"
#include "BuildContextCBI.h"
#include "CSTTokenCBI.h"
#include "CSTconverterCBI.h"
#include "PtrVecCBI.h"
#include "std/chem_string.h"
#include "ASTBuilderCBI.h"
#include "ASTCBI.h"

dispose_string::~dispose_string(){
    ptr->~string();
}

chem::string* init_chem_string(chem::string* str) {
    str->storage.constant.data = nullptr;
    str->storage.constant.length = 0;
    str->state = '0';
    return str;
}

void build_context_symbol_map(std::unordered_map<std::string_view, void*>& sym_map) {
    sym_map = {
        { "BuildContextfiles_module", BuildContextfiles_module },
        { "BuildContextchemical_files_module", BuildContextchemical_files_module },
        { "BuildContextchemical_dir_module", BuildContextchemical_dir_module },
        { "BuildContextc_file_module", BuildContextc_file_module },
        { "BuildContextcpp_file_module", BuildContextcpp_file_module },
        { "BuildContextobject_module", BuildContextobject_module },
        { "BuildContexttranslate_to_chemical", BuildContexttranslate_to_chemical },
        { "BuildContexttranslate_to_c", BuildContexttranslate_to_c },
        { "BuildContextbuild_exe", BuildContextbuild_exe },
        { "BuildContextbuild_dynamic_lib", BuildContextbuild_dynamic_lib },
        { "BuildContextbuild_cbi", BuildContextbuild_cbi },
        { "BuildContextadd_object", BuildContextadd_object },
        { "BuildContextdeclare_alias", BuildContextdeclare_alias },
        { "BuildContextbuild_path", BuildContextbuild_path },
        { "BuildContexthas_arg", BuildContexthas_arg },
        { "BuildContextget_arg", BuildContextget_arg },
        { "BuildContextremove_arg", BuildContextremove_arg },
        { "BuildContextdefine", BuildContextdefine },
        { "BuildContextundefine", BuildContextundefine },
        { "BuildContextlaunch_executable", BuildContextlaunch_executable },
        { "BuildContexton_finished", BuildContexton_finished },
        { "BuildContextlink_objects", BuildContextlink_objects },
        { "BuildContextinvoke_dlltool", BuildContextinvoke_dlltool },
        { "BuildContextinvoke_ranlib", BuildContextinvoke_ranlib },
        { "BuildContextinvoke_lib", BuildContextinvoke_lib },
        { "BuildContextinvoke_ar", BuildContextinvoke_ar }
    };
}

constexpr std::string sp_func(const std::string& string) {
    return "SourceProvider" + string;
}

void source_provider_symbol_map(std::unordered_map<std::string_view, void*>& sym_map) {
    sym_map = {
        { "SourceProviderreadCharacter", SourceProviderreadCharacter },
        { "SourceProvidereof", SourceProvidereof },
        { "SourceProviderpeek", SourceProviderpeek },
        { "SourceProviderreadUntil", SourceProviderreadUntil },
        { "SourceProviderincrement", SourceProviderincrement },
        { "SourceProviderincrement_char", SourceProviderincrement_char },
        { "SourceProvidergetLineNumber", SourceProvidergetLineNumber },
        { "SourceProvidergetLineCharNumber", SourceProvidergetLineCharNumber },
        { "SourceProviderreadEscaping", SourceProviderreadEscaping },
        { "SourceProviderreadAnything", SourceProviderreadAnything },
        { "SourceProviderreadAlpha", SourceProviderreadAlpha },
        { "SourceProviderreadUnsignedInt", SourceProviderreadUnsignedInt },
        { "SourceProviderreadNumber", SourceProviderreadNumber },
        { "SourceProviderreadAlphaNum", SourceProviderreadAlphaNum },
        { "SourceProviderreadIdentifier", SourceProviderreadIdentifier },
        { "SourceProviderreadAnnotationIdentifier", SourceProviderreadAnnotationIdentifier },
        { "SourceProviderreadWhitespaces", SourceProviderreadWhitespaces },
        { "SourceProviderhasNewLine", SourceProviderhasNewLine },
        { "SourceProviderreadNewLineChars", SourceProviderreadNewLineChars },
        { "SourceProviderreadWhitespacesAndNewLines", SourceProviderreadWhitespacesAndNewLines },

    };
}

constexpr std::string lexer_func(const std::string& name) {
    return "Lexer" + name;
};

void lexer_symbol_map(std::unordered_map<std::string_view, void*>& sym_map) {
    sym_map = {
            { "Lexerprovider", Lexerprovider },
            { "Lexertokens_size", Lexertokens_size },
            { "Lexerput", Lexerput },
            { "LexerstoreVariable", LexerstoreVariable },
            { "LexerstoreIdentifier", LexerstoreIdentifier },
            { "LexerlexGenericArgsList", LexerlexGenericArgsList },
            { "LexerlexGenericArgsListCompound", LexerlexGenericArgsListCompound },
            { "LexerlexFunctionCallWithGenericArgsList", LexerlexFunctionCallWithGenericArgsList },
            { "LexerlexFunctionCall", LexerlexFunctionCall },
            { "LexerlexAccessSpecifier", LexerlexAccessSpecifier },
            { "LexerlexAccessChainAfterId", LexerlexAccessChainAfterId },
            { "LexerlexAccessChainRecursive", LexerlexAccessChainRecursive },
            { "LexerlexAccessChain", LexerlexAccessChain },
            { "LexerlexAccessChainOrAddrOf", LexerlexAccessChainOrAddrOf },
            { "LexerlexVarInitializationTokens", LexerlexVarInitializationTokens },
            { "LexerlexAssignmentTokens", LexerlexAssignmentTokens },
            { "LexerlexDivisionOperatorToken", LexerlexDivisionOperatorToken },
            { "LexerlexLanguageOperatorToken", LexerlexLanguageOperatorToken },
            { "LexerisGenericEndAhead", LexerisGenericEndAhead },
            { "LexerlexAssignmentOperatorToken", LexerlexAssignmentOperatorToken },
            { "LexerlexLambdaTypeTokens", LexerlexLambdaTypeTokens },
            { "LexerlexGenericTypeAfterId", LexerlexGenericTypeAfterId },
            { "LexerlexRefOrGenericType", LexerlexRefOrGenericType },
            { "LexerlexArrayAndPointerTypesAfterTypeId", LexerlexArrayAndPointerTypesAfterTypeId },
            { "LexerlexTypeTokens", LexerlexTypeTokens },
            { "LexerlexTopLevelAccessSpecifiedDecls", LexerlexTopLevelAccessSpecifiedDecls },
            { "LexerlexTopLevelStatementTokens", LexerlexTopLevelStatementTokens },
            { "LexerlexNestedLevelStatementTokens", LexerlexNestedLevelStatementTokens },
            { "LexerlexStatementTokens", LexerlexStatementTokens },
            { "LexerlexThrowStatementTokens", LexerlexThrowStatementTokens },
            { "LexerlexOperatorToken", LexerlexOperatorToken },
            { "LexerlexOperatorTokenStr", LexerlexOperatorTokenStr },
            { "LexerstoreOperationToken", LexerstoreOperationToken },
            { "LexerlexOperationToken", LexerlexOperationToken },
            { "LexerlexOperatorTokenStr2", LexerlexOperatorTokenStr2 },
            { "LexerlexKeywordToken", LexerlexKeywordToken },
            { "LexerlexWSKeywordToken", LexerlexWSKeywordToken },
            { "LexerlexWSKeywordToken2", LexerlexWSKeywordToken2 },
            { "LexerlexTopLevelMultipleStatementsTokens", LexerlexTopLevelMultipleStatementsTokens },
            { "LexerlexTopLevelMultipleImportStatements", LexerlexTopLevelMultipleImportStatements },
            { "LexerlexNestedLevelMultipleStatementsTokens", LexerlexNestedLevelMultipleStatementsTokens },
            { "LexerlexMultipleStatementsTokens", LexerlexMultipleStatementsTokens },
            { "LexerlexSingleLineCommentTokens", LexerlexSingleLineCommentTokens },
            { "LexerlexMultiLineCommentTokens", LexerlexMultiLineCommentTokens },
            { "LexerlexBraceBlock", LexerlexBraceBlock },
            { "LexerlexTopLevelBraceBlock", LexerlexTopLevelBraceBlock },
            { "LexerlexBraceBlockStmts", LexerlexBraceBlockStmts },
            { "LexerlexBraceBlockOrSingleStmt", LexerlexBraceBlockOrSingleStmt },
            { "LexerlexImportIdentifierList", LexerlexImportIdentifierList },
            { "LexerlexImportStatement", LexerlexImportStatement },
            { "LexerlexDestructStatement", LexerlexDestructStatement },
            { "LexerlexReturnStatement", LexerlexReturnStatement },
            { "LexerlexConstructorInitBlock", LexerlexConstructorInitBlock },
            { "LexerlexUnsafeBlock", LexerlexUnsafeBlock },
            { "LexerlexBreakStatement", LexerlexBreakStatement },
            { "LexerlexTypealiasStatement", LexerlexTypealiasStatement },
            { "LexerlexContinueStatement", LexerlexContinueStatement },
            { "LexerlexIfExprAndBlock", LexerlexIfExprAndBlock },
            { "LexerlexIfBlockTokens", LexerlexIfBlockTokens },
            { "LexerlexDoWhileBlockTokens", LexerlexDoWhileBlockTokens },
            { "LexerlexWhileBlockTokens", LexerlexWhileBlockTokens },
            { "LexerlexForBlockTokens", LexerlexForBlockTokens },
            { "LexerlexLoopBlockTokens", LexerlexLoopBlockTokens },
            { "LexerlexParameterList", LexerlexParameterList },
            { "LexerlexFunctionSignatureTokens", LexerlexFunctionSignatureTokens },
            { "LexerlexGenericParametersList", LexerlexGenericParametersList },
            { "LexerlexAfterFuncKeyword", LexerlexAfterFuncKeyword },
            { "LexerlexFunctionStructureTokens", LexerlexFunctionStructureTokens },
            { "LexerlexInterfaceBlockTokens", LexerlexInterfaceBlockTokens },
            { "LexerlexInterfaceStructureTokens", LexerlexInterfaceStructureTokens },
            { "LexerlexNamespaceTokens", LexerlexNamespaceTokens },
            { "LexerlexStructMemberTokens", LexerlexStructMemberTokens },
            { "LexerlexStructBlockTokens", LexerlexStructBlockTokens },
            { "LexerlexStructStructureTokens", LexerlexStructStructureTokens },
            { "LexerlexVariantMemberTokens", LexerlexVariantMemberTokens },
            { "LexerlexVariantBlockTokens", LexerlexVariantBlockTokens },
            { "LexerlexVariantStructureTokens", LexerlexVariantStructureTokens },
            { "LexerlexUnionBlockTokens", LexerlexUnionBlockTokens },
            { "LexerlexUnionStructureTokens", LexerlexUnionStructureTokens },
            { "LexerlexImplBlockTokens", LexerlexImplBlockTokens },
            { "LexerlexImplTokens", LexerlexImplTokens },
            { "LexerlexEnumBlockTokens", LexerlexEnumBlockTokens },
            { "LexerlexEnumStructureTokens", LexerlexEnumStructureTokens },
            { "LexerreadWhitespace", LexerreadWhitespace },
            { "LexerlexWhitespaceToken", LexerlexWhitespaceToken },
            { "LexerlexStringToken", LexerlexStringToken },
            { "LexerlexCharToken", LexerlexCharToken },
            { "LexerlexAnnotationMacro", LexerlexAnnotationMacro },
            { "LexerlexNull", LexerlexNull },
            { "LexerlexBoolToken", LexerlexBoolToken },
            { "LexerlexUnsignedIntAsNumberToken", LexerlexUnsignedIntAsNumberToken },
            { "LexerlexNumberToken", LexerlexNumberToken },
            { "LexerlexStructValueTokens", LexerlexStructValueTokens },
            { "LexerlexValueToken", LexerlexValueToken },
            { "LexerlexSwitchCaseValue", LexerlexSwitchCaseValue },
            { "LexerlexAccessChainValueToken", LexerlexAccessChainValueToken },
            { "LexerlexArrayInit", LexerlexArrayInit },
            { "LexerlexAccessChainOrValue", LexerlexAccessChainOrValue },
            { "LexerlexValueNode", LexerlexValueNode },
            { "LexerlexIdentifierList", LexerlexIdentifierList },
            { "LexerlexLambdaAfterParamsList", LexerlexLambdaAfterParamsList },
            { "LexerlexLambdaValue", LexerlexLambdaValue },
            { "LexerlexRemainingExpression", LexerlexRemainingExpression },
            { "LexerlexLambdaOrExprAfterLParen", LexerlexLambdaOrExprAfterLParen },
            { "LexerlexParenExpressionAfterLParen", LexerlexParenExpressionAfterLParen },
            { "LexerlexParenExpression", LexerlexParenExpression },
            { "LexerlexExpressionTokens", LexerlexExpressionTokens },
            { "LexerlexSwitchStatementBlock", LexerlexSwitchStatementBlock },
            { "LexerlexTryCatchTokens", LexerlexTryCatchTokens },
            { "LexerlexUsingStatement", LexerlexUsingStatement },
    };
}

constexpr std::string token_func(const std::string& str) {
    return "CSTToken" + str;
}

void cst_token_symbol_map(std::unordered_map<std::string_view, void*>& sym_map) {
    sym_map = {
            { "CSTTokenget_value", CSTTokenget_value },
            { "CSTTokenposition", CSTTokenposition },
            { "CSTTokentokens", CSTTokentokens },
            { "CSTTokentype", CSTTokentype },
            { "CSTTokenaccept", CSTTokenaccept },
    };
}

void ptr_vec_symbol_map(std::unordered_map<std::string_view, void*>& sym_map) {
    sym_map = {
            { "PtrVec_get", PtrVec_get },
            { "PtrVec_size", PtrVec_size },
            { "PtrVec_set", PtrVec_set },
            { "PtrVec_erase", PtrVec_erase },
            { "PtrVec_push", PtrVec_push },
    };
}

void ast_builder_symbol_map(std::unordered_map<std::string_view, void*>& sym_map) {
    sym_map = {
            { "ASTBuildermake_any_type", ASTBuildermake_any_type },
            { "ASTBuildermake_array_type", ASTBuildermake_array_type },
            { "ASTBuildermake_bigint_type", ASTBuildermake_bigint_type },
            { "ASTBuildermake_bool_type", ASTBuildermake_bool_type },
            { "ASTBuildermake_char_type", ASTBuildermake_char_type },
            { "ASTBuildermake_double_type", ASTBuildermake_double_type },
            { "ASTBuildermake_dynamic_type", ASTBuildermake_dynamic_type },
            { "ASTBuildermake_float_type", ASTBuildermake_float_type },
            { "ASTBuildermake_func_type", ASTBuildermake_func_type },
            { "ASTBuildermake_generic_type", ASTBuildermake_generic_type },
            { "ASTBuildermake_int128_type", ASTBuildermake_int128_type },
            { "ASTBuildermake_int_type", ASTBuildermake_int_type },
            { "ASTBuildermake_linked_type", ASTBuildermake_linked_type },
            { "ASTBuildermake_linked_value_type", ASTBuildermake_linked_value_type },
            { "ASTBuildermake_literal_type", ASTBuildermake_literal_type },
            { "ASTBuildermake_long_type", ASTBuildermake_long_type },
            { "ASTBuildermake_ptr_type", ASTBuildermake_ptr_type },
            { "ASTBuildermake_reference_type", ASTBuildermake_reference_type },
            { "ASTBuildermake_short_type", ASTBuildermake_short_type },
            { "ASTBuildermake_string_type", ASTBuildermake_string_type },
            { "ASTBuildermake_ubigint_type", ASTBuildermake_ubigint_type },
            { "ASTBuildermake_uchar_type", ASTBuildermake_uchar_type },
            { "ASTBuildermake_uint128_type", ASTBuildermake_uint128_type },
            { "ASTBuildermake_uint_type", ASTBuildermake_uint_type },
            { "ASTBuildermake_ulong_type", ASTBuildermake_ulong_type },
            { "ASTBuildermake_ushort_type", ASTBuildermake_ushort_type },
            { "ASTBuildermake_void_type", ASTBuildermake_void_type },
            { "ASTBuildermake_access_chain", ASTBuildermake_access_chain },
            { "ASTBuildermake_addr_of_value", ASTBuildermake_addr_of_value },
            { "ASTBuildermake_array_value", ASTBuildermake_array_value },
            { "ASTBuildermake_bigint_value", ASTBuildermake_bigint_value },
            { "ASTBuildermake_bool_value", ASTBuildermake_bool_value },
            { "ASTBuildermake_casted_value", ASTBuildermake_casted_value },
            { "ASTBuildermake_char_value", ASTBuildermake_char_value },
            { "ASTBuildermake_dereference_value", ASTBuildermake_dereference_value },
            { "ASTBuildermake_double_value", ASTBuildermake_double_value },
            { "ASTBuildermake_expression_value", ASTBuildermake_expression_value },
            { "ASTBuildermake_float_value", ASTBuildermake_float_value },
            { "ASTBuildermake_function_call_value", ASTBuildermake_function_call_value },
            { "ASTBuildermake_index_op_value", ASTBuildermake_index_op_value },
            { "ASTBuildermake_int128_value", ASTBuildermake_int128_value },
            { "ASTBuildermake_int_value", ASTBuildermake_int_value },
            { "ASTBuildermake_is_value", ASTBuildermake_is_value },
            { "ASTBuildermake_lambda_function", ASTBuildermake_lambda_function },
            { "ASTBuildermake_captured_variable", ASTBuildermake_captured_variable },
            { "ASTBuildermake_long_value", ASTBuildermake_long_value },
            { "ASTBuildermake_negative_value", ASTBuildermake_negative_value },
            { "ASTBuildermake_not_value", ASTBuildermake_not_value },
            { "ASTBuildermake_null_value", ASTBuildermake_null_value },
            { "ASTBuildermake_number_value", ASTBuildermake_number_value },
            { "ASTBuildermake_short_value", ASTBuildermake_short_value },
            { "ASTBuildermake_sizeof_value", ASTBuildermake_sizeof_value },
            { "ASTBuildermake_string_value", ASTBuildermake_string_value },
            { "ASTBuildermake_struct_member_initializer", ASTBuildermake_struct_member_initializer },
            { "ASTBuildermake_struct_value", ASTBuildermake_struct_value },
            { "ASTBuildermake_ubigint_value", ASTBuildermake_ubigint_value },
            { "ASTBuildermake_uchar_value", ASTBuildermake_uchar_value },
            { "ASTBuildermake_uint128_value", ASTBuildermake_uint128_value },
            { "ASTBuildermake_uint_value", ASTBuildermake_uint_value },
            { "ASTBuildermake_ulong_value", ASTBuildermake_ulong_value },
            { "ASTBuildermake_ushort_value", ASTBuildermake_ushort_value },
            { "ASTBuildermake_value_node", ASTBuildermake_value_node },
            { "ASTBuildermake_identifier", ASTBuildermake_identifier },
            { "ASTBuildermake_variant_call", ASTBuildermake_variant_call },
            { "ASTBuildermake_variant_case", ASTBuildermake_variant_case },
            { "ASTBuildermake_variant_case_variable", ASTBuildermake_variant_case_variable },
            { "ASTBuildermake_assignment_stmt", ASTBuildermake_assignment_stmt },
            { "ASTBuildermake_break_stmt", ASTBuildermake_break_stmt },
            { "ASTBuildermake_comment_stmt", ASTBuildermake_comment_stmt },
            { "ASTBuildermake_continue_stmt", ASTBuildermake_continue_stmt },
            { "ASTBuildermake_destruct_stmt", ASTBuildermake_destruct_stmt },
            { "ASTBuildermake_return_stmt", ASTBuildermake_return_stmt },
            { "ASTBuildermake_typealias_stmt", ASTBuildermake_typealias_stmt },
            { "ASTBuildermake_using_stmt", ASTBuildermake_using_stmt },
            { "ASTBuildermake_varinit_stmt", ASTBuildermake_varinit_stmt },
            { "ASTBuildermake_scope", ASTBuildermake_scope },
            { "ASTBuildermake_do_while_loop", ASTBuildermake_do_while_loop },
            { "ASTBuildermake_enum_decl", ASTBuildermake_enum_decl },
            { "ASTBuildermake_enum_member", ASTBuildermake_enum_member },
            { "ASTBuildermake_for_loop", ASTBuildermake_for_loop },
            { "ASTBuildermake_function", ASTBuildermake_function },
            { "ASTBuildermake_function_param", ASTBuildermake_function_param },
            { "ASTBuildermake_generic_param", ASTBuildermake_generic_param },
            { "ASTBuildermake_if_stmt", ASTBuildermake_if_stmt },
            { "ASTBuildermake_impl_def", ASTBuildermake_impl_def },
            { "ASTBuildermake_init_block", ASTBuildermake_init_block },
            { "ASTBuildermake_interface_def", ASTBuildermake_interface_def },
            { "ASTBuildermake_namespace", ASTBuildermake_namespace },
            { "ASTBuildermake_struct_def", ASTBuildermake_struct_def },
            { "ASTBuildermake_struct_member", ASTBuildermake_struct_member },
            { "ASTBuildermake_union_def", ASTBuildermake_union_def },
            { "ASTBuildermake_unsafe_block", ASTBuildermake_unsafe_block },
            { "ASTBuildermake_while_loop", ASTBuildermake_while_loop },
            { "ASTBuildermake_variant_def", ASTBuildermake_variant_def },
            { "ASTBuildermake_variant_member", ASTBuildermake_variant_member },
            { "ASTBuildermake_variant_member_param", ASTBuildermake_variant_member_param },

            // you only need to mark ASTBuilder @compiler:interface to get all nodes' functions
            { "FunctionTypeget_params", FunctionTypeget_params },
            { "GenericTypeget_types", GenericTypeget_types },
            { "AccessChainget_values", AccessChainget_values },
            { "ArrayValueget_values", ArrayValueget_values },
            { "ArrayValueadd_size", ArrayValueadd_size },
            { "FunctionCallget_args", FunctionCallget_args },
            { "IndexOperatorget_values", IndexOperatorget_values },
            { "LambdaFunctionget_params", LambdaFunctionget_params },
            { "LambdaFunctionget_capture_list", LambdaFunctionget_capture_list },
            { "LambdaFunctionget_body", LambdaFunctionget_body },
            { "StructValueadd_value", StructValueadd_value },
            { "VariantCaseadd_variable", VariantCaseadd_variable },
            { "DoWhileLoopget_body", DoWhileLoopget_body },
            { "WhileLoopget_body", WhileLoopget_body },
            { "ForLoopget_body", ForLoopget_body },
            { "EnumDeclarationadd_member", EnumDeclarationadd_member },
            { "FunctionDeclarationget_params", FunctionDeclarationget_params },
            { "FunctionDeclarationget_generic_params", FunctionDeclarationget_generic_params },
            { "FunctionDeclarationadd_body", FunctionDeclarationadd_body },
            { "IfStatementget_body", IfStatementget_body },
            { "IfStatementadd_else_body", IfStatementadd_else_body },
            { "IfStatementadd_else_if", IfStatementadd_else_if },
            { "ImplDefinitionadd_function", ImplDefinitionadd_function },
            { "StructDefinitionadd_member", StructDefinitionadd_member },
            { "StructDefinitionadd_function", StructDefinitionadd_function },
            { "StructDefinitionget_generic_params", StructDefinitionget_generic_params },
            { "InterfaceDefinitionadd_function", InterfaceDefinitionadd_function },
            { "Namespaceget_children", Namespaceget_body },
            { "UnsafeBlockget_children", UnsafeBlockget_body },
            { "UnionDefinitionadd_member", UnionDefinitionadd_member },
            { "UnionDefinitionadd_function", UnionDefinitionadd_function },
            { "UnionDefinitionget_generic_params", UnionDefinitionget_generic_params },
            { "VariantDefinitionadd_member", VariantDefinitionadd_member },
            { "VariantMemberadd_param", VariantMemberadd_param },
            { "InitBlockadd_initializer", InitBlockadd_initializer },


    };
}

void cst_converter_symbol_map(std::unordered_map<std::string_view, void*>& sym_map) {
    sym_map = {
            { "CSTConverterpop_last_node", CSTConverterpop_last_node },
            { "CSTConverterpop_last_type", CSTConverterpop_last_type },
            { "CSTConverterpop_last_value", CSTConverterpop_last_value },
            { "CSTConverterput_node", CSTConverterput_node },
            { "CSTConverterput_value", CSTConverterput_value },
            { "CSTConverterput_type", CSTConverterput_type },
            { "CSTConvertervisit", CSTConvertervisit }
    };
}