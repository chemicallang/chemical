public namespace fs {

using std::Result;

public func copy_directory(src : *char, dst : *char, preserve_metadata : bool) : Result<UnitTy, FsError> {
        unsafe {
        // recursive copy: create dest dir then iterate src and copy files/dirs
        var st_res = metadata(src);
        if(st_res is Result.Err) {
            var Err(e) = st_res else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(st) = st_res else unsafe { unreachable };
        if(!st.is_dir) { return Result.Err(FsError.NotADirectory()); }
        // create dst
        var cd = create_dir_all(dst);
        if(cd is Result.Err) {
            var Err(e) = cd else unsafe { unreachable }
            return Result.Err(e)
        }
        // iterate src
        var res = read_dir(src, |src, dst, preserve_metadata|(name : *char, name_len : size_t, is_dir : bool) => {
            // skip . and ..
            if(name_len == 1 && name[0] == '.') { return true; }
            if(name_len == 2 && name[0] == '.' && name[1] == '.') { return true; }
            var srcchild : [PATH_MAX_BUF]char;
            var dstchild : [PATH_MAX_BUF]char;
            var p : size_t = 0; while(p < PATH_MAX_BUF as size_t && src[p] != 0) { srcchild[p] = src[p]; p++ }
            if(p == PATH_MAX_BUF as size_t) { return false; }
            if(p > 0 && srcchild[p-1] != '/') {
                if(p + 1 >= PATH_MAX_BUF as size_t) { return false; }
                srcchild[p++] = '/';
            }
            if(name_len >= PATH_MAX_BUF as size_t - p) { return false; }
            var q : size_t = 0; while(q <= name_len) { srcchild[p + q] = name[q]; q++ }
            var r : size_t = 0; while(r < PATH_MAX_BUF as size_t && dst[r] != 0) { dstchild[r] = dst[r]; r++ }
            if(r == PATH_MAX_BUF as size_t) { return false; }
            if(r > 0 && dstchild[r-1] != '/') {
                if(r + 1 >= PATH_MAX_BUF as size_t) { return false; }
                dstchild[r++] = '/';
            }
            if(name_len >= PATH_MAX_BUF as size_t - r) { return false; }
            q = 0; while(q <= name_len) { dstchild[r + q] = name[q]; q++ }
            if(is_dir) {
                var c = copy_directory(&raw mut srcchild[0], &raw mut dstchild[0], preserve_metadata);
                if(c is Result.Err) { var Err(e) = c else unsafe { unreachable }; return false; }
            } else {
                var c = copy_file(&raw mut srcchild[0], &raw mut dstchild[0]);
                if(c is Result.Err) { var Err(e) = c else unsafe { unreachable }; return false; }
                if(preserve_metadata) {
                    var meta = metadata(&raw mut srcchild[0]);
                    if(meta is Result.Err) {
                        var Err(e) = meta else unsafe { unreachable }
                        // TODO: cannot return the result
                        // return Result.Err<UnitTy, FsError>(e)
                        return false;
                    }
                    var Ok(m) = meta else unsafe { unreachable };
                    var setp = set_permissions(&raw mut dstchild[0], m.perms);
                    if(setp is Result.Err) { var Err(e) = setp else unsafe { unreachable }; /* non-fatal? */ }
                    var stimes = set_times(&raw mut dstchild[0], m.accessed, m.modified);
                    if(stimes is Result.Err) { var Err(e) = stimes else unsafe { unreachable }; }
                }
            }
            return true;
        });
        if(res is Result.Err) {
            var Err(e) = res else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(_) = res else unsafe { unreachable };
        return Result.Ok(UnitTy{});

        }
    }

}