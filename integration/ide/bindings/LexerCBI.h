// Copyright (c) Qinetik 2024.

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
    struct SourceProvider* provider;
    bool(*storeVariable)(struct Lexer*,char*);
    bool(*storeIdentifier)(struct Lexer*,char*);
    bool(*lexVariableToken)(struct Lexer*);
    void(*lexIdentifierToken)(struct Lexer*);
    bool(*lexAccessChainAfterId)(struct Lexer*,bool);
    bool(*lexAccessChainRecursive)(struct Lexer*,bool);
    bool(*lexAccessChain)(struct Lexer*,bool);
    bool(*lexAccessChainOrAddrOf)(struct Lexer*,bool);
    bool(*lexVarInitializationTokens)(struct Lexer*,bool,bool);
    bool(*lexAssignmentTokens)(struct Lexer*);
    bool(*lexLanguageOperatorToken)(struct Lexer*);
    bool(*lexAssignmentOperatorToken)(struct Lexer*);
    bool(*lexLambdaTypeTokens)(struct Lexer*,unsigned int);
    bool(*lexTypeTokens)(struct Lexer*);
    bool(*lexTopLevelStatementTokens)(struct Lexer*);
    bool(*lexNestedLevelStatementTokens)(struct Lexer*);
    bool(*lexStatementTokens)(struct Lexer*);
    bool(*lexOperatorToken)(struct Lexer*,char);
    bool(*lexOperatorTokenStr)(struct Lexer*,char*);
    bool(*lexOperationToken)(struct Lexer*,char,Operation);
    bool(*lexStrOperationToken)(struct Lexer*,char*,Operation);
    bool(*lexKeywordToken)(struct Lexer*,char*);
    void(*lexTopLevelMultipleStatementsTokens)(struct Lexer*);
    void(*lexTopLevelMultipleImportStatements)(struct Lexer*);
    void(*lexNestedLevelMultipleStatementsTokens)(struct Lexer*);
    void(*lexMultipleStatementsTokens)(struct Lexer*);
    bool(*lexSingleLineCommentTokens)(struct Lexer*);
    bool(*lexMultiLineCommentTokens)(struct Lexer*);
    bool(*lexBraceBlock)(struct Lexer*,char*);
    void(*lexIfExpression)(struct Lexer*);
    bool(*lexImportIdentifierList)(struct Lexer*);
    bool(*lexImportStatement)(struct Lexer*);
    bool(*lexReturnStatement)(struct Lexer*);
    bool(*lexBreakStatement)(struct Lexer*);
    bool(*lexTypealiasStatement)(struct Lexer*);
    bool(*lexContinueStatement)(struct Lexer*);
    void(*lexIfExprAndBlock)(struct Lexer*);
    bool(*lexIfBlockTokens)(struct Lexer*);
    bool(*lexDoWhileBlockTokens)(struct Lexer*);
    bool(*lexWhileBlockTokens)(struct Lexer*);
    bool(*lexForBlockTokens)(struct Lexer*);
    void(*lexParameterList)(struct Lexer*,bool,bool);
    bool(*lexFunctionSignatureTokens)(struct Lexer*);
    bool(*lexAfterFuncKeyword)(struct Lexer*);
    bool(*lexFunctionStructureTokens)(struct Lexer*,bool);
    void(*lexInterfaceBlockTokens)(struct Lexer*);
    bool(*lexInterfaceStructureTokens)(struct Lexer*);
    bool(*lexStructMemberTokens)(struct Lexer*);
    void(*lexStructBlockTokens)(struct Lexer*);
    bool(*lexStructStructureTokens)(struct Lexer*);
    bool(*collectStructAsLexer)(struct Lexer*,unsigned int,unsigned int);
    void(*lexImplBlockTokens)(struct Lexer*);
    bool(*lexImplTokens)(struct Lexer*);
    bool(*lexEnumBlockTokens)(struct Lexer*);
    bool(*lexEnumStructureTokens)(struct Lexer*);
    bool(*lexWhitespaceToken)(struct Lexer*);
    void(*lexWhitespaceAndNewLines)(struct Lexer*);
    bool(*lexStringToken)(struct Lexer*);
    bool(*lexCharToken)(struct Lexer*);
    bool(*lexAnnotationMacro)(struct Lexer*);
    bool(*lexNull)(struct Lexer*);
    bool(*lexBoolToken)(struct Lexer*);
    bool(*lexUnsignedIntAsNumberToken)(struct Lexer*);
    bool(*lexNumberToken)(struct Lexer*);
    bool(*lexStructValueTokens)(struct Lexer*);
    bool(*lexValueToken)(struct Lexer*);
    bool(*lexAccessChainValueToken)(struct Lexer*);
    bool(*lexArrayInit)(struct Lexer*);
    bool(*lexAccessChainOrValue)(struct Lexer*,bool);
    void(*lexIdentifierList)(struct Lexer*);
    void(*lexLambdaAfterParamsList)(struct Lexer*,unsigned int);
    bool(*lexLambdaValue)(struct Lexer*);
    void(*lexRemainingExpression)(struct Lexer*,unsigned int);
    bool(*lexLambdaAfterLParen)(struct Lexer*);
    void(*lexParenExpressionAfterLParen)(struct Lexer*);
    bool(*lexParenExpression)(struct Lexer*);
    bool(*lexExpressionTokens)(struct Lexer*,bool,bool);
    bool(*lexSwitchStatementBlock)(struct Lexer*);
    bool(*lexTryCatchTokens)(struct Lexer*);
    Lexer* instance;
};

void init_lexer_cbi(LexerCBI* cbi, Lexer* lexer) {
    cbi->instance = lexer;

}