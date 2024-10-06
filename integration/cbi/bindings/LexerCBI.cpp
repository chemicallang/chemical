// Copyright (c) Qinetik 2024.

#include "LexerCBI.h"
#include "lexer/Lexer.h"

SourceProvider* Lexerprovider(Lexer* lexer) {
    return &lexer->provider;
}

std::size_t Lexertokens_size(Lexer* lexer)  {
    return lexer->tokens_size();
}

void Lexerput(Lexer* lexer, chem::string* value, LexTokenType token_type, unsigned int lineNumber, unsigned int lineCharNumber) {
    lexer->emplace(token_type, { lineNumber, lineCharNumber }, value->to_view());
}

bool LexerstoreVariable(Lexer* lexer, chem::string* str)  {
    return lexer->storeVariable(str);
}

bool LexerstoreIdentifier(Lexer* lexer, chem::string* str)  {
    return lexer->storeIdentifier(str);
}

void LexerlexGenericArgsList(Lexer* lexer)  {
    return lexer->lexGenericArgsList();
}

bool LexerlexGenericArgsListCompound(Lexer* lexer)  {
    return lexer->lexGenericArgsListCompound();
}

void LexerlexFunctionCallWithGenericArgsList(Lexer* lexer)  {
    return lexer->lexFunctionCallWithGenericArgsList();
}

bool LexerlexFunctionCall(Lexer* lexer, unsigned int back_start)  {
    return lexer->lexFunctionCall(back_start);
}

bool LexerlexAccessSpecifier(Lexer* lexer, bool internal, bool protect)  {
    return lexer->lexAccessSpecifier(internal, protect);
}

bool LexerlexAccessChainAfterId(Lexer* lexer, bool lexStruct, unsigned int chain_length)  {
    return lexer->lexAccessChainAfterId(lexStruct, chain_length);
}

bool LexerlexAccessChainRecursive(Lexer* lexer, bool lexStruct, unsigned int chain_length)  {
    return lexer->lexAccessChainRecursive(lexStruct, chain_length);
}

bool LexerlexAccessChain(Lexer* lexer, bool lexStruct, bool lex_as_node)  {
    return lexer->lexAccessChain(lexStruct, lex_as_node);
}

bool LexerlexAccessChainOrAddrOf(Lexer* lexer, bool lexStruct)  {
    return lexer->lexAccessChainOrAddrOf(lexStruct);
}

bool LexerlexVarInitializationTokens(Lexer* lexer, unsigned start, bool allowDeclarations, bool requiredType)  {
    return lexer->lexVarInitializationTokens(start, allowDeclarations, requiredType);
}

bool LexerlexAssignmentTokens(Lexer* lexer)  {
    return lexer->lexAssignmentTokens();
}

bool LexerlexDivisionOperatorToken(Lexer* lexer)  {
    return lexer->lexDivisionOperatorToken();
}

bool LexerlexLanguageOperatorToken(Lexer* lexer)  {
    return lexer->lexLanguageOperatorToken();
}

bool LexerisGenericEndAhead(Lexer* lexer)  {
    return lexer->isGenericEndAhead();
}

bool LexerlexAssignmentOperatorToken(Lexer* lexer)  {
    return lexer->lexAssignmentOperatorToken();
}

bool LexerlexLambdaTypeTokens(Lexer* lexer, unsigned int start)  {
    return lexer->lexLambdaTypeTokens(start);
}

bool LexerlexGenericTypeAfterId(Lexer* lexer, unsigned int start)  {
    return lexer->lexLambdaTypeTokens(start);
}

bool LexerlexRefOrGenericType(Lexer* lexer)  {
    return lexer->lexRefOrGenericType();
}

void LexerlexArrayAndPointerTypesAfterTypeId(Lexer* lexer, unsigned int start)  {
    return lexer->lexArrayAndPointerTypesAfterTypeId(start);
}

bool LexerlexTypeTokens(Lexer* lexer)  {
    return lexer->lexTypeTokens();
}

bool LexerlexTopLevelAccessSpecifiedDecls(Lexer* lexer)  {
    return lexer->lexTopLevelAccessSpecifiedDecls();
}

bool LexerlexTopLevelStatementTokens(Lexer* lexer)  {
    return lexer->lexTopLevelStatementTokens();
}

bool LexerlexNestedLevelStatementTokens(Lexer* lexer, bool is_value, bool lex_value_node)  {
    return lexer->lexNestedLevelStatementTokens(is_value, lex_value_node);
}

bool LexerlexStatementTokens(Lexer* lexer)  {
    return lexer->lexStatementTokens();
}

bool LexerlexThrowStatementTokens(Lexer* lexer)  {
    return lexer->lexThrowStatementTokens();
}

bool LexerlexOperatorToken(Lexer* lexer, char op)  {
    return lexer->lexOperatorToken(op);
}

bool LexerlexOperatorTokenStr(Lexer* lexer, chem::string* str)  {
    return lexer->lexOperatorToken({ str->data(), str->size() });
}

void LexerstoreOperationToken(Lexer* lexer, char token, Operation op)  {
    return lexer->storeOperationToken(token, op);
}

bool LexerlexOperationToken(Lexer* lexer, char token, Operation op)  {
    return lexer->lexOperationToken(token, op);
}

bool LexerlexOperatorTokenStr2(Lexer* lexer, chem::string* str, Operation op)  {
    return lexer->lexOperatorToken({ str->data(), str->size() }, op);
}

bool LexerlexKeywordToken(Lexer* lexer, chem::string* str)  {
    return lexer->lexKeywordToken({ str->data(), str->size() });
}

bool LexerlexWSKeywordToken(Lexer* lexer, chem::string* str)  {
    return lexer->lexWSKeywordToken({ str->data(), str->size() });
}

bool LexerlexWSKeywordToken2(Lexer* lexer, chem::string* str, char may_end_at)  {
    return lexer->lexWSKeywordToken({ str->data(), str->size() }, may_end_at);
}

void LexerlexTopLevelMultipleStatementsTokens(Lexer* lexer, bool break_at_no_stmt)  {
    return lexer->lexTopLevelMultipleStatementsTokens(break_at_no_stmt);
}

void LexerlexTopLevelMultipleImportStatements(Lexer* lexer)  {
    return lexer->lexTopLevelMultipleImportStatements();
}

void LexerlexNestedLevelMultipleStatementsTokens(Lexer* lexer, bool is_value, bool lex_value_node)  {
    return lexer->lexNestedLevelMultipleStatementsTokens(is_value, lex_value_node);
}

void LexerlexMultipleStatementsTokens(Lexer* lexer)  {
    return lexer->lexNestedLevelMultipleStatementsTokens();
}

bool LexerlexSingleLineCommentTokens(Lexer* lexer)  {
    return lexer->lexSingleLineCommentTokens();
}

bool LexerlexMultiLineCommentTokens(Lexer* lexer)  {
    return lexer->lexMultiLineCommentTokens();
}

bool LexerlexBraceBlock(Lexer* lexer, void(*nested_lexer)(Lexer*))  {
    return lexer->lexBraceBlock("cbi", nested_lexer);
}

bool LexerlexTopLevelBraceBlock(Lexer* lexer)  {
    return lexer->lexTopLevelBraceBlock("cbi");
}

bool LexerlexBraceBlockStmts(Lexer* lexer)  {
    return lexer->lexBraceBlock("cbi");
}

bool LexerlexBraceBlockOrSingleStmt(Lexer* lexer, bool is_value, bool lex_value_node)  {
    return lexer->lexBraceBlockOrSingleStmt("cbi", is_value, lex_value_node);
}

bool LexerlexImportIdentifierList(Lexer* lexer)  {
    return lexer->lexImportIdentifierList();
}

bool LexerlexImportStatement(Lexer* lexer)  {
    return lexer->lexImportStatement();
}

bool LexerlexDestructStatement(Lexer* lexer)  {
    return lexer->lexDestructStatement();
}

bool LexerlexReturnStatement(Lexer* lexer)  {
    return lexer->lexReturnStatement();
}

bool LexerlexConstructorInitBlock(Lexer* lexer)  {
    return lexer->lexConstructorInitBlock();
}

bool LexerlexUnsafeBlock(Lexer* lexer)  {
    return lexer->lexUnsafeBlock();
}

bool LexerlexBreakStatement(Lexer* lexer)  {
    return lexer->lexBreakStatement();
}

bool LexerlexTypealiasStatement(Lexer* lexer, unsigned start)  {
    return lexer->lexTypealiasStatement(start);
}

bool LexerlexContinueStatement(Lexer* lexer)  {
    return lexer->lexContinueStatement();
}

void LexerlexIfExprAndBlock(Lexer* lexer, unsigned start, bool is_value, bool lex_value_node, bool top_level)  {
    return lexer->lexIfExprAndBlock(start, is_value, lex_value_node, top_level);
}

bool LexerlexIfBlockTokens(Lexer* lexer, bool is_value, bool lex_value_node, bool top_level)  {
    return lexer->lexIfBlockTokens(is_value, lex_value_node, top_level);
}

bool LexerlexDoWhileBlockTokens(Lexer* lexer)  {
    return lexer->lexDoWhileBlockTokens();
}

bool LexerlexWhileBlockTokens(Lexer* lexer)  {
    return lexer->lexWhileBlockTokens();
}

bool LexerlexForBlockTokens(Lexer* lexer)  {
    return lexer->lexForBlockTokens();
}

bool LexerlexLoopBlockTokens(Lexer* lexer, bool is_value)  {
    return lexer->lexLoopBlockTokens(is_value);
}

bool LexerlexParameterList(Lexer* lexer, bool optionalTypes, bool defValues, bool lexSelfParam, bool variadicParam)  {
    return lexer->lexParameterList(optionalTypes, defValues, lexSelfParam, variadicParam);
}

bool LexerlexFunctionSignatureTokens(Lexer* lexer)  {
    return lexer->lexFunctionSignatureTokens();
}

bool LexerlexGenericParametersList(Lexer* lexer)  {
    return lexer->lexGenericParametersList();
}

bool LexerlexAfterFuncKeyword(Lexer* lexer, bool allow_extensions)  {
    return lexer->lexAfterFuncKeyword(allow_extensions);
}

bool LexerlexFunctionStructureTokens(Lexer* lexer, unsigned start, bool allow_declaration, bool allow_extensions)  {
    return lexer->lexFunctionStructureTokens(start, allow_declaration, allow_extensions);
}

void LexerlexInterfaceBlockTokens(Lexer* lexer)  {
    return lexer->lexInterfaceBlockTokens();
}

bool LexerlexInterfaceStructureTokens(Lexer* lexer, unsigned start)  {
    return lexer->lexInterfaceStructureTokens(start);
}

bool LexerlexNamespaceTokens(Lexer* lexer, unsigned start)  {
    return lexer->lexNamespaceTokens(start);
}

bool LexerlexStructMemberTokens(Lexer* lexer)  {
    return lexer->lexStructMemberTokens();
}

void LexerlexStructBlockTokens(Lexer* lexer)  {
    return lexer->lexStructBlockTokens();
}

bool LexerlexStructStructureTokens(Lexer* lexer, unsigned start, bool unnamed, bool direct_init)  {
    return lexer->lexStructStructureTokens(start, unnamed, direct_init);
}

bool LexerlexVariantMemberTokens(Lexer* lexer)  {
    return lexer->lexVariantMemberTokens();
}

void LexerlexVariantBlockTokens(Lexer* lexer)  {
    return lexer->lexVariantBlockTokens();
}

bool LexerlexVariantStructureTokens(Lexer* lexer, unsigned start)  {
    return lexer->lexVariantStructureTokens();
}

void LexerlexUnionBlockTokens(Lexer* lexer)  {
    return lexer->lexUnionBlockTokens();
}

bool LexerlexUnionStructureTokens(Lexer* lexer, unsigned start, bool unnamed, bool direct_init)  {
    return lexer->lexUnionStructureTokens(start, unnamed, direct_init);
}

void LexerlexImplBlockTokens(Lexer* lexer)  {
    return lexer->lexImplBlockTokens();
}

bool LexerlexImplTokens(Lexer* lexer)  {
    return lexer->lexImplTokens();
}

bool LexerlexEnumBlockTokens(Lexer* lexer)  {
    return lexer->lexEnumBlockTokens();
}

bool LexerlexEnumStructureTokens(Lexer* lexer, unsigned start)  {
    return lexer->lexEnumStructureTokens(start);
}

bool LexerreadWhitespace(Lexer* lexer)  {
    return lexer->readWhitespace();
}

bool LexerlexWhitespaceToken(Lexer* lexer)  {
    return lexer->lexWhitespaceToken();
}

bool LexerlexStringToken(Lexer* lexer)  {
    return lexer->lexStringToken();
}

bool LexerlexCharToken(Lexer* lexer)  {
    return lexer->lexCharToken();
}

bool LexerlexAnnotationMacro(Lexer* lexer)  {
    return lexer->lexAnnotationMacro();
}

bool LexerlexNull(Lexer* lexer)  {
    return lexer->lexNull();
}

bool LexerlexBoolToken(Lexer* lexer)  {
    return lexer->lexBoolToken();
}

bool LexerlexUnsignedIntAsNumberToken(Lexer* lexer)  {
    return lexer->lexUnsignedIntAsNumberToken();
}

bool LexerlexNumberToken(Lexer* lexer)  {
    return lexer->lexNumberToken();
}

bool LexerlexStructValueTokens(Lexer* lexer, unsigned back_start)  {
    return lexer->lexStructValueTokens(back_start);
}

bool LexerlexValueToken(Lexer* lexer)  {
    return lexer->lexValueToken();
}

bool LexerlexSwitchCaseValue(Lexer* lexer)  {
    return lexer->lexSwitchCaseValue();
}

bool LexerlexAccessChainValueToken(Lexer* lexer)  {
    return lexer->lexAccessChainValueToken();
}

bool LexerlexArrayInit(Lexer* lexer)  {
    return lexer->lexArrayInit();
}

bool LexerlexAccessChainOrValue(Lexer* lexer, bool lexStruct)  {
    return lexer->lexAccessChainOrValue(lexStruct);
}

bool LexerlexValueNode(Lexer* lexer)  {
    return lexer->lexValueNode();
}

void LexerlexIdentifierList(Lexer* lexer)  {
    return lexer->lexIdentifierList();
}

void LexerlexLambdaAfterParamsList(Lexer* lexer, unsigned int start)  {
    return lexer->lexLambdaAfterParamsList(start);
}

bool LexerlexLambdaValue(Lexer* lexer)  {
    return lexer->lexLambdaValue();
}

bool LexerlexRemainingExpression(Lexer* lexer, unsigned start)  {
    return lexer->lexRemainingExpression(start);
}

bool LexerlexLambdaOrExprAfterLParen(Lexer* lexer)  {
    return lexer->lexLambdaOrExprAfterLParen();
}

void LexerlexParenExpressionAfterLParen(Lexer* lexer)  {
    return lexer->lexParenExpressionAfterLParen();
}

bool LexerlexParenExpression(Lexer* lexer)  {
    return lexer->lexParenExpression();
}

bool LexerlexExpressionTokens(Lexer* lexer, bool lexStruct, bool lambda)  {
    return lexer->lexExpressionTokens(lexStruct, lambda);
}

bool LexerlexSwitchStatementBlock(Lexer* lexer, bool is_value, bool lex_value_node)  {
    return lexer->lexSwitchStatementBlock(is_value, lex_value_node);
}

bool LexerlexTryCatchTokens(Lexer* lexer)  {
    return lexer->lexTryCatchTokens();
}

bool LexerlexUsingStatement(Lexer* lexer)  {
    return lexer->lexUsingStatement();
}