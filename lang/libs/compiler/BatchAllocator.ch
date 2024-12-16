import "@std/std.ch"

@compiler.interface
struct BatchAllocator {

    /**
     * allocate a pointer with given object size, you must destruct the allocated pointer
     * otherwise there'll be a memory leak
     */
    func allocate_size(&self, obj_size : size_t, alignment : size_t) : *mut char;

}