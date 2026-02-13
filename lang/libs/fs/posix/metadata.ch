public namespace fs {

using std::Result;

func metadata_native(path : path_ptr) : Result<Metadata, FsError> {
    var st : Stat;
    var r = lstat(path, &mut st);
    if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
    var m : Metadata;
    var mode : int = st.st_mode as int;
    m.is_dir = ((mode & 0xF000) == 0x4000);
    m.is_file = ((mode & 0xF000) == 0x8000);
    m.is_symlink = ((mode & 0xF000) == 0xA000);
    m.len = st.st_size as size_t;
    m.modified = st.st_mtime.tv_sec as i64;
    m.accessed = st.st_atime.tv_sec as i64;
    m.created = st.st_ctime.tv_sec as i64;
    m.perms = (st.st_mode & 0x1FF) as u32;
    return Result.Ok(m);
}

func metadata(path : *char) : Result<Metadata, FsError> {
    return metadata_native(path)
}

}