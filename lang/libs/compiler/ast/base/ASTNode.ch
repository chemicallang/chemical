import "ASTNodeKind.ch"

struct ASTNode {

    func getKind(&self) : ASTNodeKind

    func declare_top_level(&self, ptr_ref : **mut ASTNode);

    func declare_and_link(&self, ptr_ref : **mut ASTNode);

}
