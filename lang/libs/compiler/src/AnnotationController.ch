public struct AnnotationDefinition {}

@compiler.interface
public struct AnnotationController {

    // returns true on failure
    func markSingle(&self, node : *mut ASTNode, def : *mut AnnotationDefinition, args : std::span<Value*>) : bool

    func mark(&self, node : *mut ASTNode, def : *mut AnnotationDefinition, args : std::span<Value*>)

    func collect(&self, node : *mut ASTNode, def : *mut AnnotationDefinition, args : std::span<Value*>)

    func markAndCollect(&self, node : *mut ASTNode, def : *mut AnnotationDefinition, args : std::span<Value*>)

    func handleAnnotation(&self, parser : *mut Parser, node : *mut ASTNode, def : *mut AnnotationDefinition, args : std::span<Value*>) : bool

    func isMarked(&self, node : *mut ASTNode, name : &std::string_view) : bool


}