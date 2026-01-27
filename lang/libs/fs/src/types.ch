public namespace fs {

using std::Result;

public variant FsError {
    Io(code : int, message : *char)
    NotFound()
    AlreadyExists()
    PermissionDenied()
    InvalidInput()
    NotADirectory()
    IsADirectory()
    WouldBlock()
    PathTooLong()
    Unsupported()   // used only for extremely exotic requests
    Other(msg : *char)

    func message(&self) : std::string {
        switch(self) {
            Io(code, message) => {
                var msg = std::string("io error with code ")
                msg.append_integer(code)
                msg.append_view(std::string_view(" and message '"))
                msg.append_char_ptr(message)
                msg.append('\'')
                return msg;
            }
            NotFound() => return std::string("FsError.NotFound")
            AlreadyExists() => return std::string("FsError.AlreadyExists")
            PermissionDenied() => return std::string("FsError.PermissionDenied")
            InvalidInput() => return std::string("FsError.InvalidInput")
            NotADirectory() => return std::string("FsError.NotADirectory")
            IsADirectory() => return std::string("FsError.IsADirectory")
            WouldBlock() => return std::string("FsError.WouldBlock")
            PathTooLong() => return std::string("FsError.PathTooLong")
            Unsupported() => return std::string("FsError.Unsupported")   // used only for extremely exotic requests
            Other(msg) => return std::string(msg);
        }
    }

}

public struct Metadata {
    var is_file : bool;
    var is_dir : bool;
    var is_symlink : bool;
    var len : size_t;
    var modified : i64;
    var accessed : i64;
    var created : i64;
    var perms : u32; // POSIX rwx bits or Windows attributes mapping
}

public struct OpenOptions {
    var read : bool;
    var write : bool;
    var append : bool;
    var create : bool;
    var create_new : bool; // exclusive create
    var truncate : bool;
    var binary : bool;
}

public struct File {
    if(def.windows) {
        struct { var handle : void*; } win;
    } else {
        struct { var fd : int; } unix;
    }
    var valid : bool;
}

}