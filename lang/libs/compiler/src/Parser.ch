
@compiler.interface
public interface Parser {

    func getTokenPtr(&self) : *mut *mut Token

    func getEncodedLocation(&self, token : *Token) : ubigint

    func getAnnotationController(&self) : *mut AnnotationController

    func getIs64Bit(&self) : bool

    func getParentNodePtr(&self) : *mut *mut ASTNode

    func getCurrentFilePath(&self) : std::string_view

    func parseExpression(&self, builder : *mut ASTBuilder, parseStruct : bool = false, parseLambda : bool = false) : *mut Value

    func parseNestedLevelStatement(&self, builder : *mut ASTBuilder) : *mut ASTNode

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
