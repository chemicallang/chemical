public namespace fs {

using std::Result;

// exists / is_file / is_dir convenience
public func exists(path : *char) : bool {
    var r = metadata(path);
    return !(r is Result.Err);
}

public func is_file(path : *char) : Result<bool, FsError> {
    var r = metadata(path);
    if(r is Result.Err) { var Err(e) = r else unreachable; return Result.Err(e); }
    var Ok(m) = r else unreachable;
    return Result.Ok(m.is_file);
}

public func is_dir(path : *char) : Result<bool, FsError> {
    var r = metadata(path);
    if(r is Result.Err) { var Err(e) = r else unreachable; return Result.Err(e); }
    var Ok(m) = r else unreachable;
    return Result.Ok(m.is_dir);
}

}