public struct AnnotationDefinition {}

@compiler.interface
public interface AnnotationController {

    func getDefinition(&self, name : &std::string_view) : *mut AnnotationDefinition;

    // returns true on failure
    func markSingle(&self, node : *mut ASTNode, def : *mut AnnotationDefinition, args : std::span<*mut Value>) : bool

    func mark(&self, node : *mut ASTNode, def : *mut AnnotationDefinition, args : std::span<*mut Value>)

    func collect(&self, node : *mut ASTNode, def : *mut AnnotationDefinition, args : std::span<*mut Value>)

    func markAndCollect(&self, node : *mut ASTNode, def : *mut AnnotationDefinition, args : std::span<*mut Value>)

    func handleAnnotation(&self, parser : *mut Parser, node : *mut ASTNode, def : *mut AnnotationDefinition, args : std::span<*mut Value>) : bool

    func isMarked(&self, node : *mut ASTNode, name : &std::string_view) : bool


}