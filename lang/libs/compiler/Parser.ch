import "Token.ch"
import "ASTBuilder.ch"
import "@std/string_view.h"

@compiler.interface
struct Parser {

    func getTokenPtr(&self) : Token**

    // nodes are retained across multiple modules
    func getGlobalBuilder(&self) : ASTBuilder*

    // nodes are not retained after generating this module's code
    // only nodes 'internal' to module (not public) should go into this
    func getModuleBuilder(&self) : ASTBuilder*

    func getIs64Bit(&self) : bool

    func getParentNodePtr(&self) : ASTNode**

    func getCurrentFuncTypePtr(&self) : FunctionType**

    func getCurrentLoopNodePtr(&self) : LoopASTNode**

    func getCurrentFilePath(&self) : std::string_view

}

func (parser : &Parser) getToken() : Token* {
    return parser.getTokenPtr();
}

func (parser : &Parser) increment() {
    var ptr = parser.getTokenPtr();
    *ptr = (*ptr) + 1
}

func (Parser : &Parser) increment_if(type : int) : bool {
    var ptr = parser.getTokenPtr();
    var token = *ptr;
    if(token.type == type) {
        *ptr = (*ptr) + 1;
        return true;
    } else {
        return false;
    }
}

func (parser : &Parser) getParentNode() : ASTNode* {
    return *parser.getParentNodePtr();
}

func (parser : &Parser) getCurrentFuncType() : FunctionType* {
    return *parser.getCurrentFuncTypePtr();
}

func (parser : &Parser) getCurrentLoopNode() : LoopASTNode* {
    return *parser.getCurrentLoopNodePtr();

}

func (parser : &Parser) setParentNode(node : ASTNode*) {
    var ptr = parser.getParentNodePtr();
    *ptr = node;
}

func (parser : &Parser) setCurrentFuncType(type : FunctionType*) {
    var ptr = parser.getCurrentFuncTypePtr();
    *ptr = type;
}

func (parser : &Parser) setCurrentLoopNode(node : LoopASTNode*) {
    var ptr = parser.getCurrentLoopNodePtr();
    *ptr = node;
}