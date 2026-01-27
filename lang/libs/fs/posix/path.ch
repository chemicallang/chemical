public namespace fs {

using std::Result;

func canonicalize(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    // POSIX: realpath
    var resptr = fs::realpath(path, out);
    if(resptr == 0) {
        // errno must be mapped by user; assume errno accessor exists
        return Result.Err(FsError.Io(-1, "realpath failed\0"));
    }
    var plen : size_t = 0; while(out[plen] != 0) { plen++ }
    return Result.Ok(plen);
}

}