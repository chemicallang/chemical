import "@std/std.ch"
import "./BatchAllocator.ch"

struct ASTAllocator : BatchAllocator {

    /**
     * this specific method must be used only with nodes that are part of the AST
     * given below is an extension function that takes a generic node as a generic argument
     * which should be used
     */
    func allocate_node(&self, obj_size : size_t, alignment : size_t) : *mut char;

}

func <T> (allocator : &ASTAllocator) allocate() : *mut T {
    return allocator.allocate_node(#sizeof(T), #alignof(T)) as *mut T;
}