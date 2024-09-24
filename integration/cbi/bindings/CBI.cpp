// Copyright (c) Qinetik 2024.

#include "CBI.h"
#include "SourceProviderCBI.h"
#include "LexerCBI.h"
#include "BuildContextCBI.h"
#include "std/chem_string.h"

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