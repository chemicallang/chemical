// Copyright (c) Qinetik 2024.

#include "LexerCBI.h"
#include "parser/Parser.h"

std::size_t Lexertokens_size(Parser* lexer)  {
    return lexer->tokens_size();
}

CSTToken* Lexerput(Parser* lexer, chem::string* value, LexTokenType token_type, unsigned int lineNumber, unsigned int lineCharNumber) {
    lexer->emplace(token_type, { lineNumber, lineCharNumber }, value->to_view());
    return lexer->unit.tokens.back();
}

void LexerlexGenericArgsList(Parser* lexer)  {
    return lexer->lexGenericArgsList();
}

bool LexerlexGenericArgsListCompound(Parser* lexer)  {
    return lexer->lexGenericArgsListCompound();
}

void LexerlexFunctionCallWithGenericArgsList(Parser* lexer)  {
    return lexer->lexFunctionCallWithGenericArgsList();
}

bool LexerlexFunctionCall(Parser* lexer, unsigned int back_start)  {
    return lexer->lexFunctionCall(back_start);
}

bool LexerlexAccessSpecifier(Parser* lexer, bool internal, bool protect)  {
    return lexer->lexAccessSpecifier(internal, protect);
}

bool LexerlexAccessChainAfterId(Parser* lexer, bool lexStruct, unsigned int chain_length)  {
    return lexer->lexAccessChainAfterId(lexStruct, chain_length);
}

bool LexerlexAccessChainRecursive(Parser* lexer, bool lexStruct, unsigned int chain_length)  {
    return lexer->lexAccessChainRecursive(lexStruct, chain_length);
}

bool LexerlexAccessChain(Parser* lexer, bool lexStruct, bool lex_as_node)  {
    return lexer->lexAccessChain(lexStruct, lex_as_node);
}

bool LexerlexAccessChainOrAddrOf(Parser* lexer, bool lexStruct)  {
    return lexer->lexAccessChainOrAddrOf(lexStruct);
}

bool LexerlexVarInitializationTokens(Parser* lexer, unsigned start, bool allowDeclarations, bool requiredType)  {
    return lexer->lexVarInitializationTokens(start, allowDeclarations, requiredType);
}

bool LexerlexAssignmentTokens(Parser* lexer)  {
    return lexer->lexAssignmentTokens();
}

bool LexerlexLanguageOperatorToken(Parser* lexer)  {
    return lexer->lexLanguageOperatorToken();
}

bool LexerisGenericEndAhead(Parser* lexer)  {
    return lexer->isGenericEndAhead();
}

bool LexerlexAssignmentOperatorToken(Parser* lexer)  {
    return lexer->lexAssignmentOperatorToken();
}

bool LexerlexLambdaTypeTokens(Parser* lexer, unsigned int start)  {
    return lexer->lexLambdaTypeTokens(start);
}

bool LexerlexGenericTypeAfterId(Parser* lexer, unsigned int start)  {
    return lexer->lexLambdaTypeTokens(start);
}

bool LexerlexRefOrGenericType(Parser* lexer)  {
    return lexer->lexRefOrGenericType();
}

void LexerlexArrayAndPointerTypesAfterTypeId(Parser* lexer, unsigned int start)  {
    return lexer->lexArrayAndPointerTypesAfterTypeId(start);
}

bool LexerlexTypeTokens(Parser* lexer)  {
    return lexer->lexTypeTokens();
}

bool LexerlexTopLevelAccessSpecifiedDecls(Parser* lexer)  {
    return lexer->lexTopLevelAccessSpecifiedDecls();
}

bool LexerlexTopLevelStatementTokens(Parser* lexer)  {
    return lexer->lexTopLevelStatementTokens();
}

bool LexerlexNestedLevelStatementTokens(Parser* lexer, bool is_value, bool lex_value_node)  {
    return lexer->lexNestedLevelStatementTokens(is_value, lex_value_node);
}

bool LexerlexStatementTokens(Parser* lexer)  {
    return lexer->lexStatementTokens();
}

bool LexerlexThrowStatementTokens(Parser* lexer)  {
    return lexer->lexThrowStatementTokens();
}

void LexerlexTopLevelMultipleStatementsTokens(Parser* lexer, bool break_at_no_stmt)  {
    return lexer->lexTopLevelMultipleStatementsTokens(break_at_no_stmt);
}

void LexerlexTopLevelMultipleImportStatements(Parser* lexer)  {
    return lexer->lexTopLevelMultipleImportStatements();
}

void LexerlexNestedLevelMultipleStatementsTokens(Parser* lexer, bool is_value, bool lex_value_node)  {
    return lexer->lexNestedLevelMultipleStatementsTokens(is_value, lex_value_node);
}

void LexerlexMultipleStatementsTokens(Parser* lexer)  {
    return lexer->lexNestedLevelMultipleStatementsTokens();
}

bool LexerlexSingleLineCommentTokens(Parser* lexer)  {
    return lexer->lexSingleLineCommentTokens();
}

bool LexerlexMultiLineCommentTokens(Parser* lexer)  {
    return lexer->lexMultiLineCommentTokens();
}

bool LexerlexBraceBlock(Parser* lexer, void(*nested_lexer)(Parser*))  {
    return lexer->lexBraceBlock("cbi", nested_lexer);
}

bool LexerlexTopLevelBraceBlock(Parser* lexer)  {
    return lexer->lexTopLevelBraceBlock("cbi");
}

bool LexerlexBraceBlockStmts(Parser* lexer)  {
    return lexer->lexBraceBlock("cbi");
}

bool LexerlexBraceBlockOrSingleStmt(Parser* lexer, bool is_value, bool lex_value_node)  {
    return lexer->lexBraceBlockOrSingleStmt("cbi", is_value, lex_value_node);
}

bool LexerlexImportIdentifierList(Parser* lexer)  {
    return lexer->lexImportIdentifierList();
}

bool LexerlexImportStatement(Parser* lexer)  {
    return lexer->lexImportStatement();
}

bool LexerlexDestructStatement(Parser* lexer)  {
    return lexer->lexDestructStatement();
}

bool LexerlexReturnStatement(Parser* lexer)  {
    return lexer->lexReturnStatement();
}

bool LexerlexConstructorInitBlock(Parser* lexer)  {
    return lexer->lexConstructorInitBlock();
}

bool LexerlexUnsafeBlock(Parser* lexer)  {
    return lexer->lexUnsafeBlock();
}

bool LexerlexBreakStatement(Parser* lexer)  {
    return lexer->lexBreakStatement();
}

bool LexerlexTypealiasStatement(Parser* lexer, unsigned start)  {
    return lexer->lexTypealiasStatement(start);
}

bool LexerlexContinueStatement(Parser* lexer)  {
    return lexer->lexContinueStatement();
}

bool LexerlexIfExprAndBlock(Parser* lexer, unsigned start, bool is_value, bool lex_value_node, bool top_level)  {
    return lexer->lexIfExprAndBlock(start, is_value, lex_value_node, top_level);
}

bool LexerlexIfBlockTokens(Parser* lexer, bool is_value, bool lex_value_node, bool top_level)  {
    return lexer->lexIfBlockTokens(is_value, lex_value_node, top_level);
}

bool LexerlexDoWhileBlockTokens(Parser* lexer)  {
    return lexer->lexDoWhileBlockTokens();
}

bool LexerlexWhileBlockTokens(Parser* lexer)  {
    return lexer->lexWhileBlockTokens();
}

bool LexerlexForBlockTokens(Parser* lexer)  {
    return lexer->lexForBlockTokens();
}

bool LexerlexLoopBlockTokens(Parser* lexer, bool is_value)  {
    return lexer->lexLoopBlockTokens(is_value);
}

bool LexerlexParameterList(Parser* lexer, bool optionalTypes, bool defValues, bool lexSelfParam, bool variadicParam)  {
    return lexer->lexParameterList(optionalTypes, defValues, lexSelfParam, variadicParam);
}

bool LexerlexFunctionSignatureTokens(Parser* lexer)  {
    return lexer->lexFunctionSignatureTokens();
}

bool LexerlexGenericParametersList(Parser* lexer)  {
    return lexer->lexGenericParametersList();
}

bool LexerlexAfterFuncKeyword(Parser* lexer, bool allow_extensions)  {
    return lexer->lexAfterFuncKeyword(allow_extensions);
}

bool LexerlexFunctionStructureTokens(Parser* lexer, unsigned start, bool allow_declaration, bool allow_extensions)  {
    return lexer->lexFunctionStructureTokens(start, allow_declaration, allow_extensions);
}

void LexerlexInterfaceBlockTokens(Parser* lexer)  {
    return lexer->lexInterfaceBlockTokens();
}

bool LexerlexInterfaceStructureTokens(Parser* lexer, unsigned start)  {
    return lexer->lexInterfaceStructureTokens(start);
}

bool LexerlexNamespaceTokens(Parser* lexer, unsigned start)  {
    return lexer->lexNamespaceTokens(start);
}

bool LexerlexStructMemberTokens(Parser* lexer)  {
    return lexer->lexStructMemberTokens();
}

void LexerlexStructBlockTokens(Parser* lexer)  {
    return lexer->lexStructBlockTokens();
}

bool LexerlexStructStructureTokens(Parser* lexer, unsigned start, bool unnamed, bool direct_init)  {
    return lexer->lexStructStructureTokens(start, unnamed, direct_init);
}

bool LexerlexVariantMemberTokens(Parser* lexer)  {
    return lexer->lexVariantMemberTokens();
}

void LexerlexVariantBlockTokens(Parser* lexer)  {
    return lexer->lexVariantBlockTokens();
}

bool LexerlexVariantStructureTokens(Parser* lexer, unsigned start)  {
    return lexer->lexVariantStructureTokens();
}

void LexerlexUnionBlockTokens(Parser* lexer)  {
    return lexer->lexUnionBlockTokens();
}

bool LexerlexUnionStructureTokens(Parser* lexer, unsigned start, bool unnamed, bool direct_init)  {
    return lexer->lexUnionStructureTokens(start, unnamed, direct_init);
}

void LexerlexImplBlockTokens(Parser* lexer)  {
    return lexer->lexImplBlockTokens();
}

bool LexerlexImplTokens(Parser* lexer)  {
    return lexer->lexImplTokens();
}

bool LexerlexEnumBlockTokens(Parser* lexer)  {
    return lexer->lexEnumBlockTokens();
}

bool LexerlexEnumStructureTokens(Parser* lexer, unsigned start)  {
    return lexer->lexEnumStructureTokens(start);
}

bool LexerreadWhitespace(Parser* lexer)  {
    return lexer->readWhitespace();
}

bool LexerlexWhitespaceToken(Parser* lexer)  {
    return lexer->lexWhitespaceToken();
}

bool LexerlexStringToken(Parser* lexer)  {
    return lexer->lexStringToken();
}

bool LexerlexCharToken(Parser* lexer)  {
    return lexer->lexCharToken();
}

bool LexerlexAnnotationMacro(Parser* lexer)  {
    return lexer->lexAnnotationMacro();
}

bool LexerlexNull(Parser* lexer)  {
    return lexer->lexNull();
}

bool LexerlexBoolToken(Parser* lexer)  {
    return lexer->lexBoolToken();
}

bool LexerlexUnsignedIntAsNumberToken(Parser* lexer)  {
    return lexer->lexUnsignedIntAsNumberToken();
}

bool LexerlexNumberToken(Parser* lexer)  {
    return lexer->lexNumberToken();
}

bool LexerlexStructValueTokens(Parser* lexer, unsigned back_start)  {
    return lexer->lexStructValueTokens(back_start);
}

bool LexerlexAccessChainValueToken(Parser* lexer)  {
    return lexer->lexAccessChainValueToken();
}

bool LexerlexArrayInit(Parser* lexer)  {
    return lexer->lexArrayInit();
}

bool LexerlexAccessChainOrValue(Parser* lexer, bool lexStruct)  {
    return lexer->lexAccessChainOrValue(lexStruct);
}

bool LexerlexValueNode(Parser* lexer)  {
    return lexer->lexValueNode();
}

void LexerlexIdentifierList(Parser* lexer)  {
    return lexer->lexIdentifierList();
}

bool LexerlexLambdaAfterParamsList(Parser* lexer, unsigned int start)  {
    return lexer->lexLambdaAfterParamsList(start);
}

bool LexerlexLambdaValue(Parser* lexer)  {
    return lexer->lexLambdaValue();
}

bool LexerlexRemainingExpression(Parser* lexer, unsigned start)  {
    return lexer->lexRemainingExpression(start);
}

bool LexerlexLambdaOrExprAfterLParen(Parser* lexer)  {
    return lexer->lexLambdaOrExprAfterLParen();
}

bool LexerlexParenExpressionAfterLParen(Parser* lexer)  {
    return lexer->lexParenExpressionAfterLParen();
}

bool LexerlexParenExpression(Parser* lexer)  {
    return lexer->lexParenExpression();
}

bool LexerlexExpressionTokens(Parser* lexer, bool lexStruct, bool lambda)  {
    return lexer->lexExpressionTokens(lexStruct, lambda);
}

bool LexerlexSwitchStatementBlock(Parser* lexer, bool is_value, bool lex_value_node)  {
    return lexer->lexSwitchStatementBlock(is_value, lex_value_node);
}

bool LexerlexTryCatchTokens(Parser* lexer)  {
    return lexer->lexTryCatchTokens();
}

bool LexerlexUsingStatement(Parser* lexer)  {
    return lexer->lexUsingStatement();
}