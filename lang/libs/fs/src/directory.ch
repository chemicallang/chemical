public namespace fs {

using std::Result;

public func copy_directory(src : *char, dst : *char, preserve_metadata : bool) : Result<UnitTy, FsError> {
    // recursive copy: create dest dir then iterate src and copy files/dirs
    var st_res = metadata(src);
    if(st_res is Result.Err) {
        var Err(e) = st_res else unreachable
        return Result.Err(e)
    }
    var Ok(st) = st_res else unreachable;
    if(!st.is_dir) { return Result.Err(FsError.NotADirectory()); }
    // create dst
    var cd = create_dir_all(dst);
    if(cd is Result.Err) {
        var Err(e) = cd else unreachable
        return Result.Err(e)
    }
    // iterate src
    var res = read_dir(src, |src, dst, preserve_metadata|(name : *char, name_len : size_t, is_dir : bool) => {
        // skip . and ..
        if(name_len == 1 && name[0] == '.') { return true; }
        if(name_len == 2 && name[0] == '.' && name[1] == '.') { return true; }
        var srcchild : [PATH_MAX_BUF]char;
        var dstchild : [PATH_MAX_BUF]char;
        var p : size_t = 0; while(src[p] != 0) { srcchild[p] = src[p]; p++ }
        if(p > 0 && srcchild[p-1] != '/') { srcchild[p++] = '/'; }
        var q : size_t = 0; while(q <= name_len) { srcchild[p + q] = name[q]; q++ }
        var r : size_t = 0; while(dst[r] != 0) { dstchild[r] = dst[r]; r++ }
        if(r > 0 && dstchild[r-1] != '/') { dstchild[r++] = '/'; }
        q = 0; while(q <= name_len) { dstchild[r + q] = name[q]; q++ }
        if(is_dir) {
            var c = copy_directory(&mut srcchild[0], &mut dstchild[0], preserve_metadata);
            if(c is Result.Err) { var Err(e) = c else unreachable; return false; }
        } else {
            var c = copy_file(&mut srcchild[0], &mut dstchild[0]);
            if(c is Result.Err) { var Err(e) = c else unreachable; return false; }
            if(preserve_metadata) {
                var meta = metadata(&mut srcchild[0]);
                if(meta is Result.Err) {
                    var Err(e) = meta else unreachable
                    // TODO: cannot return the result
                    // return Result.Err<UnitTy, FsError>(e)
                    return false;
                }
                var Ok(m) = meta else unreachable;
                var setp = set_permissions(&mut dstchild[0], m.perms);
                if(setp is Result.Err) { var Err(e) = setp else unreachable; /* non-fatal? */ }
                var stimes = set_times(&mut dstchild[0], m.accessed, m.modified);
                if(stimes is Result.Err) { var Err(e) = stimes else unreachable; }
            }
        }
        return true;
    });
    if(res is Result.Err) {
        var Err(e) = res else unreachable
        return Result.Err(e)
    }
    var Ok(_) = res else unreachable;
    return Result.Ok(UnitTy{});
}

}