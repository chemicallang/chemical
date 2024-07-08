// Copyright (c) Qinetik 2024.

#pragma once

#include "SourceProviderCBI.h"
#include "ast/utils/Operation.h"

class Lexer;

/**
 * A CBI is a Compiler Binding Interface
 * when user calls functions in the compiler from the code written that is currently
 * being processed by the compiler, The user uses CBI to call those functions
 *
 * A CBI contains struct members and functions as function pointers in order
 * A pointer to the actual instance is placed at the end of the struct, which will be used
 * to call the actual functions, This ordering is very important
 *
 * CBI interfaces are generated from our chemical code present in lang/lib/compiler
 * using t2c with options --inline-struct-member-fn-types --cpp-like
 * a instance member field is required, which is added at the end
 */
struct LexerCBI {
    struct SourceProviderCBI* provider;
    bool(*storeVariable)(struct LexerCBI*,char*);
    bool(*storeIdentifier)(struct LexerCBI*,char*);
    bool(*lexVariableToken)(struct LexerCBI*);
    bool(*lexIdentifierToken)(struct LexerCBI*);
    bool(*lexAccessChainAfterId)(struct LexerCBI*,bool);
    bool(*lexAccessChainRecursive)(struct LexerCBI*,bool);
    bool(*lexAccessChain)(struct LexerCBI*,bool);
    bool(*lexAccessChainOrAddrOf)(struct LexerCBI*,bool);
    bool(*lexVarInitializationTokens)(struct LexerCBI*,bool,bool);
    bool(*lexAssignmentTokens)(struct LexerCBI*);
    bool(*lexLanguageOperatorToken)(struct LexerCBI*);
    bool(*lexAssignmentOperatorToken)(struct LexerCBI*);
    bool(*lexLambdaTypeTokens)(struct LexerCBI*,unsigned int);
    bool(*lexTypeTokens)(struct LexerCBI*);
    bool(*lexTopLevelStatementTokens)(struct LexerCBI*);
    bool(*lexNestedLevelStatementTokens)(struct LexerCBI*);
    bool(*lexStatementTokens)(struct LexerCBI*);
    bool(*lexOperatorToken)(struct LexerCBI*,char);
    bool(*lexOperatorTokenStr)(struct LexerCBI*,char*);
    bool(*lexOperationToken)(struct LexerCBI*,char,Operation);
    bool(*lexStrOperationToken)(struct LexerCBI*,char*,Operation);
    bool(*lexKeywordToken)(struct LexerCBI*,char*);
    void(*lexTopLevelMultipleStatementsTokens)(struct LexerCBI*);
    void(*lexTopLevelMultipleImportStatements)(struct LexerCBI*);
    void(*lexNestedLevelMultipleStatementsTokens)(struct LexerCBI*);
    void(*lexMultipleStatementsTokens)(struct LexerCBI*);
    bool(*lexSingleLineCommentTokens)(struct LexerCBI*);
    bool(*lexMultiLineCommentTokens)(struct LexerCBI*);
    bool(*lexBraceBlock)(struct LexerCBI*,char*);
    void(*lexIfExpression)(struct LexerCBI*);
    bool(*lexImportIdentifierList)(struct LexerCBI*);
    bool(*lexImportStatement)(struct LexerCBI*);
    bool(*lexReturnStatement)(struct LexerCBI*);
    bool(*lexBreakStatement)(struct LexerCBI*);
    bool(*lexTypealiasStatement)(struct LexerCBI*);
    bool(*lexContinueStatement)(struct LexerCBI*);
    void(*lexIfExprAndBlock)(struct LexerCBI*);
    bool(*lexIfBlockTokens)(struct LexerCBI*);
    bool(*lexDoWhileBlockTokens)(struct LexerCBI*);
    bool(*lexWhileBlockTokens)(struct LexerCBI*);
    bool(*lexForBlockTokens)(struct LexerCBI*);
    void(*lexParameterList)(struct LexerCBI*,bool,bool);
    bool(*lexFunctionSignatureTokens)(struct LexerCBI*);
    bool(*lexAfterFuncKeyword)(struct LexerCBI*);
    bool(*lexFunctionStructureTokens)(struct LexerCBI*,bool);
    void(*lexInterfaceBlockTokens)(struct LexerCBI*);
    bool(*lexInterfaceStructureTokens)(struct LexerCBI*);
    bool(*lexStructMemberTokens)(struct LexerCBI*);
    void(*lexStructBlockTokens)(struct LexerCBI*);
    bool(*lexStructStructureTokens)(struct LexerCBI*);
    bool(*collectStructAsLexer)(struct LexerCBI*,unsigned int,unsigned int);
    void(*lexImplBlockTokens)(struct LexerCBI*);
    bool(*lexImplTokens)(struct LexerCBI*);
    bool(*lexEnumBlockTokens)(struct LexerCBI*);
    bool(*lexEnumStructureTokens)(struct LexerCBI*);
    bool(*lexWhitespaceToken)(struct LexerCBI*);
    void(*lexWhitespaceAndNewLines)(struct LexerCBI*);
    bool(*lexStringToken)(struct LexerCBI*);
    bool(*lexCharToken)(struct LexerCBI*);
    bool(*lexAnnotationMacro)(struct LexerCBI*);
    bool(*lexNull)(struct LexerCBI*);
    bool(*lexBoolToken)(struct LexerCBI*);
    bool(*lexUnsignedIntAsNumberToken)(struct LexerCBI*);
    bool(*lexNumberToken)(struct LexerCBI*);
    bool(*lexStructValueTokens)(struct LexerCBI*);
    bool(*lexValueToken)(struct LexerCBI*);
    bool(*lexAccessChainValueToken)(struct LexerCBI*);
    bool(*lexArrayInit)(struct LexerCBI*);
    bool(*lexAccessChainOrValue)(struct LexerCBI*,bool);
    void(*lexIdentifierList)(struct LexerCBI*);
    void(*lexLambdaAfterParamsList)(struct LexerCBI*,unsigned int);
    bool(*lexLambdaValue)(struct LexerCBI*);
    bool(*lexRemainingExpression)(struct LexerCBI*,unsigned int);
    bool(*lexLambdaAfterLParen)(struct LexerCBI*);
    void(*lexParenExpressionAfterLParen)(struct LexerCBI*);
    bool(*lexParenExpression)(struct LexerCBI*);
    bool(*lexExpressionTokens)(struct LexerCBI*,bool,bool);
    bool(*lexSwitchStatementBlock)(struct LexerCBI*);
    bool(*lexTryCatchTokens)(struct LexerCBI*);
    Lexer* instance;
};

/**
 * this function should be called on cbi, to make it a valid binding
 * now cbi has been prepared, but it lacks instance, for which bind should be used
 * this should be done once, to ensure cbi methods can be called
 */
void prep_lexer_cbi(LexerCBI* cbi, SourceProviderCBI* provider);

/**
 * this function is used to connect the given lexer to the lexer cbi
 * this can be done again and again, to change instance of lexer that will receive calls
 * from cbi
 */
void bind_lexer_cbi(LexerCBI* cbi, SourceProviderCBI* provider_cbi, Lexer* lexer);