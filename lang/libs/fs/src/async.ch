// Async wrappers for the filesystem library (design §1.9 F11 / §7 Tier 4).
//
// These are additive: the synchronous `fs` API is unchanged. Each wrapper is an
// `async func` with a concrete return type, so the compiler emits its future
// vtable (no hand-written generic `FutureTable<T>` needed).
//
// v1 note: the body performs the blocking syscall directly, so it does not yet
// keep the executor responsive. Once `spawn_blocking` is unblocked (see
// lang/docs/async-library-integration.md §12), these bodies move to it without
// changing their signatures.
public namespace fs {

using std::Result;

public async func read_entire_file_async(path : *char) : Result<std::vector<u8>, FsError> {
    return read_entire_file(path)
}

public async func write_text_file_async(path : *char, data : *u8, data_len : size_t) : Result<UnitTy, FsError> {
    return write_text_file(path, data, data_len)
}

public async func atomic_write_async(path : *char, data : *u8, data_len : size_t) : Result<UnitTy, FsError> {
    return atomic_write(path, data, data_len)
}

}
