// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/utils/Operation.h"
#include "parser/model/LexTokenType.h"
#include "CBIUtils.h"
#include "cst/base/CSTToken.h"

class Parser;

class SourceProvider;

extern "C" {

    std::size_t Lexertokens_size(Parser* lexer);

    CSTToken* Lexerput(Parser* lexer, chem::string* value, LexTokenType token_type, unsigned int lineNumber, unsigned int lineCharNumber);

    void LexerlexGenericArgsList(Parser* lexer);

    bool LexerlexGenericArgsListCompound(Parser* lexer);

    void LexerlexFunctionCallWithGenericArgsList(Parser* lexer);

    bool LexerlexFunctionCall(Parser* lexer, unsigned int back_start);

    bool LexerlexAccessSpecifier(Parser* lexer, bool internal, bool protect);

    bool LexerlexAccessChainAfterId(Parser* lexer, bool lexStruct, unsigned int chain_length);

    bool LexerlexAccessChainRecursive(Parser* lexer, bool lexStruct, unsigned int chain_length);

    bool LexerlexAccessChain(Parser* lexer, bool lexStruct, bool lex_as_node);

    bool LexerlexAccessChainOrAddrOf(Parser* lexer, bool lexStruct);

    bool LexerlexVarInitializationTokens(Parser* lexer, unsigned start, bool allowDeclarations, bool requiredType);

    bool LexerlexAssignmentTokens(Parser* lexer);

    bool LexerlexLanguageOperatorToken(Parser* lexer);

    bool LexerisGenericEndAhead(Parser* lexer);

    bool LexerlexAssignmentOperatorToken(Parser* lexer);

    bool LexerlexLambdaTypeTokens(Parser* lexer, unsigned int start);

    bool LexerlexGenericTypeAfterId(Parser* lexer, unsigned int start);

    bool LexerlexRefOrGenericType(Parser* lexer);

    void LexerlexArrayAndPointerTypesAfterTypeId(Parser* lexer, unsigned int start);

    bool LexerlexTypeTokens(Parser* lexer);

    bool LexerlexTopLevelAccessSpecifiedDecls(Parser* lexer);

    bool LexerlexTopLevelStatementTokens(Parser* lexer);

    bool LexerlexNestedLevelStatementTokens(Parser* lexer, bool is_value, bool lex_value_node);

    bool LexerlexStatementTokens(Parser* lexer);

    bool LexerlexThrowStatementTokens(Parser* lexer);

    void LexerlexTopLevelMultipleStatementsTokens(Parser* lexer, bool break_at_no_stmt);

    void LexerlexTopLevelMultipleImportStatements(Parser* lexer);

    void LexerlexNestedLevelMultipleStatementsTokens(Parser* lexer, bool is_value, bool lex_value_node);

    void LexerlexMultipleStatementsTokens(Parser* lexer);

    bool LexerlexSingleLineCommentTokens(Parser* lexer);

    bool LexerlexMultiLineCommentTokens(Parser* lexer);

    bool LexerlexBraceBlock(Parser* lexer, void(* nested_lexer)(Parser*));

    bool LexerlexTopLevelBraceBlock(Parser* lexer);

    bool LexerlexBraceBlockStmts(Parser* lexer);

    bool LexerlexBraceBlockOrSingleStmt(Parser* lexer, bool is_value, bool lex_value_node);

    bool LexerlexImportIdentifierList(Parser* lexer);

    bool LexerlexImportStatement(Parser* lexer);

    bool LexerlexDestructStatement(Parser* lexer);

    bool LexerlexReturnStatement(Parser* lexer);

    bool LexerlexConstructorInitBlock(Parser* lexer);

    bool LexerlexUnsafeBlock(Parser* lexer);

    bool LexerlexBreakStatement(Parser* lexer);

    bool LexerlexTypealiasStatement(Parser* lexer, unsigned start);

    bool LexerlexContinueStatement(Parser* lexer);

    bool LexerlexIfExprAndBlock(Parser* lexer, unsigned start, bool is_value, bool lex_value_node, bool top_level);

    bool LexerlexIfBlockTokens(Parser* lexer, bool is_value, bool lex_value_node, bool top_level);

    bool LexerlexDoWhileBlockTokens(Parser* lexer);

    bool LexerlexWhileBlockTokens(Parser* lexer);

    bool LexerlexForBlockTokens(Parser* lexer);

    bool LexerlexLoopBlockTokens(Parser* lexer, bool is_value);

    bool LexerlexParameterList(Parser* lexer, bool optionalTypes, bool defValues, bool lexSelfParam, bool variadicParam);

    bool LexerlexFunctionSignatureTokens(Parser* lexer);

    bool LexerlexGenericParametersList(Parser* lexer);

    bool LexerlexAfterFuncKeyword(Parser* lexer, bool allow_extensions);

    bool LexerlexFunctionStructureTokens(Parser* lexer, unsigned start, bool allow_declaration, bool allow_extensions);

    void LexerlexInterfaceBlockTokens(Parser* lexer);

    bool LexerlexInterfaceStructureTokens(Parser* lexer, unsigned start);

    bool LexerlexNamespaceTokens(Parser* lexer, unsigned start);

    bool LexerlexStructMemberTokens(Parser* lexer);

    void LexerlexStructBlockTokens(Parser* lexer);

    bool LexerlexStructStructureTokens(Parser* lexer, unsigned start, bool unnamed, bool direct_init);

    bool LexerlexVariantMemberTokens(Parser* lexer);

    void LexerlexVariantBlockTokens(Parser* lexer);

    bool LexerlexVariantStructureTokens(Parser* lexer, unsigned start);

    void LexerlexUnionBlockTokens(Parser* lexer);

    bool LexerlexUnionStructureTokens(Parser* lexer, unsigned start, bool unnamed, bool direct_init);

    void LexerlexImplBlockTokens(Parser* lexer);

    bool LexerlexImplTokens(Parser* lexer);

    bool LexerlexEnumBlockTokens(Parser* lexer);

    bool LexerlexEnumStructureTokens(Parser* lexer, unsigned start);

    bool LexerreadWhitespace(Parser* lexer);

    bool LexerlexWhitespaceToken(Parser* lexer);

    bool LexerlexStringToken(Parser* lexer);

    bool LexerlexCharToken(Parser* lexer);

    bool LexerlexAnnotationMacro(Parser* lexer);

    bool LexerlexNull(Parser* lexer);

    bool LexerlexBoolToken(Parser* lexer);

    bool LexerlexUnsignedIntAsNumberToken(Parser* lexer);

    bool LexerlexNumberToken(Parser* lexer);

    bool LexerlexStructValueTokens(Parser* lexer, unsigned back_start);

    bool LexerlexValueToken(Parser* lexer);

    bool LexerlexSwitchCaseValue(Parser* lexer);

    bool LexerlexAccessChainValueToken(Parser* lexer);

    bool LexerlexArrayInit(Parser* lexer);

    bool LexerlexAccessChainOrValue(Parser* lexer, bool lexStruct);

    bool LexerlexValueNode(Parser* lexer);

    void LexerlexIdentifierList(Parser* lexer);

    bool LexerlexLambdaAfterParamsList(Parser* lexer, unsigned int start);

    bool LexerlexLambdaValue(Parser* lexer);

    bool LexerlexRemainingExpression(Parser* lexer, unsigned start);

    bool LexerlexLambdaOrExprAfterLParen(Parser* lexer);

    bool LexerlexParenExpressionAfterLParen(Parser* lexer);

    bool LexerlexParenExpression(Parser* lexer);

    bool LexerlexExpressionTokens(Parser* lexer, bool lexStruct, bool lambda);

    bool LexerlexSwitchStatementBlock(Parser* lexer, bool is_value, bool lex_value_node);

    bool LexerlexTryCatchTokens(Parser* lexer);

    bool LexerlexUsingStatement(Parser* lexer);

}