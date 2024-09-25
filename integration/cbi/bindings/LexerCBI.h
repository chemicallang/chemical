// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/utils/Operation.h"
#include "lexer/model/LexTokenType.h"
#include "CBIUtils.h"

class Lexer;

extern "C" {

    std::size_t Lexertokens_size(Lexer* lexer);

    void Lexerput(Lexer* lexer, chem::string* value, LexTokenType token_type, unsigned int lineNumber, unsigned int lineCharNumber);

    bool LexerstoreVariable(Lexer* lexer, chem::string* str);

    bool LexerstoreIdentifier(Lexer* lexer, chem::string* str);

    void LexerlexGenericArgsList(Lexer* lexer);

    bool LexerlexGenericArgsListCompound(Lexer* lexer);

    void LexerlexFunctionCallWithGenericArgsList(Lexer* lexer);

    bool LexerlexFunctionCall(Lexer* lexer, unsigned int back_start);

    bool LexerlexAccessSpecifier(Lexer* lexer, bool internal, bool protect);

    bool LexerlexAccessChainAfterId(Lexer* lexer, bool lexStruct, unsigned int chain_length);

    bool LexerlexAccessChainRecursive(Lexer* lexer, bool lexStruct, unsigned int chain_length);

    bool LexerlexAccessChain(Lexer* lexer, bool lexStruct, bool lex_as_node);

    bool LexerlexAccessChainOrAddrOf(Lexer* lexer, bool lexStruct);

    bool LexerlexVarInitializationTokens(Lexer* lexer, unsigned start, bool allowDeclarations, bool requiredType);

    bool LexerlexAssignmentTokens(Lexer* lexer);

    bool LexerlexDivisionOperatorToken(Lexer* lexer);

    bool LexerlexLanguageOperatorToken(Lexer* lexer);

    bool LexerisGenericEndAhead(Lexer* lexer);

    bool LexerlexAssignmentOperatorToken(Lexer* lexer);

    bool LexerlexLambdaTypeTokens(Lexer* lexer, unsigned int start);

    bool LexerlexGenericTypeAfterId(Lexer* lexer, unsigned int start);

    bool LexerlexRefOrGenericType(Lexer* lexer);

    void LexerlexArrayAndPointerTypesAfterTypeId(Lexer* lexer, unsigned int start);

    bool LexerlexTypeTokens(Lexer* lexer);

    bool LexerlexTopLevelAccessSpecifiedDecls(Lexer* lexer);

    bool LexerlexTopLevelStatementTokens(Lexer* lexer);

    bool LexerlexNestedLevelStatementTokens(Lexer* lexer, bool is_value, bool lex_value_node);

    bool LexerlexStatementTokens(Lexer* lexer);

    bool LexerlexThrowStatementTokens(Lexer* lexer);

    bool LexerlexOperatorToken(Lexer* lexer, char op);

    bool LexerlexOperatorTokenStr(Lexer* lexer, chem::string* str);

    void LexerstoreOperationToken(Lexer* lexer, char token, Operation op);

    bool LexerlexOperationToken(Lexer* lexer, char token, Operation op);

    bool LexerlexOperatorTokenStr2(Lexer* lexer, chem::string* str, Operation op);

    bool LexerlexKeywordToken(Lexer* lexer, chem::string* str);

    bool LexerlexWSKeywordToken(Lexer* lexer, chem::string* str);

    bool LexerlexWSKeywordToken2(Lexer* lexer, chem::string* str, char may_end_at);

    void LexerlexTopLevelMultipleStatementsTokens(Lexer* lexer, bool break_at_no_stmt);

    void LexerlexTopLevelMultipleImportStatements(Lexer* lexer);

    void LexerlexNestedLevelMultipleStatementsTokens(Lexer* lexer, bool is_value, bool lex_value_node);

    void LexerlexMultipleStatementsTokens(Lexer* lexer);

    bool LexerlexSingleLineCommentTokens(Lexer* lexer);

    bool LexerlexMultiLineCommentTokens(Lexer* lexer);

    bool LexerlexBraceBlock(Lexer* lexer, void(* nested_lexer)(Lexer*));

    bool LexerlexTopLevelBraceBlock(Lexer* lexer);

    bool LexerlexBraceBlockStmts(Lexer* lexer);

    bool LexerlexBraceBlockOrSingleStmt(Lexer* lexer, bool is_value, bool lex_value_node);

    bool LexerlexImportIdentifierList(Lexer* lexer);

    bool LexerlexImportStatement(Lexer* lexer);

    bool LexerlexDestructStatement(Lexer* lexer);

    bool LexerlexReturnStatement(Lexer* lexer);

    bool LexerlexConstructorInitBlock(Lexer* lexer);

    bool LexerlexUnsafeBlock(Lexer* lexer);

    bool LexerlexBreakStatement(Lexer* lexer);

    bool LexerlexTypealiasStatement(Lexer* lexer, unsigned start);

    bool LexerlexContinueStatement(Lexer* lexer);

    void LexerlexIfExprAndBlock(Lexer* lexer, bool is_value, bool lex_value_node, bool top_level);

    bool LexerlexIfBlockTokens(Lexer* lexer, bool is_value, bool lex_value_node, bool top_level);

    bool LexerlexDoWhileBlockTokens(Lexer* lexer);

    bool LexerlexWhileBlockTokens(Lexer* lexer);

    bool LexerlexForBlockTokens(Lexer* lexer);

    bool LexerlexLoopBlockTokens(Lexer* lexer, bool is_value);

    void LexerlexParameterList(Lexer* lexer, bool optionalTypes, bool defValues, bool lexSelfParam, bool variadicParam);

    bool LexerlexFunctionSignatureTokens(Lexer* lexer);

    bool LexerlexGenericParametersList(Lexer* lexer);

    bool LexerlexAfterFuncKeyword(Lexer* lexer, bool allow_extensions);

    bool LexerlexFunctionStructureTokens(Lexer* lexer, unsigned start, bool allow_declaration, bool allow_extensions);

    void LexerlexInterfaceBlockTokens(Lexer* lexer);

    bool LexerlexInterfaceStructureTokens(Lexer* lexer, unsigned start);

    bool LexerlexNamespaceTokens(Lexer* lexer, unsigned start);

    bool LexerlexStructMemberTokens(Lexer* lexer);

    void LexerlexStructBlockTokens(Lexer* lexer);

    bool LexerlexStructStructureTokens(Lexer* lexer, unsigned start, bool unnamed, bool direct_init);

    bool LexerlexVariantMemberTokens(Lexer* lexer);

    void LexerlexVariantBlockTokens(Lexer* lexer);

    bool LexerlexVariantStructureTokens(Lexer* lexer, unsigned start);

    void LexerlexUnionBlockTokens(Lexer* lexer);

    bool LexerlexUnionStructureTokens(Lexer* lexer, unsigned start, bool unnamed, bool direct_init);

    void LexerlexImplBlockTokens(Lexer* lexer);

    bool LexerlexImplTokens(Lexer* lexer);

    bool LexerlexEnumBlockTokens(Lexer* lexer);

    bool LexerlexEnumStructureTokens(Lexer* lexer, unsigned start);

    bool LexerreadWhitespace(Lexer* lexer);

    bool LexerlexWhitespaceToken(Lexer* lexer);

    bool LexerlexStringToken(Lexer* lexer);

    bool LexerlexCharToken(Lexer* lexer);

    bool LexerlexAnnotationMacro(Lexer* lexer);

    bool LexerlexNull(Lexer* lexer);

    bool LexerlexBoolToken(Lexer* lexer);

    bool LexerlexUnsignedIntAsNumberToken(Lexer* lexer);

    bool LexerlexNumberToken(Lexer* lexer);

    bool LexerlexStructValueTokens(Lexer* lexer, unsigned back_start);

    bool LexerlexValueToken(Lexer* lexer);

    bool LexerlexSwitchCaseValue(Lexer* lexer);

    bool LexerlexAccessChainValueToken(Lexer* lexer);

    bool LexerlexArrayInit(Lexer* lexer);

    bool LexerlexAccessChainOrValue(Lexer* lexer, bool lexStruct);

    bool LexerlexValueNode(Lexer* lexer);

    void LexerlexIdentifierList(Lexer* lexer);

    void LexerlexLambdaAfterParamsList(Lexer* lexer, unsigned int start);

    bool LexerlexLambdaValue(Lexer* lexer);

    bool LexerlexRemainingExpression(Lexer* lexer, unsigned start);

    bool LexerlexLambdaOrExprAfterLParen(Lexer* lexer);

    void LexerlexParenExpressionAfterLParen(Lexer* lexer);

    bool LexerlexParenExpression(Lexer* lexer);

    bool LexerlexExpressionTokens(Lexer* lexer, bool lexStruct, bool lambda);

    bool LexerlexSwitchStatementBlock(Lexer* lexer, bool is_value, bool lex_value_node);

    bool LexerlexTryCatchTokens(Lexer* lexer);

    bool LexerlexUsingStatement(Lexer* lexer);

}