// Default frame allocation hooks used by compiler-generated async ramps
// (design Section 12.3). They are declared `@extern` in `core::async`; these are
// the concrete definitions. An executor may replace them later (task arena)
// without changing generated code, because ramps call through these symbols.
public namespace async {

    @no_mangle
    public func chemical_async_frame_alloc(size : u64, align : u64) : *mut void {
        return malloc(size)
    }

    @no_mangle
    public func chemical_async_frame_free(ptr : *mut void, size : u64, align : u64) {
        free(ptr)
    }

}
