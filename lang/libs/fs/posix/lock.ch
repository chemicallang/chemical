public namespace fs {

using std::Result;

func lock_file_shared(path : *char) : Result<File, FsError> {
    var opts : OpenOptions; opts.read = true; opts.write = false;
    var fo = file_open(path, opts);
    if(fo is Result.Err) {
        var Err(e) = fo else unreachable;
        return Result.Err(e);
    }
    var Ok(f) = fo else unreachable;
    var r = flock(f.unix.fd, LOCK_SH);
    if(r != 0) { file_close(&mut f); return Result.Err(posix_errno_to_fs(get_errno())); }
    return Result.Ok(f);
}

func lock_file_exclusive(path : *char) : Result<File, FsError> {
    var opts : OpenOptions; opts.read = true; opts.write = true;
    var fo = file_open(path, opts);
    if(fo is Result.Err) {
        var Err(e) = fo else unreachable;
        return Result.Err(e);
    }
    var Ok(f) = fo else unreachable;
    var r = flock(f.unix.fd, LOCK_EX);
    if(r != 0) { file_close(&mut f); return Result.Err(posix_errno_to_fs(get_errno())); }
    return Result.Ok(f);
}

}