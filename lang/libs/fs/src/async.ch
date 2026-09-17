// Async wrappers for the filesystem library (design §1.9 F11 / §7 Tier 4).
//
// These are additive: the synchronous `fs` API is unchanged. Each wrapper is an
// `async func` with a concrete return type, and its body offloads the blocking
// syscall to the runtime's thread pool via `async::spawn_blocking`, so the
// executor keeps running while the disk I/O is in flight.
//
// Lifetime note: the arguments are captured into the task, so their pointed-to
// data must stay alive until the returned future completes (the usual async
// contract).
public namespace fs {

using std::Result;

public async func read_entire_file_async(path : *char) : Result<std::vector<u8>, FsError> {
    var result = await async::spawn_blocking<Result<std::vector<u8>, FsError>>(
        |path|() => read_entire_file(path)
    )
    return result
}

public async func write_text_file_async(path : *char, data : *u8, data_len : size_t) : Result<UnitTy, FsError> {
    var result = await async::spawn_blocking<Result<UnitTy, FsError>>(
        |path, data, data_len|() => write_text_file(path, data, data_len)
    )
    return result
}

public async func atomic_write_async(path : *char, data : *u8, data_len : size_t) : Result<UnitTy, FsError> {
    var result = await async::spawn_blocking<Result<UnitTy, FsError>>(
        |path, data, data_len|() => atomic_write(path, data, data_len)
    )
    return result
}

}
