public struct ASTNode {

    func getEncodedLocation(&self) : ubigint

    func getKind(&self) : ASTNodeKind

    // may return null
    func getParent(&self) : *mut ASTNode

    func child(&self, name : &std::string_view) : *mut ASTNode

    func getAccessSpecifier(&self) : AccessSpecifier

}

public func (node : &mut ASTNode) getModScope() : *mut ModuleScope {
    const p = node.getParent();
    if(p == null) {
        return null;
    } else if(p.getKind() == ASTNodeKind.ModuleScope) {
        return p as *mut ModuleScope
    } else {
        return p.getModScope()
    }
}