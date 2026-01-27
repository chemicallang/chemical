public namespace fs {

func posix_errno_to_fs(e : int) : FsError {
    if(e == 2) { return FsError.NotFound(); }
    if(e == 13) { return FsError.PermissionDenied(); }
    if(e == 17) { return FsError.AlreadyExists(); }
    if(e == 21) { return FsError.IsADirectory(); }
    if(e == 20) { return FsError.NotADirectory(); }
    if(e == 11) { return FsError.WouldBlock(); }
    return FsError.Io(e, "posix error\0");
}

func winerr_to_fs(code : int) : FsError {
    if(code == 2) { return FsError.NotFound(); }
    if(code == 5) { return FsError.PermissionDenied(); }
    if(code == 80) { return FsError.AlreadyExists(); }
    return FsError.Io(code, "win32 error\0");
}

}