import "./SourceProvider.ch"
import "./LexTokenType.ch"

@compiler:interface
struct Lexer {

    /**
     * get the source provider associated with this lexer
     */
    func provider(&self) : SourceProvider*

    /**
     * get the current tokens size
     */
    func tokens_size(&self) : size_t

    /**
     * put the given token into the tokens vector held by this Lexer
     */
    func put(&self, value : string*, token_type : LexTokenType, lineNumber : uint, lineCharNumber : uint)

    /**
     * consumes a identifier and store as a variable token
     * @return true if identifier is not empty, false if it is
     */
    func storeVariable (&self, identifier : string*) :  bool;

    /**
     * consumes a identifier and store as an identifier token
     * @return true if identifier is not empty, false if it is
     */
    func storeIdentifier (&self, identifier : string*) :  bool;

    /**
     * it will lex generic args list, it should be called after the '<'
     * after this function a '>' should be lexed as well, and then
     * compound it into a generic args list
     */
    func lexGenericArgsList (&self) :  void;

    /**
     * this will compound the generic args list
     * It expects '<' and then generic args and then '>'
     */
    func lexGenericArgsListCompound (&self) :  bool;

    /**
     * lexes a function call, after the '<' for generic start
     */
    func lexFunctionCallWithGenericArgsList (&self) :  void;

    /**
     * lexes a function call, that is args ')' without function name
     */
    func lexFunctionCall (&self, back_start : uint) :  bool;

    /**
     * lexes a keyword access specifier public, private, internal & (if protect is true, then protected)
     */
    func lexAccessSpecifier (&self, internal : bool, protect : bool) :  bool;

    /**
     * after an identifier has been consumed
     * we call this method to lex an access chain after it
     * identifier .element1.element2.element3
     * this is the method called by lexAccessChain after finding a identifier
     * @param assChain is the access chain in an assignment
     */
    func lexAccessChainAfterId (&self, lexStruct : bool, chain_length : uint) :  bool;

    /**
     * this method does not compound the access chain, so can be called recursively
     * this method is called by lexAccessChain to not compound access chains nested in it
     * @param assChain is the access chain in an assignment
     */
    func lexAccessChainRecursive (&self, lexStruct : bool, chain_length : uint) :  bool;

    /**
     * this lexes an access chain like x.y.z or just simply an identifier
     * @param assChain is the access chain in an assignment
     * @param lexStruct also lex a struct if found -> StructName { v1, v2 }
     */
    func lexAccessChain (&self, lexStruct : bool, lex_as_node : bool) :  bool;

    /**
     * it lexes a access chain, but allows a '&' operator before it to get the address of value
     * so this allows a.b.c or &a.b.c
     */
    func lexAccessChainOrAddrOf (&self, lexStruct : bool) :  bool;

    /**
     * lex allowDeclarations or initialization tokens
     * like var x : int; or var x : int = 5;
     * @param allowDeclarations when true, it will allow allowDeclarations only without the value initialization
     * like #var x : int; when false however, it'll be strict initialization
     * @return whether it was able to lex the tokens for the statement
     */
    func lexVarInitializationTokens (&self, start : uint, allowDeclarations : bool, requiredType : bool) :  bool;

    /**
     * lex assignment tokens
     * like x = 5;
     * @return whether it was able to lex teh tokens for the statement
     */
    func lexAssignmentTokens (&self) :  bool;

    /**
     * exclusive method for division operator
     * since it checks that after a single slash (/) the next is also not slash (/)
     * which would make it a comment
     */
    func lexDivisionOperatorToken (&self) :  bool;

    /**
     * This lexes a operation token in between two values
     * for example x (token) y -> x + y or x - y
     * @return whether the language operator token has been lexed
     */
    func lexLanguageOperatorToken (&self) :  bool;

    /**
     * this is invoked by expressions , when user types sum < identifier, it can mean two things
     * 1 - is sum less than identifier  (expression)
     * 2 - sum < identifier >  (generic)
     * when we encounter a less than sign, we call this function to check if there's a generic end (>) ahead
     */
    func isGenericEndAhead (&self) :  bool;

    /**
     * This lexes a operation token before assignment '='
     * for example +=, -=
     * in this case, equal sign is ignored and operation is determined solely based on the token before it
     * @return whether the language operator token has been lexed
     */
    func lexAssignmentOperatorToken (&self) :  bool;

    /**
     * lex lambda type tokens
     */
    func lexLambdaTypeTokens (&self, start : uint) :  bool;

    /**
     * will lex a generic type after identifier
     */
    func lexGenericTypeAfterId (&self, start : uint) :  bool;

    /**
     * will lex a referenced or generic type
     */
    func lexRefOrGenericType (&self) :  bool;

    /**
     * lex array and pointer types after type id
     */
    func lexArrayAndPointerTypesAfterTypeId (&self, start : uint) :  void;

    /**
     * lex type tokens
     */
    func lexTypeTokens (&self) :  bool;

    /**
     * lexes a single top level statement, top level means in file scope, These include
     * functions, structs, interfaces, implementations, enum, annotations
     * comments, variable initialization with value, constants
     * @return
     */
    func lexTopLevelStatementTokens (&self) :  bool;

    /**
     * lexes a single nested level statement, nested level means not top level (must not be in file scope)
     * These exclude functions, enum, structs, interfaces, implementations in nested scopes
     */
    func lexNestedLevelStatementTokens (&self, is_value : bool, lex_value_node : bool) :  bool;

    /**
     * lexes a single statement (of any type)
     * @return whether a statement was lexed successfully
     */
    func lexStatementTokens (&self) :  bool;

    /**
     * lexes the given operator as length 1 character operator token
     * @param op
     * @return whether the token was found
     */
    func lexOperatorToken (&self, op : char) :  bool;

    /**
     * lexes the given operator as a string operator token
     * @param op
     * @return whether the token was found
     */
    func lexOperatorTokenStr (&self, op : string*) :  bool;

    /**
     * store an operation token
     */
    func storeOperationToken (&self, token : char, op : Operation) : void

    /**
     * lexes the given operator as length 1 character operator token
     * @param op
     * @return whether the token was found
     */
    func lexOperationToken (&self, token : char, op : Operation) :  bool;

    /**
     * lexes the given operator as a string operator token
     * @return whether the token was found
     */
    func lexOperatorTokenStr2 (&self, token : string*, op : Operation) :  bool;

    /**
     * lexes a keyword token for the given keyword
     * @param keyword
     * @return  whether the keyword was found
     */
    func lexKeywordToken (&self, keyword : string*) :  bool;

    /**
     * lexes a keyword token, after which whitespace is present
     */
    func lexWSKeywordToken (&self, keyword : string*) :  bool;

    /**
     * lex a whitespaced keyword token, which may end at the given character if not whitespace
     */
    func lexWSKeywordToken2 (&self, keyword : string*, may_end_at : char) :  bool;

    /**
     * All top levels statements lexed, These include
     * functions, structs, interfaces, implementations
     * comments, variable initialization with value, constants
     */
    func lexTopLevelMultipleStatementsTokens (&self, break_at_no_stmt : bool) :  void;

    /**
     * All import statements defined at top level will be lexed
     * @param should cause error on invalid syntax, or stop
     */
    func lexTopLevelMultipleImportStatements (&self) :  void;

    /**
     * lexes a multiple nested level statement, nested level means not top level (must not be in file scope)
     * These exclude functions, enum, structs, interfaces, implementations in nested scopes
     */
    func lexNestedLevelMultipleStatementsTokens (&self, is_value : bool, lex_value_node : bool) :  void;

    /**
     * this lexes the tokens inside the body of a structure
     * this basically lexes multiple statements
     */
    func lexMultipleStatementsTokens (&self) :  void;

    /**
     * lex single comment comment
     */
    func lexSingleLineCommentTokens (&self) :  bool;

    /**
     * lex multi line comment tokens
     */
    func lexMultiLineCommentTokens (&self) :  bool;

    /**
     * lexes a brace block, { statement(s) }
     */
    func lexBraceBlock (&self, nested_lexer : (lexer : Lexer*) => void) :  bool;

    /**
     * lexes top level brace block
     */
    func lexTopLevelBraceBlock (&self) :  bool;

    /**
     * lexes a brace block, { statement(s) }
     */
    func lexBraceBlockStmts (&self) :  bool;

    /**
     * lexes a brace block or a value
     */
    func lexBraceBlockOrSingleStmt (&self, is_value : bool, lex_value_node : bool) :  bool;

    /**
     * lexes import identifier list example : { something, something }
     */
    func lexImportIdentifierList (&self) :  bool;

    /**
     * lexes import statement
     * @return
     */
    func lexImportStatement (&self) :  bool;

    /**
     * lexes destruct statement
     */
    func lexDestructStatement (&self) :  bool;

    /**
     * lexes return statement
     * @return
     */
    func lexReturnStatement (&self) :  bool;

    /**
     * lexes init block
     */
    func lexConstructorInitBlock (&self) :  bool;

    /**
     * lexes unsafe block
     */
    func lexUnsafeBlock (&self) :  bool;

    /**
     * lexes break statement
     * @return
     */
    func lexBreakStatement (&self) :  bool;

    /**
     * lexes a single typealias statement
     */
    func lexTypealiasStatement (&self, start : uint) :  bool;

    /**
     * lexes continue statement
     * @return
     */
    func lexContinueStatement (&self) :  bool;

    /**
     * lexes a single if expr and the body without else if or else
     * meaning '(' expr ')' '{' body '}'
     * @return
     */
    func lexIfExprAndBlock (&self, start : uint, is_value : bool, lex_value_node : bool, top_level : bool) :  void;

    /**
     * lex if block
     */
    func lexIfBlockTokens (&self, is_value : bool, lex_value_node : bool, top_level : bool) :  bool;

    /**
     * lex do while block
     */
    func lexDoWhileBlockTokens (&self) :  bool;

    /**
     * lex while block
     */
    func lexWhileBlockTokens (&self) :  bool;

    /**
     * lex for block tokens
     */
    func lexForBlockTokens (&self) :  bool;

    /**
     * lex loop block tokens
     */
    func lexLoopBlockTokens (&self, is_value : bool) :  bool;

    /**
     * lex parameter list
     */
    func lexParameterList (&self, optionalTypes : bool, defValues : bool, lexSelfParam : bool, variadicParam : bool) :  bool;

    /**
    * lexes a function signature with parameters
    * @return
    */
    func lexFunctionSignatureTokens (&self) :  bool;

    /**
    * lexes generic parameters list
    * @return
    */
    func lexGenericParametersList (&self) :  bool;

    /**
     * lex after func keyword has been incremented
     */
    func lexAfterFuncKeyword (&self, allow_extensions : bool) :  bool;

    /**
     * lexes a function block with parameters
     * @param allow_declaration allows a declaration, without body of the function that is
     * @return
     */
    func lexFunctionStructureTokens (&self, start : uint, allow_declaration : bool, allow_extensions : bool) :  bool;

    /**
     * lexes interface block, this means { member(s) }
     * without the `interface` keyword and name identifier
     * @return
     */
    func lexInterfaceBlockTokens (&self) :  void;

    /**
     * lexes a interface structure
     * @return
     */
    func lexInterfaceStructureTokens (&self, start : uint) :  bool;

    /**
     * lexes a namespace
     * @return
     */
    func lexNamespaceTokens (&self, start : uint) :  bool;

    /**
     * lexes a single member of the struct
     * @return
     */
    func lexStructMemberTokens (&self) :  bool;

    /**
     * lexes struct block, this means { member(s) }
     * without the `struct` keyword and name identifier
     * @return
     */
    func lexStructBlockTokens (&self) :  void;

    /**
     * lexes a struct block
     * @return
     */
    func lexStructStructureTokens (&self, start : uint, unnamed : bool, direct_init : bool) :  bool;

    /**
     * lexes a variant member
     * @return
     */
    func lexVariantMemberTokens (&self) :  bool;

    /**
     * lexes a variant block
     * @return
     */
    func lexVariantBlockTokens (&self) :  void;

    /**
     * lexes a variant decl
     * @return
     */
    func lexVariantStructureTokens (&self, start : uint) :  bool;

    /**
     * lexes a union block
     * @return
     */
    func lexUnionBlockTokens (&self) : void;

    /**
     * lexes a union decl
     * @return
     */
    func lexUnionStructureTokens (&self, start : bool, unnamed : bool, direct_init : bool) : bool;

    /**
     * lexes a impl block tokens
     * @return
     */
    func lexImplBlockTokens (&self) :  void;

    /**
     * lexes a impl block
     * @return
     */
    func lexImplTokens (&self) :  bool;

    /**
     * lexes an enum block, this means { enum(s) }
     * without the `enum` keyword and name identifier
     * @return
     */
    func lexEnumBlockTokens (&self) :  bool;

    /**
     * lexes a enum block
     * @return
     */
    func lexEnumStructureTokens (&self, start : uint) :  bool;

    /**
     * lex whitespace tokens
     * @return
     */
    func readWhitespace (&self) :  bool;

    /**
     * lex whitespace tokens
     * @return
     */
    func lexWhitespaceToken (&self) :  bool;

    /**
     * a utility function to lex whitespace tokens and also skip new lines
     */
    func lexWhitespaceAndNewLines (&self) :  void;

    /**
     * lexes a string token, string is enclosed inside double quotes
     * @return whether a string has been lexed
     */
    func lexStringToken (&self) :  bool;

    /**
     * lexes a char token, char is enclosed inside single quotes
     * @return whether a char has been lexed
     */
    func lexCharToken (&self) :  bool;

    /**
     * lex hash macro
     * @return
     */
    func lexAnnotationMacro (&self) :  bool;

    /**
     * lexes a null value
     */
    func lexNull (&self) :  bool;

    /**
     * lexes a bool, true or false
     * @return whether a bool has been lexed
     */
    func lexBoolToken (&self) :  bool;

    /**
      * lex a unsigned int as number token
      */
    func lexUnsignedIntAsNumberToken (&self) :  bool;

    /**
     * lex an number token
     * @return whether a token was lexed or not
     */
    func lexNumberToken (&self) :  bool;

    /**
     * lexes tokens for a complete struct object initialization
     * @return
     */
    func lexStructValueTokens (&self, back_start : uint) :  bool;

    /**
     * lexes value tokens like integer, string
     */
    func lexValueToken (&self) :  bool;

    /**
     * lexes switch case value
     */
    func lexSwitchCaseValue (&self) : bool;

    /**
     * values like integer and string, but appearing in access chain
     */
    func lexAccessChainValueToken (&self) :  bool;

    /**
     * lexes array syntax values like [1,2,3,4]
     * for easy array creation
     * @return
     */
    func lexArrayInit (&self) :  bool;

    /**
     * lexes access chain like x.y.z or a value like 10, could be int, string, char
     * @return
     */
    func lexAccessChainOrValue (&self, lexStruct : bool) :  bool;

    /**
     * lex value node
     * @return
     */
    func lexValueNode (&self) :  bool;

    /**
     * lexes a identifier list like id1,id2
     */
    func lexIdentifierList (&self) :  void;

    /**
     * lex lambda after params list
     */
    func lexLambdaAfterParamsList (&self, start : uint) :  void;

    /**
     * lexes a single lambda function (PARAM1, PARAM2)[CAP1, CAP2] => {}
     */
    func lexLambdaValue (&self) :  bool;

    /**
     * lexes remaining expression, this is used by lexExpressionTokens
     * this lexes the expression tokens after the first identifier / value
     * for example in expression a + b, after lexing a + b will lexed by this function
     * @param start is the start of the expression, index in tokens vector !
     */
    func lexRemainingExpression (&self, start : uint) :  bool;

    /**
     * it will lex a lambda meaning '() => {}' in a paren expression
     * it assumes you've already consumed '('
     */
    func lexLambdaOrExprAfterLParen (&self) :  bool;

    /**
     * it will lex a paren expression, meaning '(' expr ')'
     * it assumes you've already consumed '('
     */
    func lexParenExpressionAfterLParen (&self) :  void;

    /**
     * lex a parenthesized expression '(x + 5)'
     */
    func lexParenExpression (&self) :  bool;

    /**
     * lexes an expression token which can contain access chain and values
     * @return whether an expression has been lexed, the expression can also be a single identifier or value
     */
    func lexExpressionTokens (&self, lexStruct : bool, lambda : bool) :  bool;

    /**
     * lexes switch block
     */
    func lexSwitchStatementBlock (&self, is_value : bool, lex_value_node : bool) :  bool;

    /**
     * lexes try catch block statements
     */
    func lexTryCatchTokens (&self) :  bool;

    /**
     * lexes using statement
     */
    func lexUsingStatement (&self) :  bool;


}