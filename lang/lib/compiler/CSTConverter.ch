import "./ASTBuilder.ch"

@compiler:interface
public struct CSTConverter : public ASTBuilder {

    func put_value(&self, value : Value*, token : CSTToken*);

    func put_node(&self, node : ASTNode*, token : CSTToken*);

    func put_type(&self, type : BaseType*, token : CSTToken*);

    func pop_last_value(&self) : Value*;

    func pop_last_node(&self) : ASTNode*;

    func pop_last_type(&self) : BaseType*;

    func visit(&self, token : CSTToken*);

}