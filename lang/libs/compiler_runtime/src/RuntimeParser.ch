/**
 * Runtime implementation of the compiler `Parser` interface.
 *
 * `Parser` is a static interface (via @compiler.interface). Inside a compiler
 * plugin (CBI) build the compiler provides the implementation backed by the
 * real chemical parser. In a runtime build, this struct implements the
 * interface by replaying a pre-tokenized list of tokens, so the shared
 * js_parser can be reused at runtime.
 *
 * The impl methods emit the exact same mangled symbols that the C++ binder
 * provides in CBI mode, so the shared parser code works unchanged.
 */
using namespace std;

public struct RuntimeParser {
    var current : *mut Token
    var tokens : std::vector<Token>
    var parent : *mut ASTNode
}

public func (p : &mut RuntimeParser) setup(tokens : std::vector<Token>) {
    p.tokens = tokens
    p.current = p.tokens.data() as *mut Token
}

impl Parser for RuntimeParser {

    func getTokenPtr(&self) : *mut *mut Token {
        return &raw mut self.current
    }

    func getEncodedLocation(&self, token : *Token) : ubigint {
        return 0
    }

    func getAnnotationController(&self) : *mut AnnotationController {
        return null
    }

    func getIs64Bit(&self) : bool {
        return true
    }

    func getParentNodePtr(&self) : *mut *mut ASTNode {
        return &raw mut self.parent
    }

    func getCurrentFilePath(&self) : std::string_view {
        return std::string_view("")
    }

    func parseExpression(&self, builder : *mut ASTBuilder, parseStruct : bool, parseLambda : bool) : *mut Value {
        return null
    }

    func parseExpressionOrArrayOrStruct(&self, builder : *mut ASTBuilder, parseLambda : bool) : *mut Value {
        return null
    }

    func parseNestedLevelStatement(&self, builder : *mut ASTBuilder) : *mut ASTNode {
        return null
    }

    func error_at(&self, msg : std::string_view, token : *mut Token) {
    }

}
