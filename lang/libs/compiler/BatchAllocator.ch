import "@std/std.ch"
import "@std/string_view.ch"

@compiler.interface
struct BatchAllocator {

    /**
     * allocate a pointer with given object size, you must destruct the allocated pointer
     * otherwise there'll be a memory leak
     */
    func allocate_size(&self, obj_size : size_t, alignment : size_t) : *mut char;

    /**
     * allocate a string on this allocator
     */
    func allocate_str(&self, data : *mut char, size : size_t) : *mut char

}

func (allocator : &mut BatchAllocator) allocate_view(view : &std::string_view) : std::string_view {
    return std::string_view(allocator.allocate_str(view.data(), view.size()), view.size());
}