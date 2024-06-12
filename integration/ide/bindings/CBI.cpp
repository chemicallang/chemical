// Copyright (c) Qinetik 2024.

#include "SourceProviderCBI.h"
#include "LexerCBI.h"
#include "lexer/Lexer.h"
#include "stream/SourceProvider.h"

void init_lexer_cbi(LexerCBI* cbi, Lexer* lexer) {
    cbi->instance = lexer;
    init_source_provider_cbi(&cbi->provider, &lexer->provider);
    cbi->storeVariable = [](LexerCBI* cbi, char* variable){
        return cbi->instance->storeVariable(variable);
    };
    cbi->storeIdentifier = [](LexerCBI* cbi, char* id){
        return cbi->instance->storeIdentifier(id);
    };
    cbi->lexVariableToken = [](LexerCBI* cbi){
        return cbi->instance->lexVariableToken();
    };
    cbi->lexIdentifierToken = [](LexerCBI* cbi){
        return cbi->instance->lexIdentifierToken();
    };
    cbi->lexAccessChainAfterId = [](LexerCBI* cbi, bool lexStruct){
        return cbi->instance->lexAccessChainAfterId(lexStruct);
    };
    cbi->lexAccessChainRecursive = [](LexerCBI* cbi, bool lexStruct){
        return cbi->instance->lexAccessChainRecursive(lexStruct);
    };
    cbi->lexAccessChain = [](LexerCBI* cbi, bool lexStruct){
        return cbi->instance->lexAccessChain(lexStruct);
    };
    cbi->lexAccessChainOrAddrOf = [](LexerCBI* cbi, bool lexStruct){
        return cbi->instance->lexAccessChainOrAddrOf(lexStruct);
    };
    cbi->lexVarInitializationTokens = [](LexerCBI* cbi, bool allowDecls, bool requiredType){
        return cbi->instance->lexVarInitializationTokens(allowDecls, requiredType);
    };
    cbi->lexAssignmentTokens = [](LexerCBI* cbi){
        return cbi->instance->lexAssignmentTokens();
    };
    cbi->lexLanguageOperatorToken = [](LexerCBI* cbi){
        return cbi->instance->lexLanguageOperatorToken();
    };
    cbi->lexAssignmentOperatorToken = [](LexerCBI* cbi){
        return cbi->instance->lexAssignmentOperatorToken();
    };
    cbi->lexLambdaTypeTokens = [](LexerCBI* cbi, unsigned int start){
        return cbi->instance->lexLambdaTypeTokens(start);
    };
    cbi->lexTypeTokens = [](LexerCBI* cbi){
        return cbi->instance->lexTypeTokens();
    };
    cbi->lexTopLevelStatementTokens = [](LexerCBI* cbi){
        return cbi->instance->lexTopLevelStatementTokens();
    };
    cbi->lexNestedLevelStatementTokens = [](LexerCBI* cbi){
        return cbi->instance->lexNestedLevelStatementTokens();
    };
    cbi->lexStatementTokens = [](LexerCBI* cbi){
        return cbi->instance->lexStatementTokens();
    };
    cbi->lexOperatorToken = [](LexerCBI* cbi, char op){
        return cbi->instance->lexOperatorToken(op);
    };
    cbi->lexOperatorTokenStr = [](LexerCBI* cbi, char* str){
        return cbi->instance->lexOperatorToken(str);
    };
    cbi->lexOperationToken = [](LexerCBI* cbi, char token, Operation op){
        return cbi->instance->lexOperationToken(token, op);
    };
    cbi->lexStrOperationToken = [](LexerCBI* cbi, char* token, Operation op){
        return cbi->instance->lexOperatorToken(token, op);
    };
    cbi->lexKeywordToken = [](LexerCBI* cbi, char* keyword){
        return cbi->instance->lexKeywordToken(keyword);
    };
    cbi->lexTopLevelMultipleStatementsTokens = [](LexerCBI* cbi){
        return cbi->instance->lexTopLevelMultipleStatementsTokens();
    };
    cbi->lexTopLevelMultipleImportStatements = [](LexerCBI* cbi){
        return cbi->instance->lexTopLevelMultipleImportStatements();
    };
    cbi->lexNestedLevelMultipleStatementsTokens = [](LexerCBI* cbi){
        return cbi->instance->lexNestedLevelMultipleStatementsTokens();
    };
    cbi->lexMultipleStatementsTokens = [](LexerCBI* cbi){
        return cbi->instance->lexMultipleStatementsTokens();
    };
    cbi->lexSingleLineCommentTokens = [](LexerCBI* cbi){
        return cbi->instance->lexSingleLineCommentTokens();
    };
    cbi->lexMultiLineCommentTokens = [](LexerCBI* cbi){
        return cbi->instance->lexMultiLineCommentTokens();
    };
    cbi->lexBraceBlock = [](LexerCBI* cbi, char* forThing){
        return cbi->instance->lexBraceBlock(forThing);
    };
    cbi->lexIfExpression = [](LexerCBI* cbi){
        return cbi->instance->lexIfExpression();
    };
    cbi->lexImportIdentifierList = [](LexerCBI* cbi){
        return cbi->instance->lexImportIdentifierList();
    };
    cbi->lexImportStatement = [](LexerCBI* cbi){
        return cbi->instance->lexImportStatement();
    };
    cbi->lexReturnStatement = [](LexerCBI* cbi){
        return cbi->instance->lexReturnStatement();
    };
    cbi->lexBreakStatement = [](LexerCBI* cbi){
        return cbi->instance->lexBreakStatement();
    };
    cbi->lexTypealiasStatement = [](LexerCBI* cbi){
        return cbi->instance->lexTypealiasStatement();
    };
    cbi->lexContinueStatement = [](LexerCBI* cbi){
        return cbi->instance->lexContinueStatement();
    };
    cbi->lexIfExprAndBlock = [](LexerCBI* cbi){
        return cbi->instance->lexIfExprAndBlock();
    };
    cbi->lexIfBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexIfBlockTokens();
    };
    cbi->lexDoWhileBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexDoWhileBlockTokens();
    };
    cbi->lexWhileBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexWhileBlockTokens();
    };
    cbi->lexForBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexForBlockTokens();
    };
    cbi->lexParameterList = [](LexerCBI* cbi, bool optionalTypes, bool defValues){
        return cbi->instance->lexParameterList(optionalTypes, defValues);
    };
    cbi->lexFunctionSignatureTokens = [](LexerCBI* cbi){
        return cbi->instance->lexFunctionSignatureTokens();
    };
    cbi->lexAfterFuncKeyword = [](LexerCBI* cbi){
        return cbi->instance->lexAfterFuncKeyword();
    };
    cbi->lexFunctionStructureTokens = [](LexerCBI* cbi, bool allowDecls){
        return cbi->instance->lexFunctionStructureTokens(allowDecls);
    };
    cbi->lexInterfaceBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexInterfaceBlockTokens();
    };
    cbi->lexInterfaceStructureTokens = [](LexerCBI* cbi){
        return cbi->instance->lexInterfaceStructureTokens();
    };
    cbi->lexStructMemberTokens = [](LexerCBI* cbi){
        return cbi->instance->lexStructMemberTokens();
    };
    cbi->lexStructBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexStructBlockTokens();
    };
    cbi->lexStructStructureTokens = [](LexerCBI* cbi){
        return cbi->instance->lexStructStructureTokens();
    };
    cbi->collectStructAsLexer = [](LexerCBI* cbi, unsigned int start, unsigned int end){
        return cbi->instance->collect_cbi_node(start, end);
    };
    cbi->lexImplBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexImplBlockTokens();
    };
    cbi->lexImplTokens = [](LexerCBI* cbi){
        return cbi->instance->lexImplTokens();
    };
    cbi->lexEnumBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexEnumBlockTokens();
    };
    cbi->lexEnumStructureTokens = [](LexerCBI* cbi){
        return cbi->instance->lexEnumStructureTokens();
    };
    cbi->lexWhitespaceToken = [](LexerCBI* cbi){
        return cbi->instance->lexWhitespaceToken();
    };
    cbi->lexWhitespaceAndNewLines = [](LexerCBI* cbi){
        return cbi->instance->lexWhitespaceAndNewLines();
    };
    cbi->lexStringToken = [](LexerCBI* cbi){
        return cbi->instance->lexStringToken();
    };
    cbi->lexCharToken = [](LexerCBI* cbi){
        return cbi->instance->lexCharToken();
    };
    cbi->lexAnnotationMacro = [](LexerCBI* cbi){
        return cbi->instance->lexAnnotationMacro();
    };
    cbi->lexNull = [](LexerCBI* cbi){
        return cbi->instance->lexNull();
    };
    cbi->lexBoolToken = [](LexerCBI* cbi){
        return cbi->instance->lexBoolToken();
    };
    cbi->lexUnsignedIntAsNumberToken = [](LexerCBI* cbi){
        return cbi->instance->lexUnsignedIntAsNumberToken();
    };
    cbi->lexNumberToken = [](LexerCBI* cbi){
        return cbi->instance->lexNumberToken();
    };
    cbi->lexStructValueTokens = [](LexerCBI* cbi){
        return cbi->instance->lexStructValueTokens();
    };
    cbi->lexValueToken = [](LexerCBI* cbi){
        return cbi->instance->lexValueToken();
    };
    cbi->lexAccessChainValueToken = [](LexerCBI* cbi){
        return cbi->instance->lexAccessChainValueToken();
    };
    cbi->lexArrayInit = [](LexerCBI* cbi){
        return cbi->instance->lexArrayInit();
    };
    cbi->lexAccessChainOrValue = [](LexerCBI* cbi, bool lexStruct){
        return cbi->instance->lexAccessChainOrValue(lexStruct);
    };
    cbi->lexIdentifierList = [](LexerCBI* cbi){
        return cbi->instance->lexIdentifierList();
    };
    cbi->lexLambdaAfterParamsList = [](LexerCBI* cbi, unsigned int start){
        return cbi->instance->lexLambdaAfterParamsList(start);
    };
    cbi->lexLambdaValue = [](LexerCBI* cbi){
        return cbi->instance->lexLambdaValue();
    };
    cbi->lexRemainingExpression = [](LexerCBI* cbi, unsigned int start){
        return cbi->instance->lexRemainingExpression(start);
    };
    cbi->lexLambdaAfterLParen = [](LexerCBI* cbi){
        return cbi->instance->lexLambdaAfterLParen();
    };
    cbi->lexParenExpressionAfterLParen = [](LexerCBI* cbi){
        return cbi->instance->lexParenExpressionAfterLParen();
    };
    cbi->lexParenExpression = [](LexerCBI* cbi){
        return cbi->instance->lexParenExpression();
    };
    cbi->lexExpressionTokens = [](LexerCBI* cbi, bool lexStruct, bool lambda){
        return cbi->instance->lexExpressionTokens(lexStruct, lambda);
    };
    cbi->lexSwitchStatementBlock = [](LexerCBI* cbi){
        return cbi->instance->lexSwitchStatementBlock();
    };
    cbi->lexTryCatchTokens = [](LexerCBI* cbi){
        return cbi->instance->lexTryCatchTokens();
    };
}

void init_source_provider_cbi(SourceProviderCBI* cbi, SourceProvider* provider) {
    cbi->instance = provider;
    cbi->currentPosition = [](struct SourceProviderCBI* cbi){
        return cbi->instance->currentPosition();
    };
    cbi->readCharacter = [](struct SourceProviderCBI* cbi){
        return cbi->instance->readCharacter();
    };
    cbi->eof = [](struct SourceProviderCBI* cbi){
        return cbi->instance->eof();
    };
    cbi->peek = [](struct SourceProviderCBI* cbi){
        return cbi->instance->peek();
    };
    cbi->peek_at = [](struct SourceProviderCBI* cbi, int ahead){
        return cbi->instance->peek(ahead);
    };
    cbi->readUntil = [](struct SourceProviderCBI* cbi, char stop){
        return (char*) cbi->instance->readUntil(stop).c_str();
    };
    cbi->increment = [](struct SourceProviderCBI* cbi, char* text, bool peek){
        return cbi->instance->increment(text, peek);
    };
    cbi->increment_char = [](struct SourceProviderCBI* cbi, char c){
        return cbi->instance->increment(c);
    };
    cbi->readAllFromHere = [](struct SourceProviderCBI* cbi){
        return (char*) cbi->instance->readAllFromHere().c_str();
    };
    cbi->getLineNumber = [](struct SourceProviderCBI* cbi){
        return cbi->instance->getLineNumber();
    };
    cbi->getLineCharNumber = [](struct SourceProviderCBI* cbi){
        return cbi->instance->getLineCharNumber();
    };
    cbi->readEscaping = [](struct SourceProviderCBI* cbi, char* value, char stopAt){
        throw std::runtime_error("This requires that char* is passed as string");
//        cbi->instance->readEscaping(value, stopAt);
    };
    cbi->readAnything = [](struct SourceProviderCBI* cbi, char until){
        return (char*) cbi->instance->readAnything(until).c_str();
    };
    cbi->readAlpha = [](struct SourceProviderCBI* cbi){
        return (char*) cbi->instance->readAlpha().c_str();
    };
    cbi->readUnsignedInt = [](struct SourceProviderCBI* cbi){
        return (char*) cbi->instance->readUnsignedInt().c_str();
    };
    cbi->readNumber = [](struct SourceProviderCBI* cbi){
        return (char*) cbi->instance->readNumber().c_str();
    };
    cbi->readAlphaNum = [](struct SourceProviderCBI* cbi){
        return (char*) cbi->instance->readAlphaNum().c_str();
    };
    cbi->readIdentifier = [](struct SourceProviderCBI* cbi){
        return (char*) cbi->instance->readIdentifier().c_str();
    };
    cbi->readAnnotationIdentifierInto = [](struct SourceProviderCBI* cbi, char* into){
        throw std::runtime_error("This requires that char* is passed as string");
//        cbi->instance->readAnnotationIdentifier(into);
    };
    cbi->readAnnotationIdentifier = [](struct SourceProviderCBI* cbi){
        return (char*) cbi->instance->readAnnotationIdentifier().c_str();
    };
    cbi->readWhitespaces = [](struct SourceProviderCBI* cbi){
        return cbi->instance->readWhitespaces();
    };
    cbi->hasNewLine = [](struct SourceProviderCBI* cbi){
        return cbi->instance->hasNewLine();
    };
    cbi->readNewLineChars = [](struct SourceProviderCBI* cbi){
        return cbi->instance->readNewLineChars();
    };
    cbi->readWhitespacesAndNewLines = [](struct SourceProviderCBI* cbi){
        return cbi->instance->readWhitespacesAndNewLines();
    };
}