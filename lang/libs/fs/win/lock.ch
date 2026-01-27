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
    // use LockFileEx with LOCKFILE_FAIL_IMMEDIATELY & LOCKFILE_SHARED_LOCK
    var ok = LockFileEx(f.win.handle, LOCKFILE_FAIL_IMMEDIATELY as DWORD, 0, 0xFFFFFFFF, 0xFFFFFFFF, null);
    if(ok == 0) { var e = GetLastError(); file_close(&mut f); return Result.Err(winerr_to_fs(e as int)); }
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
    var ok = LockFileEx(f.win.handle, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, null);
    if(ok == 0) { var e = GetLastError(); file_close(&mut f); return Result.Err(winerr_to_fs(e as int)); }
    return Result.Ok(f);
}

}