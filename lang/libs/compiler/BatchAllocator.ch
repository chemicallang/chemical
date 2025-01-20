import "@std/std.ch"
import "@std/string_view.ch"

@compiler.interface
struct BatchAllocator {

    /**
     * allocate a pointer with given object size, you must destruct the allocated pointer
     * otherwise there'll be a memory leak
     */
    func allocate_size(&self, obj_size : size_t, alignment : size_t) : *mut char;

}

// adds +1 to size to accomodate for the last null terminator \0 in string
// allocates string with size
func (allocator : &mut BatchAllocator) allocate_str_size(size : size_t) : *mut char {
    return allocator.allocate_size(#sizeof(char) * size + 1, #alignof(char))
}

func (allocator : &mut BatchAllocator) allocate_str(data : *mut char, size : size_t) : *mut char {
    const ptr = allocator.allocate_size(#sizeof(char) * (size + 1), #alignof(char))
    memcpy(ptr, data, size)
    *(ptr + size) = '\0'
    return ptr;
}

func (allocator : &mut BatchAllocator) allocate_view(view : &std::string_view) : std::string_view {
    return std::string_view(allocator.allocate_str(view.data(), view.size()), view.size());
}