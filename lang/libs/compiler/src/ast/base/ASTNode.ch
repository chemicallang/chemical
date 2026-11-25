public struct ASTNode {

    func getEncodedLocation(&self) : ubigint

    func getKind(&self) : ASTNodeKind

    // may return null
    func getParent(&self) : *mut ASTNode

    func child(&self, name : &std::string_view) : *mut ASTNode

    // The declaration's own name (generic declarations resolve to their master
    // implementation). Empty for node kinds that are not named declarations.
    func getNodeIdentifier(&self) : std::string_view

    func getAccessSpecifier(&self) : AccessSpecifier

}

public func (node : &mut ASTNode) getModScope() : *mut ModuleScope {
        unsafe {
        const p = node.getParent();
        if(p == null) {
            return null;
        } else if(p.getKind() == ASTNodeKind.ModuleScope) {
            return p as *mut ModuleScope
        } else {
            return p.getModScope()
        }

        }
    }