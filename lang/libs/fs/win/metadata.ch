public namespace fs {

using std::Result;

func metadata_native(path : path_ptr) : Result<Metadata, FsError> {
    var fi : WIN32_FILE_ATTRIBUTE_DATA;
    var ok = GetFileAttributesExW(path as LPCWSTR, GET_FILEEX_INFO_LEVELS.GetFileExInfoStandard, &mut fi);
    if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    var m : Metadata;
    var attrs = fi.dwFileAttributes;
    m.is_dir = ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0);
    m.is_file = !m.is_dir;
    m.is_symlink = ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0);
    // convert LARGE_INTEGER FILETIME to epoch seconds (simplified)
    m.len = (fi.nFileSizeHigh as size_t) << 32 | (fi.nFileSizeLow as size_t);
    m.modified = filetime_to_unix(fi.ftLastWriteTime);
    m.accessed = filetime_to_unix(fi.ftLastAccessTime);
    m.created = filetime_to_unix(fi.ftCreationTime);
    m.perms = attrs as u32;
    return Result.Ok(m);
}

public func metadata(path : *char) : Result<Metadata, FsError> {
    var wbuf : [WIN_MAX_PATH]u16;
    var conv = utf8_to_utf16(path, &mut wbuf[0], WIN_MAX_PATH as size_t);
    if(conv is Result.Err) {
        var Err(e) = conv else unreachable
        return Result.Err(e)
    }
    return metadata_native(&mut wbuf[0])
}

}