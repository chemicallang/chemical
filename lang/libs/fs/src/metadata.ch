public namespace fs {

using std::Result;

// is_file / is_dir convenience
func is_file(path : *char) : Result<bool, FsError> {
    var r = metadata(path);
    if(r is Result.Err) { var Err(e) = r else unreachable; return Result.Err(e); }
    var Ok(m) = r else unreachable;
    return Result.Ok(m.is_file);
}

func is_dir(path : *char) : Result<bool, FsError> {
    var r = metadata(path);
    if(r is Result.Err) { var Err(e) = r else unreachable; return Result.Err(e); }
    var Ok(m) = r else unreachable;
    return Result.Ok(m.is_dir);
}

}