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
    bool(*collectStructAsLexer)(struct Lexer*,unsigned int,unsigned int);
    bool(*lexAccessChain)(struct Lexer*,bool);
    bool(*lexAccessChainAfterId)(struct Lexer*,bool);
    bool(*lexAccessChainOrAddrOf)(struct Lexer*,bool);
    bool(*lexAccessChainOrValue)(struct Lexer*,bool);
    bool(*lexAccessChainRecursive)(struct Lexer*,bool);
    bool(*lexAccessChainValueToken)(struct Lexer*);
    bool(*lexAfterFuncKeyword)(struct Lexer*);
    bool(*lexAnnotationMacro)(struct Lexer*);
    bool(*lexArrayInit)(struct Lexer*);
    bool(*lexAssignmentOperatorToken)(struct Lexer*);
    bool(*lexAssignmentTokens)(struct Lexer*);
    bool(*lexBoolToken)(struct Lexer*);
    bool(*lexBraceBlock)(struct Lexer*,char*);
    bool(*lexBreakStatement)(struct Lexer*);
    bool(*lexCharToken)(struct Lexer*);
    bool(*lexContinueStatement)(struct Lexer*);
    bool(*lexDoWhileBlockTokens)(struct Lexer*);
    bool(*lexEnumBlockTokens)(struct Lexer*);
    bool(*lexEnumStructureTokens)(struct Lexer*);
    bool(*lexExpressionTokens)(struct Lexer*,bool,bool);
    bool(*lexForBlockTokens)(struct Lexer*);
    bool(*lexFunctionSignatureTokens)(struct Lexer*);
    bool(*lexFunctionStructureTokens)(struct Lexer*,bool);
    void(*lexIdentifierList)(struct Lexer*);
    void(*lexIdentifierToken)(struct Lexer*);
    bool(*lexIfBlockTokens)(struct Lexer*);
    void(*lexIfExprAndBlock)(struct Lexer*);
    void(*lexIfExpression)(struct Lexer*);
    void(*lexImplBlockTokens)(struct Lexer*);
    bool(*lexImplTokens)(struct Lexer*);
    bool(*lexImportIdentifierList)(struct Lexer*);
    bool(*lexImportStatement)(struct Lexer*);
    void(*lexInterfaceBlockTokens)(struct Lexer*);
    bool(*lexInterfaceStructureTokens)(struct Lexer*);
    bool(*lexKeywordToken)(struct Lexer*,char*);
    bool(*lexLambdaAfterLParen)(struct Lexer*);
    void(*lexLambdaAfterParamsList)(struct Lexer*,unsigned int);
    bool(*lexLambdaTypeTokens)(struct Lexer*,unsigned int);
    bool(*lexLambdaValue)(struct Lexer*);
    bool(*lexLanguageOperatorToken)(struct Lexer*);
    bool(*lexMultiLineCommentTokens)(struct Lexer*);
    void(*lexMultipleStatementsTokens)(struct Lexer*);
    void(*lexNestedLevelMultipleStatementsTokens)(struct Lexer*);
    bool(*lexNestedLevelStatementTokens)(struct Lexer*);
    bool(*lexNull)(struct Lexer*);
    bool(*lexNumberToken)(struct Lexer*);
    bool(*lexOperationToken)(struct Lexer*,char,Operation);
    bool(*lexOperatorToken)(struct Lexer*,char);
    bool(*lexOperatorTokenStr)(struct Lexer*,char*);
    void(*lexParameterList)(struct Lexer*,bool,bool);
    bool(*lexParenExpression)(struct Lexer*);
    void(*lexParenExpressionAfterLParen)(struct Lexer*);
    void(*lexRemainingExpression)(struct Lexer*,unsigned int);
    bool(*lexReturnStatement)(struct Lexer*);
    bool(*lexSingleLineCommentTokens)(struct Lexer*);
    bool(*lexStatementTokens)(struct Lexer*);
    bool(*lexStrOperationToken)(struct Lexer*,char*,Operation);
    bool(*lexStringToken)(struct Lexer*);
    void(*lexStructBlockTokens)(struct Lexer*);
    bool(*lexStructMemberTokens)(struct Lexer*);
    bool(*lexStructStructureTokens)(struct Lexer*);
    bool(*lexStructValueTokens)(struct Lexer*);
    bool(*lexSwitchStatementBlock)(struct Lexer*);
    void(*lexTopLevelMultipleImportStatements)(struct Lexer*);
    void(*lexTopLevelMultipleStatementsTokens)(struct Lexer*);
    bool(*lexTopLevelStatementTokens)(struct Lexer*);
    bool(*lexTryCatchTokens)(struct Lexer*);
    bool(*lexTypeTokens)(struct Lexer*);
    bool(*lexTypealiasStatement)(struct Lexer*);
    bool(*lexUnsignedIntAsNumberToken)(struct Lexer*);
    bool(*lexValueToken)(struct Lexer*);
    bool(*lexVarInitializationTokens)(struct Lexer*,bool,bool);
    bool(*lexVariableToken)(struct Lexer*);
    bool(*lexWhileBlockTokens)(struct Lexer*);
    void(*lexWhitespaceAndNewLines)(struct Lexer*);
    bool(*lexWhitespaceToken)(struct Lexer*);
    struct SourceProvider* provider;
    bool(*storeIdentifier)(struct Lexer*,char*);
    bool(*storeVariable)(struct Lexer*,char*);
    Lexer* instance;
};