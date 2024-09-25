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

constexpr std::string bc_func(const std::string& name) {
    return "BuildContext" + name;
}

void build_context_symbol_map(std::unordered_map<std::string, void*>& sym_map) {
    sym_map = {
        {bc_func("files_module"), BuildContextfiles_module },
        {bc_func("chemical_files_module"), BuildContextchemical_files_module },
        {bc_func("chemical_dir_module"), BuildContextchemical_dir_module },
        {bc_func("c_file_module"), BuildContextc_file_module },
        {bc_func("cpp_file_module"), BuildContextcpp_file_module },
        {bc_func("object_module"), BuildContextobject_module },
        {bc_func("translate_to_chemical"), BuildContexttranslate_to_chemical },
        {bc_func("translate_to_c"), BuildContexttranslate_to_c },
        {bc_func("build_exe"), BuildContextbuild_exe },
        {bc_func("build_dynamic_lib"), BuildContextbuild_dynamic_lib },
        {bc_func("build_cbi"), BuildContextbuild_cbi },
        {bc_func("add_object"), BuildContextadd_object },
        {bc_func("declare_alias"), BuildContextdeclare_alias },
        {bc_func("build_path"), BuildContextbuild_path },
        {bc_func("has_arg"), BuildContexthas_arg },
        {bc_func("get_arg"), BuildContextget_arg },
        {bc_func("remove_arg"), BuildContextremove_arg },
        {bc_func("define"), BuildContextdefine },
        {bc_func("undefine"), BuildContextundefine },
        {bc_func("launch_executable"), BuildContextlaunch_executable },
        {bc_func("on_finished"), BuildContexton_finished },
        {bc_func("link_objects"), BuildContextlink_objects },
        {bc_func("invoke_dlltool"), BuildContextinvoke_dlltool },
        {bc_func("invoke_ranlib"), BuildContextinvoke_ranlib },
        {bc_func("invoke_lib"), BuildContextinvoke_lib },
        {bc_func("invoke_ar"), BuildContextinvoke_ar }
    };
}

constexpr std::string sp_func(const std::string& string) {
    return "SourceProvider" + string;
}

void source_provider_symbol_map(std::unordered_map<std::string, void*>& sym_map) {
    sym_map = {
        {sp_func("readCharacter"), SourceProviderreadCharacter },
        {sp_func("eof"), SourceProvidereof },
        {sp_func("peek"), SourceProviderpeek },
        {sp_func("readUntil"), SourceProviderreadUntil },
        {sp_func("increment"), SourceProviderincrement },
        {sp_func("increment_char"), SourceProviderincrement_char },
        {sp_func("getLineNumber"), SourceProvidergetLineNumber },
        {sp_func("getLineCharNumber"), SourceProvidergetLineCharNumber },
        {sp_func("readEscaping"), SourceProviderreadEscaping },
        {sp_func("readAnything"), SourceProviderreadAnything },
        {sp_func("readAlpha"), SourceProviderreadAlpha },
        {sp_func("readUnsignedInt"), SourceProviderreadUnsignedInt },
        {sp_func("readNumber"), SourceProviderreadNumber },
        {sp_func("readAlphaNum"), SourceProviderreadAlphaNum },
        {sp_func("readIdentifier"), SourceProviderreadIdentifier },
        {sp_func("readAnnotationIdentifier"), SourceProviderreadAnnotationIdentifier },
        {sp_func("readWhitespaces"), SourceProviderreadWhitespaces },
        {sp_func("hasNewLine"), SourceProviderhasNewLine },
        {sp_func("readNewLineChars"), SourceProviderreadNewLineChars },
        {sp_func("readWhitespacesAndNewLines"), SourceProviderreadWhitespacesAndNewLines },

    };
}

constexpr std::string lexer_func(const std::string& name) {
    return "Lexer" + name;
};

void lexer_symbol_map(std::unordered_map<std::string, void*>& sym_map) {
    sym_map = {
            {lexer_func("tokens_size"), Lexertokens_size },
            {lexer_func("storeVariable"), LexerstoreVariable },
            {lexer_func("storeIdentifier"), LexerstoreIdentifier },
            {lexer_func("lexGenericArgsList"), LexerlexGenericArgsList },
            {lexer_func("lexGenericArgsListCompound"), LexerlexGenericArgsListCompound },
            {lexer_func("lexFunctionCallWithGenericArgsList"), LexerlexFunctionCallWithGenericArgsList },
            {lexer_func("lexFunctionCall"), LexerlexFunctionCall },
            {lexer_func("lexAccessSpecifier"), LexerlexAccessSpecifier },
            {lexer_func("lexAccessChainAfterId"), LexerlexAccessChainAfterId },
            {lexer_func("lexAccessChainRecursive"), LexerlexAccessChainRecursive },
            {lexer_func("lexAccessChain"), LexerlexAccessChain },
            {lexer_func("lexAccessChainOrAddrOf"), LexerlexAccessChainOrAddrOf },
            {lexer_func("lexVarInitializationTokens"), LexerlexVarInitializationTokens },
            {lexer_func("lexAssignmentTokens"), LexerlexAssignmentTokens },
            {lexer_func("lexDivisionOperatorToken"), LexerlexDivisionOperatorToken },
            {lexer_func("lexLanguageOperatorToken"), LexerlexLanguageOperatorToken },
            {lexer_func("isGenericEndAhead"), LexerisGenericEndAhead },
            {lexer_func("lexAssignmentOperatorToken"), LexerlexAssignmentOperatorToken },
            {lexer_func("lexLambdaTypeTokens"), LexerlexLambdaTypeTokens },
            {lexer_func("lexGenericTypeAfterId"), LexerlexGenericTypeAfterId },
            {lexer_func("lexRefOrGenericType"), LexerlexRefOrGenericType },
            {lexer_func("lexArrayAndPointerTypesAfterTypeId"), LexerlexArrayAndPointerTypesAfterTypeId },
            {lexer_func("lexTypeTokens"), LexerlexTypeTokens },
            {lexer_func("lexTopLevelAccessSpecifiedDecls"), LexerlexTopLevelAccessSpecifiedDecls },
            {lexer_func("lexTopLevelStatementTokens"), LexerlexTopLevelStatementTokens },
            {lexer_func("lexNestedLevelStatementTokens"), LexerlexNestedLevelStatementTokens },
            {lexer_func("lexStatementTokens"), LexerlexStatementTokens },
            {lexer_func("lexThrowStatementTokens"), LexerlexThrowStatementTokens },
            {lexer_func("lexOperatorToken"), LexerlexOperatorToken },
            {lexer_func("lexOperatorTokenStr"), LexerlexOperatorTokenStr },
            {lexer_func("storeOperationToken"), LexerstoreOperationToken },
            {lexer_func("lexOperationToken"), LexerlexOperationToken },
            {lexer_func("lexOperatorTokenStr2"), LexerlexOperatorTokenStr2 },
            {lexer_func("lexKeywordToken"), LexerlexKeywordToken },
            {lexer_func("lexWSKeywordToken"), LexerlexWSKeywordToken },
            {lexer_func("lexWSKeywordToken2"), LexerlexWSKeywordToken2 },
            {lexer_func("lexTopLevelMultipleStatementsTokens"), LexerlexTopLevelMultipleStatementsTokens },
            {lexer_func("lexTopLevelMultipleImportStatements"), LexerlexTopLevelMultipleImportStatements },
            {lexer_func("lexNestedLevelMultipleStatementsTokens"), LexerlexNestedLevelMultipleStatementsTokens },
            {lexer_func("lexMultipleStatementsTokens"), LexerlexMultipleStatementsTokens },
            {lexer_func("lexSingleLineCommentTokens"), LexerlexSingleLineCommentTokens },
            {lexer_func("lexMultiLineCommentTokens"), LexerlexMultiLineCommentTokens },
            {lexer_func("lexBraceBlock"), LexerlexBraceBlock },
            {lexer_func("lexTopLevelBraceBlock"), LexerlexTopLevelBraceBlock },
            {lexer_func("lexBraceBlockStmts"), LexerlexBraceBlockStmts },
            {lexer_func("lexBraceBlockOrSingleStmt"), LexerlexBraceBlockOrSingleStmt },
            {lexer_func("lexImportIdentifierList"), LexerlexImportIdentifierList },
            {lexer_func("lexImportStatement"), LexerlexImportStatement },
            {lexer_func("lexDestructStatement"), LexerlexDestructStatement },
            {lexer_func("lexReturnStatement"), LexerlexReturnStatement },
            {lexer_func("lexConstructorInitBlock"), LexerlexConstructorInitBlock },
            {lexer_func("lexUnsafeBlock"), LexerlexUnsafeBlock },
            {lexer_func("lexBreakStatement"), LexerlexBreakStatement },
            {lexer_func("lexTypealiasStatement"), LexerlexTypealiasStatement },
            {lexer_func("lexContinueStatement"), LexerlexContinueStatement },
            {lexer_func("lexIfExprAndBlock"), LexerlexIfExprAndBlock },
            {lexer_func("lexIfBlockTokens"), LexerlexIfBlockTokens },
            {lexer_func("lexDoWhileBlockTokens"), LexerlexDoWhileBlockTokens },
            {lexer_func("lexWhileBlockTokens"), LexerlexWhileBlockTokens },
            {lexer_func("lexForBlockTokens"), LexerlexForBlockTokens },
            {lexer_func("lexLoopBlockTokens"), LexerlexLoopBlockTokens },
            {lexer_func("lexParameterList"), LexerlexParameterList },
            {lexer_func("lexFunctionSignatureTokens"), LexerlexFunctionSignatureTokens },
            {lexer_func("lexGenericParametersList"), LexerlexGenericParametersList },
            {lexer_func("lexAfterFuncKeyword"), LexerlexAfterFuncKeyword },
            {lexer_func("lexFunctionStructureTokens"), LexerlexFunctionStructureTokens },
            {lexer_func("lexInterfaceBlockTokens"), LexerlexInterfaceBlockTokens },
            {lexer_func("lexInterfaceStructureTokens"), LexerlexInterfaceStructureTokens },
            {lexer_func("lexNamespaceTokens"), LexerlexNamespaceTokens },
            {lexer_func("lexStructMemberTokens"), LexerlexStructMemberTokens },
            {lexer_func("lexStructBlockTokens"), LexerlexStructBlockTokens },
            {lexer_func("lexStructStructureTokens"), LexerlexStructStructureTokens },
            {lexer_func("lexVariantMemberTokens"), LexerlexVariantMemberTokens },
            {lexer_func("lexVariantBlockTokens"), LexerlexVariantBlockTokens },
            {lexer_func("lexVariantStructureTokens"), LexerlexVariantStructureTokens },
            {lexer_func("lexUnionBlockTokens"), LexerlexUnionBlockTokens },
            {lexer_func("lexUnionStructureTokens"), LexerlexUnionStructureTokens },
            {lexer_func("lexImplBlockTokens"), LexerlexImplBlockTokens },
            {lexer_func("lexImplTokens"), LexerlexImplTokens },
            {lexer_func("lexEnumBlockTokens"), LexerlexEnumBlockTokens },
            {lexer_func("lexEnumStructureTokens"), LexerlexEnumStructureTokens },
            {lexer_func("readWhitespace"), LexerreadWhitespace },
            {lexer_func("lexWhitespaceToken"), LexerlexWhitespaceToken },
            {lexer_func("lexStringToken"), LexerlexStringToken },
            {lexer_func("lexCharToken"), LexerlexCharToken },
            {lexer_func("lexAnnotationMacro"), LexerlexAnnotationMacro },
            {lexer_func("lexNull"), LexerlexNull },
            {lexer_func("lexBoolToken"), LexerlexBoolToken },
            {lexer_func("lexUnsignedIntAsNumberToken"), LexerlexUnsignedIntAsNumberToken },
            {lexer_func("lexNumberToken"), LexerlexNumberToken },
            {lexer_func("lexStructValueTokens"), LexerlexStructValueTokens },
            {lexer_func("lexValueToken"), LexerlexValueToken },
            {lexer_func("lexSwitchCaseValue"), LexerlexSwitchCaseValue },
            {lexer_func("lexAccessChainValueToken"), LexerlexAccessChainValueToken },
            {lexer_func("lexArrayInit"), LexerlexArrayInit },
            {lexer_func("lexAccessChainOrValue"), LexerlexAccessChainOrValue },
            {lexer_func("lexValueNode"), LexerlexValueNode },
            {lexer_func("lexIdentifierList"), LexerlexIdentifierList },
            {lexer_func("lexLambdaAfterParamsList"), LexerlexLambdaAfterParamsList },
            {lexer_func("lexLambdaValue"), LexerlexLambdaValue },
            {lexer_func("lexRemainingExpression"), LexerlexRemainingExpression },
            {lexer_func("lexLambdaOrExprAfterLParen"), LexerlexLambdaOrExprAfterLParen },
            {lexer_func("lexParenExpressionAfterLParen"), LexerlexParenExpressionAfterLParen },
            {lexer_func("lexParenExpression"), LexerlexParenExpression },
            {lexer_func("lexExpressionTokens"), LexerlexExpressionTokens },
            {lexer_func("lexSwitchStatementBlock"), LexerlexSwitchStatementBlock },
            {lexer_func("lexTryCatchTokens"), LexerlexTryCatchTokens },
            {lexer_func("lexUsingStatement"), LexerlexUsingStatement },
    };
}

constexpr std::string token_func(const std::string& str) {
    return "CSTToken" + str;
}

void cst_token_symbol_map(std::unordered_map<std::string, void*>& sym_map) {
    sym_map = {
            { token_func("get_value"), CSTTokenget_value },
            { token_func("position"), CSTTokenposition },
            { token_func("tokens"), CSTTokentokens },
            { token_func("type"), CSTTokentype },
            { token_func("accept"), CSTTokenaccept },
    };
}

constexpr std::string tokens_vec_fn(const std::string& str) {
    return "TokensVec" + str;
}

void ptr_vec_symbol_map(std::unordered_map<std::string, void*>& sym_map) {
    sym_map = {
            { tokens_vec_fn("_get"), PtrVec_get },
            { tokens_vec_fn("_size"), PtrVec_size },
            { tokens_vec_fn("_set"), PtrVec_set },
            { tokens_vec_fn("_erase"), PtrVec_erase },
            { tokens_vec_fn("_push"), PtrVec_push },
    };
}

constexpr std::string builder_fn(const std::string& str) {
    return "ASTBuilder" + str;
}

void ast_builder_symbol_map(std::unordered_map<std::string, void*>& sym_map) {
    sym_map = {
            { builder_fn("make_any_type"), ASTBuildermake_any_type },
            { builder_fn("make_array_type"), ASTBuildermake_array_type },
            { builder_fn("make_bigint_type"), ASTBuildermake_bigint_type },
            { builder_fn("make_bool_type"), ASTBuildermake_bool_type },
            { builder_fn("make_char_type"), ASTBuildermake_char_type },
            { builder_fn("make_double_type"), ASTBuildermake_double_type },
            { builder_fn("make_dynamic_type"), ASTBuildermake_dynamic_type },
            { builder_fn("make_float_type"), ASTBuildermake_float_type },
            { builder_fn("make_func_type"), ASTBuildermake_func_type },
            { builder_fn("make_generic_type"), ASTBuildermake_generic_type },
            { builder_fn("make_int128_type"), ASTBuildermake_int128_type },
            { builder_fn("make_int_type"), ASTBuildermake_int_type },
            { builder_fn("make_linked_type"), ASTBuildermake_linked_type },
            { builder_fn("make_linked_value_type"), ASTBuildermake_linked_value_type },
            { builder_fn("make_literal_type"), ASTBuildermake_literal_type },
            { builder_fn("make_long_type"), ASTBuildermake_long_type },
            { builder_fn("make_ptr_type"), ASTBuildermake_ptr_type },
            { builder_fn("make_reference_type"), ASTBuildermake_reference_type },
            { builder_fn("make_short_type"), ASTBuildermake_short_type },
            { builder_fn("make_string_type"), ASTBuildermake_string_type },
            { builder_fn("make_ubigint_type"), ASTBuildermake_ubigint_type },
            { builder_fn("make_uchar_type"), ASTBuildermake_uchar_type },
            { builder_fn("make_uint128_type"), ASTBuildermake_uint128_type },
            { builder_fn("make_uint_type"), ASTBuildermake_uint_type },
            { builder_fn("make_ulong_type"), ASTBuildermake_ulong_type },
            { builder_fn("make_ushort_type"), ASTBuildermake_ushort_type },
            { builder_fn("make_void_type"), ASTBuildermake_void_type },
            { builder_fn("make_access_chain"), ASTBuildermake_access_chain },
            { builder_fn("make_addr_of_value"), ASTBuildermake_addr_of_value },
            { builder_fn("make_array_value"), ASTBuildermake_array_value },
            { builder_fn("make_bigint_value"), ASTBuildermake_bigint_value },
            { builder_fn("make_bool_value"), ASTBuildermake_bool_value },
            { builder_fn("make_casted_value"), ASTBuildermake_casted_value },
            { builder_fn("make_char_value"), ASTBuildermake_char_value },
            { builder_fn("make_dereference_value"), ASTBuildermake_dereference_value },
            { builder_fn("make_double_value"), ASTBuildermake_double_value },
            { builder_fn("make_expression_value"), ASTBuildermake_expression_value },
            { builder_fn("make_float_value"), ASTBuildermake_float_value },
            { builder_fn("make_function_call_value"), ASTBuildermake_function_call_value },
            { builder_fn("make_index_op_value"), ASTBuildermake_index_op_value },
            { builder_fn("make_int128_value"), ASTBuildermake_int128_value },
            { builder_fn("make_int_value"), ASTBuildermake_int_value },
            { builder_fn("make_is_value"), ASTBuildermake_is_value },
            { builder_fn("make_lambda_function"), ASTBuildermake_lambda_function },
            { builder_fn("make_captured_variable"), ASTBuildermake_captured_variable },
            { builder_fn("make_long_value"), ASTBuildermake_long_value },
            { builder_fn("make_negative_value"), ASTBuildermake_negative_value },
            { builder_fn("make_not_value"), ASTBuildermake_not_value },
            { builder_fn("make_null_value"), ASTBuildermake_null_value },
            { builder_fn("make_number_value"), ASTBuildermake_number_value },
            { builder_fn("make_short_value"), ASTBuildermake_short_value },
            { builder_fn("make_sizeof_value"), ASTBuildermake_sizeof_value },
            { builder_fn("make_string_value"), ASTBuildermake_string_value },
            { builder_fn("make_struct_member_initializer"), ASTBuildermake_struct_member_initializer },
            { builder_fn("make_struct_struct_value"), ASTBuildermake_struct_struct_value },
            { builder_fn("make_ubigint_value"), ASTBuildermake_ubigint_value },
            { builder_fn("make_uchar_value"), ASTBuildermake_uchar_value },
            { builder_fn("make_uint128_value"), ASTBuildermake_uint128_value },
            { builder_fn("make_uint_value"), ASTBuildermake_uint_value },
            { builder_fn("make_ulong_value"), ASTBuildermake_ulong_value },
            { builder_fn("make_ushort_value"), ASTBuildermake_ushort_value },
            { builder_fn("make_value_node"), ASTBuildermake_value_node },
            { builder_fn("make_identifier"), ASTBuildermake_identifier },
            { builder_fn("make_variant_call"), ASTBuildermake_variant_call },
            { builder_fn("make_variant_case"), ASTBuildermake_variant_case },
            { builder_fn("make_variant_case_variable"), ASTBuildermake_variant_case_variable },
            { builder_fn("make_assignment_stmt"), ASTBuildermake_assignment_stmt },
            { builder_fn("make_break_stmt"), ASTBuildermake_break_stmt },
            { builder_fn("make_comment_stmt"), ASTBuildermake_comment_stmt },
            { builder_fn("make_continue_stmt"), ASTBuildermake_continue_stmt },
            { builder_fn("make_destruct_stmt"), ASTBuildermake_destruct_stmt },
            { builder_fn("make_return_stmt"), ASTBuildermake_return_stmt },
            { builder_fn("make_typealias_stmt"), ASTBuildermake_typealias_stmt },
            { builder_fn("make_using_stmt"), ASTBuildermake_using_stmt },
            { builder_fn("make_varinit_stmt"), ASTBuildermake_varinit_stmt },
            { builder_fn("make_scope"), ASTBuildermake_scope },
            { builder_fn("make_do_while_loop"), ASTBuildermake_do_while_loop },
            { builder_fn("make_enum_decl"), ASTBuildermake_enum_decl },
            { builder_fn("make_enum_member"), ASTBuildermake_enum_member },
            { builder_fn("make_for_loop"), ASTBuildermake_for_loop },
            { builder_fn("make_function"), ASTBuildermake_function },
            { builder_fn("make_function_param"), ASTBuildermake_function_param },
            { builder_fn("make_generic_param"), ASTBuildermake_generic_param },
            { builder_fn("make_if_stmt"), ASTBuildermake_if_stmt },
            { builder_fn("make_impl_def"), ASTBuildermake_impl_def },
            { builder_fn("make_init_block"), ASTBuildermake_init_block },
            { builder_fn("make_interface_def"), ASTBuildermake_interface_def },
            { builder_fn("make_namespace"), ASTBuildermake_namespace },
            { builder_fn("make_struct_def"), ASTBuildermake_struct_def },
            { builder_fn("make_struct_member"), ASTBuildermake_struct_member },
            { builder_fn("make_union_def"), ASTBuildermake_union_def },
            { builder_fn("make_unsafe_block"), ASTBuildermake_unsafe_block },
            { builder_fn("make_while_loop"), ASTBuildermake_while_loop },
            { builder_fn("make_variant_def"), ASTBuildermake_variant_def },
            { builder_fn("make_variant_member"), ASTBuildermake_variant_member },
            { builder_fn("make_variant_member_param"), ASTBuildermake_variant_member_param },

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
            { "StructValueget_generic_list", StructValueget_generic_list },
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

constexpr std::string converter_fn(const std::string& str) {
    return "CSTConverter" + str;
}

void cst_converter_symbol_map(std::unordered_map<std::string, void*>& sym_map) {
    sym_map = {
            { converter_fn("pop_last_node"), CSTConverterpop_last_node },
            { converter_fn("pop_last_type"), CSTConverterpop_last_type },
            { converter_fn("pop_last_value"), CSTConverterpop_last_value },
            { converter_fn("put_node"), CSTConverterput_node },
            { converter_fn("put_value"), CSTConverterput_value },
            { converter_fn("put_type"), CSTConverterput_type },
            { converter_fn("visit"), CSTConvertervisit }
    };
}