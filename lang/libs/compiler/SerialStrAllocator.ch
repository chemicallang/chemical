import "@std/string_view.ch"

@compiler.interface
struct SerialStrAllocator {

    /**
     * Dispose the current string you've built and start a new one
     */
    func deallocate(&self);

    /**
     * Get the view into current string without disposing
     */
    func current_view(&self) : std::string_view

    /**
     * Get the finalized view of the string you've build
     * After this call append will put character into a new string
     */
    func finalize_view(&self) : std::string_view

    /**
     * Append a character into the current string you are building
     */
    func append(&self, c : char);

}