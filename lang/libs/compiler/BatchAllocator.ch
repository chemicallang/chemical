import "@std/std.ch"

@compiler.interface
struct BatchAllocator {

    /**
     * allocate a pointer with given object size, you must destruct the allocated pointer
     * otherwise there'll be a memory leak
     */
    func allocate_size(obj_size : size_t, alignment : size_t) : *mut char;

    /**
     * allocate the given string on the allocator and get a c string
     */
    func allocate_str(data : *char, size : size_t) : *mut char

}