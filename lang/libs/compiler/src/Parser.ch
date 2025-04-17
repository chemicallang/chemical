import "@std/string_view.ch"

@compiler.interface
public struct Parser {

    func getTokenPtr(&self) : *mut *mut Token

    // nodes are retained across multiple modules
    func getGlobalBuilder(&self) : *mut ASTBuilder

    // nodes are not retained after generating this module's code
    // only nodes 'internal' to module (not public) should go into this
    func getModuleBuilder(&self) : *mut ASTBuilder

    func getIs64Bit(&self) : bool

    func getParentNodePtr(&self) : *mut *mut ASTNode

    func getCurrentFilePath(&self) : std::string_view

    func parseExpression(&self, builder : *mut ASTBuilder, parseStruct : bool = false, parseLambda : bool = false) : *mut Value

    func error_at(&self, msg : std::string_view, token : *mut Token);

}

public func (parser : &mut Parser) getToken() : *mut Token {
    return *parser.getTokenPtr();
}

public func (parser : &mut Parser) setToken(token : *mut Token) {
    var ptr = parser.getTokenPtr()
    *ptr = token;
}

public func (parser : &mut Parser) increment() {
    var ptr = parser.getTokenPtr();
    *ptr = (*ptr) + 1
}

public func (parser : &mut Parser) increment_if(type : int) : bool {
    var ptr = parser.getTokenPtr();
    var token = *ptr;
    if(token.type == type) {
        *ptr = (*ptr) + 1;
        return true;
    } else {
        return false;
    }
}

public func (parser : &mut Parser) get_incrementing_if(type : int) : *mut Token {
    var ptr = parser.getTokenPtr();
    var token = *ptr;
    if(token.type == type) {
        *ptr = (*ptr) + 1;
        return token;
    } else {
        return null;
    }
}

public func (parser : &mut Parser) getParentNode() : *mut ASTNode {
    return *parser.getParentNodePtr();
}

public func (parser : &mut Parser) setParentNode(node : *mut ASTNode) {
    var ptr = parser.getParentNodePtr();
    *ptr = node;
}

public func (parser : &mut Parser) error(msg : std::string_view) {
    return parser.error_at(msg, parser.getToken())
}
