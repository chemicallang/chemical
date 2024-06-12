import "./SourceProvider.ch"

// TODO change this, enum's don't support @scope:lexer
typealias Operation = uint

@scope:lexer
struct Lexer {

    var provider : SourceProvider;

    /**
     * consumes a identifier and store as a variable token
     * @return true if identifier is not empty, false if it is
     */
    var storeVariable : (&self, identifier : string) => bool;

    /**
     * consumes a identifier and store as an identifier token
     * @return true if identifier is not empty, false if it is
     */
    var storeIdentifier : (&self, identifier : string) => bool;

    /**
     * lex a variable token into tokens until the until character occurs
     * only lexes the token if the identifier is not empty
     */
    var lexVariableToken : (&self) => bool;

    /**
     * lex an identifier token into tokens until the until character occurs
     * only lexes the token if the identifier is not empty
     */
    var lexIdentifierToken : (&self) => bool;

    /**
     * after an identifier has been consumed
     * we call this method to lex an access chain after it
     * identifier .element1.element2.element3
     * this is the method called by lexAccessChain after finding a identifier
     * @param assChain is the access chain in an assignment
     */
    var lexAccessChainAfterId : (&self, lexStruct : bool) => bool;

    /**
     * this method does not compound the access chain, so can be called recursively
     * this method is called by lexAccessChain to not compound access chains nested in it
     * @param assChain is the access chain in an assignment
     */
    var lexAccessChainRecursive : (&self, lexStruct : bool) => bool;

    /**
     * this lexes an access chain like x.y.z or just simply an identifier
     * @param assChain is the access chain in an assignment
     * @param lexStruct also lex a struct if found -> StructName { v1, v2 }
     * @return
     */
    var lexAccessChain : (&self, lexStruct : bool) => bool;

    /**
     * it lexes a access chain, but allows a '&' operator before it to get the address of value
     * so this allows a.b.c or &a.b.c
     */
    var lexAccessChainOrAddrOf : (&self, lexStruct : bool) => bool;

    /**
     * lex allowDeclarations or initialization tokens
     * like var x : int; or var x : int = 5;
     * @param allowDeclarations when true, it will allow allowDeclarations only without the value initialization
     * like #var x : int; when false however, it'll be strict initialization
     * @return whether it was able to lex the tokens for the statement
     */
    var lexVarInitializationTokens : (&self, allowDeclarations : bool, requiredType : bool) => bool;

    /**
     * lex assignment tokens
     * like x = 5;
     * @return whether it was able to lex teh tokens for the statement
     */
    var lexAssignmentTokens : (&self) => bool;

    /**
     * This lexes a operation token in between two values
     * for example x (token) y -> x + y or x - y
     * @return whether the language operator token has been lexed
     */
    var lexLanguageOperatorToken : (&self) => bool;

    /**
     * This lexes a operation token before assignment '='
     * for example +=, -=
     * in this case, equal sign is ignored and operation is determined solely based on the token before it
     * @return whether the language operator token has been lexed
     */
    var lexAssignmentOperatorToken : (&self) => bool;

    /**
     * lex lambda type tokens
     */
    var lexLambdaTypeTokens : (&self, start : uint) => bool;

    /**
     * lex type tokens
     */
    var lexTypeTokens : (&self) => bool;

    /**
     * lexes a single top level statement, top level means in file scope, These include
     * functions, structs, interfaces, implementations, enum, annotations
     * comments, variable initialization with value, constants
     * @return
     */
    var lexTopLevelStatementTokens : (&self) => bool;

    /**
     * lexes a single nested level statement, nested level means not top level (must not be in file scope)
     * These exclude functions, enum, structs, interfaces, implementations in nested scopes
     */
    var lexNestedLevelStatementTokens : (&self) => bool;

    /**
     * lexes a single statement (of any type)
     * @return whether a statement was lexed successfully
     */
    var lexStatementTokens : (&self) => bool;

    /**
     * lexes the given operator as length 1 character operator token
     * @param op
     * @return whether the token was found
     */
    var lexOperatorToken : (&self, op : char) => bool;

    /**
     * lexes the given operator as a string operator token
     * @param op
     * @return whether the token was found
     */
    var lexOperatorTokenStr : (&self, op : string) => bool;

    /**
     * lexes the given operator as length 1 character operator token
     * @param op
     * @return whether the token was found
     */
    var lexOperationToken : (&self, token : char, op : Operation) => bool;

    /**
     * lexes the given operator as a string operator token
     * @param op
     * @return whether the token was found
     */
    var lexStrOperationToken : (&self, token : string, op : Operation) => bool;

    /**
     * lexes a keyword token for the given keyword
     * @param keyword
     * @return  whether the keyword was found
     */
    var lexKeywordToken : (&self, keyword : string) => bool;

    /**
     * All top levels statements lexed, These include
     * functions, structs, interfaces, implementations
     * comments, variable initialization with value, constants
     */
    var lexTopLevelMultipleStatementsTokens : (&self) => void;

    /**
     * All import statements defined at top level will be lexed
     * @param should cause error on invalid syntax, or stop
     */
    var lexTopLevelMultipleImportStatements : (&self) => void;

    /**
     * lexes a multiple nested level statement, nested level means not top level (must not be in file scope)
     * These exclude functions, enum, structs, interfaces, implementations in nested scopes
     */
    var lexNestedLevelMultipleStatementsTokens : (&self) => void;

    /**
     * this lexes the tokens inside the body of a structure
     * this basically lexes multiple statements
     */
    var lexMultipleStatementsTokens : (&self) => void;

    /**
     * lex single comment comment
     */
    var lexSingleLineCommentTokens : (&self) => bool;

    /**
     * lex multi line comment tokens
     */
    var lexMultiLineCommentTokens : (&self) => bool;

    /**
     * lexes a brace block, { statement(s) }
     */
    var lexBraceBlock : (&self, forThing : string) => bool;

    /**
     * lexes an expression for if statement including parens '(' expr ')'
     * @return
     */
    var lexIfExpression : (&self) => void;

    /**
     * lexes import identifier list example : { something, something }
     */
    var lexImportIdentifierList : (&self) => bool;

    /**
     * lexes import statement
     * @return
     */
    var lexImportStatement : (&self) => bool;

    /**
     * lexes return statement
     * @return
     */
    var lexReturnStatement : (&self) => bool;

    /**
     * lexes break statement
     * @return
     */
    var lexBreakStatement : (&self) => bool;

    /**
     * lexes a single typealias statement
     */
    var lexTypealiasStatement : (&self) => bool;

    /**
     * lexes continue statement
     * @return
     */
    var lexContinueStatement : (&self) => bool;

    /**
     * lexes a single if expr and the body without else if or else
     * meaning '(' expr ')' '{' body '}'
     * @return
     */
    var lexIfExprAndBlock : (&self) => void;

    /**
     * lex if block
     */
    var lexIfBlockTokens : (&self) => bool;

    /**
     * lex do while block
     */
    var lexDoWhileBlockTokens : (&self) => bool;

    /**
     * lex while block
     */
    var lexWhileBlockTokens : (&self) => bool;

    /**
     * lex for block tokens
     */
    var lexForBlockTokens : (&self) => bool;

    /**
     * lex parameter list
     */
    var lexParameterList : (&self, optionalTypes : bool, defValues : bool) => void;

    /**
    * lexes a function signature with parameters
    * @return
    */
    var lexFunctionSignatureTokens : (&self) => bool;

    /**
     * lex after func keyword has been incremented
     */
    var lexAfterFuncKeyword : (&self) => bool;

    /**
     * lexes a function block with parameters
     * @param allow_declaration allows a declaration, without body of the function that is
     * @return
     */
    var lexFunctionStructureTokens : (&self, allow_declaration : bool) => bool;

    /**
     * lexes interface block, this means { member(s) }
     * without the `interface` keyword and name identifier
     * @return
     */
    var lexInterfaceBlockTokens : (&self) => void;

    /**
     * lexes a interface structure
     * @return
     */
    var lexInterfaceStructureTokens : (&self) => bool;

    /**
     * lexes a single member of the struct
     * @return
     */
    var lexStructMemberTokens : (&self) => bool;

    /**
     * lexes struct block, this means { member(s) }
     * without the `struct` keyword and name identifier
     * @return
     */
    var lexStructBlockTokens : (&self) => void;

    /**
     * lexes a struct block
     * @return
     */
    var lexStructStructureTokens : (&self) => bool;

    /**
     * this will try to collect current struct as a lexer
     * @param start is the start position inside the tokens vector
     */
    var collectStructAsLexer : (&self, start : uint, end : uint) => bool;

    /**
     * lexes a impl block tokens
     * @return
     */
    var lexImplBlockTokens : (&self) => void;

    /**
     * lexes a impl block
     * @return
     */
    var lexImplTokens : (&self) => bool;

    /**
     * lexes an enum block, this means { enum(s) }
     * without the `enum` keyword and name identifier
     * @return
     */
    var lexEnumBlockTokens : (&self) => bool;

    /**
     * lexes a enum block
     * @return
     */
    var lexEnumStructureTokens : (&self) => bool;

    /**
     * lex whitespace tokens
     * @return
     */
    var lexWhitespaceToken : (&self) => bool;

    /**
     * a utility function to lex whitespace tokens and also skip new lines
     */
    var lexWhitespaceAndNewLines : (&self) => void;

    /**
     * lexes a string token, string is enclosed inside double quotes
     * @return whether a string has been lexed
     */
    var lexStringToken : (&self) => bool;

    /**
     * lexes a char token, char is enclosed inside single quotes
     * @return whether a char has been lexed
     */
    var lexCharToken : (&self) => bool;

    /**
     * lex hash macro
     * @return
     */
    var lexAnnotationMacro : (&self) => bool;

    /**
     * lexes a null value
     */
    var lexNull : (&self) => bool;

    /**
     * lexes a bool, true or false
     * @return whether a bool has been lexed
     */
    var lexBoolToken : (&self) => bool;

    /**
      * lex a unsigned int as number token
      */
    var lexUnsignedIntAsNumberToken : (&self) => bool;

    /**
     * lex an number token
     * @return whether a token was lexed or not
     */
    var lexNumberToken : (&self) => bool;

    /**
     * lexes tokens for a complete struct object initialization
     * @return
     */
    var lexStructValueTokens : (&self) => bool;

    /**
     * lexes value tokens like integer, string
     */
    var lexValueToken : (&self) => bool;

    /**
     * values like integer and string, but appearing in access chain
     */
    var lexAccessChainValueToken : (&self) => bool;

    /**
     * lexes array syntax values like [1,2,3,4]
     * for easy array creation
     * @return
     */
    var lexArrayInit : (&self) => bool;

    /**
     * lexes access chain like x.y.z or a value like 10, could be int, string, char
     * @return
     */
    var lexAccessChainOrValue : (&self, lexStruct : bool) => bool;

    /**
     * lexes a identifier list like id1,id2
     */
    var lexIdentifierList : (&self) => void;

    /**
     * lex lambda after params list
     */
    var lexLambdaAfterParamsList : (&self, start : uint) => void;

    /**
     * lexes a single lambda function (PARAM1, PARAM2)[CAP1, CAP2] => {}
     */
    var lexLambdaValue : (&self) => bool;

    /**
     * lexes remaining expression, this is used by lexExpressionTokens
     * this lexes the expression tokens after the first identifier / value
     * for example in expression a + b, after lexing a + b will lexed by this function
     * @param start is the start of the expression, index in tokens vector !
     */
    var lexRemainingExpression : (&self, start : uint) => bool;

    /**
     * it will lex a lambda meaning '() => {}' in a paren expression
     * it assumes you've already consumed '('
     */
    var lexLambdaAfterLParen : (&self) => bool;

    /**
     * it will lex a paren expression, meaning '(' expr ')'
     * it assumes you've already consumed '('
     */
    var lexParenExpressionAfterLParen : (&self) => void;

    /**
     * lex a parenthesized expression '(x + 5)'
     */
    var lexParenExpression : (&self) => bool;

    /**
     * lexes an expression token which can contain access chain and values
     * @return whether an expression has been lexed, the expression can also be a single identifier or value
     */
    var lexExpressionTokens : (&self, lexStruct : bool, lambda : bool) => bool;

    /**
     * lexes switch block
     */
    var lexSwitchStatementBlock : (&self) => bool;

    /**
     * lexes try catch block statements
     */
    var lexTryCatchTokens : (&self) => bool;


}