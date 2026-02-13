public type __fsblkcnt_t = ulong
public type __fsfilcnt_t = ulong

@extern
public struct statvfs {
    var f_bsize : ulong;
    var f_frsize : ulong;
    var f_blocks : __fsblkcnt_t;
    var f_bfree : __fsblkcnt_t;
    var f_bavail : __fsblkcnt_t;
    var f_files : __fsfilcnt_t;
    var f_ffree : __fsfilcnt_t;
    var f_favail : __fsfilcnt_t;
    var f_fsid : ulong;
    var f_flag : ulong;
    var f_namemax : ulong;
    var f_type : uint;
    var __f_spare : int[5];
};

public namespace fs {

    @extern public func realpath(path : *char, resolved : *mut char) : *char;
    @extern public func chmod(path : *char, mode : u32) : int;

    public type blksize_t = i64;
    public type fsblkcnt_t = u64;
    public type fsfilcnt_t = u64;

    const AT_FDCWD = -100;               // use current working directory
    const AT_SYMLINK_NOFOLLOW = 0x100;   // do not follow symlinks (Linux typical)
    @extern public func utimensat(dirfd : int, pathname : *char, times : *timespec, flags : int) : int;
    @extern public func fsync(fd : int) : int;

    @extern public func link(oldpath : *char, newpath : *char) : int;
    @extern public func symlink(target : *char, linkpath : *char) : int;

    // Returns file descriptor on success, -1 on error (and modifies template in-place).
    @extern public func mkstemp(template : *mut char) : int;

    public struct Statvfs {
        var f_bsize   : ulong;   // file system block size
        var f_frsize  : ulong;   // fragment size
        var f_blocks  : fsblkcnt_t;      // size of fs in f_frsize units
        var f_bfree   : fsblkcnt_t;      // # free blocks
        var f_bavail  : fsblkcnt_t;      // # free blocks for unprivileged users
        var f_files   : fsfilcnt_t;      // # inodes
        var f_ffree   : fsfilcnt_t;      // # free inodes
        var f_favail  : fsfilcnt_t;      // # free inodes for unprivileged users
        var f_fsid    : ulong;   // file system ID
        var f_flag    : ulong;   // mount flags
        var f_namemax : ulong;   // maximum filename length
    }
    @extern public func statvfs(path : *char, out : *mut Statvfs) : int;

    @extern public func flock(fd : int, operation : int) : int;

    const LOCK_SH = 1;   // shared lock
    const LOCK_EX = 2;   // exclusive lock
    const LOCK_NB = 4;   // non-blocking
    const LOCK_UN = 8;   // unlock

    public type ssize_t = isize;

    // POSIX readlink declaration:
    // ssize_t readlink(const char *path, char *buf, size_t bufsiz);
    @extern public func readlink(path : *char, buf : *mut char, bufsiz : size_t) : ssize_t;


    // POSIX externs
    // type DIR = *void;
    struct dirent { var d_name : [PATH_MAX_BUF]char; }
    struct stat { var st_mode : u32; }

    // @extern public func opendir(path : *char) : DIR;
    // @extern public func readdir(dir : DIR) : *mut dirent;
    // @extern public func closedir(dir : DIR) : int;
    // @extern public func lstat(path : *char, out : *mut Stat) : int;
    @extern public func unlink(path : *char) : int;
    @extern public func rmdir(path : *char) : int;


    @extern public func openat(dirfd : int, path : *char, flags : int, mode : u32) : int;
    @extern public func dup(fd : int) : int;
    @extern public func fdopendir(fd : int) : *mut DIR;

    @extern public func fstatat(dirfd : int, pathname : *char, out : *mut Stat, flags : int) : int;

    @extern public func unlinkat(dirfd : int, pathname : *char, flags : int) : int;

    // common flags (Linux values)
    const O_RDONLY = 0;
    const O_DIRECTORY = 0x20000;           // typical Linux value; adjust if your platform differs
    const AT_REMOVEDIR = 0x200;            // unlinkat flag to remove a directory (Linux)

    const FILE_ATTRIBUTE_DIRECTORY = 0x10;

    const S_IFMT  = 0xF000;
    const S_IFDIR = 0x4000;

}