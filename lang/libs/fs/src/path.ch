public namespace fs {

using std::Result;
using path::Path;

public type path_ptr = if(def.windows) *u16 else *char

public type mut_path_ptr = if(def.windows) *mut u16 else *mut char

// Resolve a safe `path::Path` to the native NUL-terminated form cached by
// `PathBuf`. Rejects interior NUL and views that cannot provide a safe native
// pointer (for example a Windows `Path::new` without conversion).
public func native_path(path : &Path) : Result<path_ptr, FsError> {
    if(path.has_nul()) {
        return Result.Err(FsError.InvalidInput())
    }
    var p = path.native_ptr()
    if(p == null) {
        return Result.Err(FsError.InvalidInput())
    }
    return Result.Ok(p)
}



} // namespace fs
