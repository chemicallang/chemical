public type __dev_t = ulong
public type __ino_t = ulong
public type __nlink_t = ulong
public type __mode_t = uint
public type __uid_t = uint
public type __gid_t = uint
public type __off_t = long
public type __blksize_t = long
public type __blkcnt_t = long
public type __syscall_slong_t = long
public type __time_t = long

// @extern
// public struct timespec {
//     var tv_sec : __time_t;		/* Seconds.  */
//     var tv_nsec : __syscall_slong_t;	/* Nanoseconds.  */
// };

@extern
public struct stat {
    var st_dev : __dev_t;		/* Device.  */
    var st_ino : __ino_t;		/* File serial number.	*
    var st_nlink : __nlink_t;		/* Link count.  */
    var st_mode : __mode_t;		/* File mode.  */
    var st_uid : __uid_t;		/* User ID of the file's owner.	*/
    var st_gid : __gid_t;		/* Group ID of the file's group.*/
    var __pad0 : int;
    var st_rdev : __dev_t;		/* Device number, if device.  */
    var st_size : __off_t;			/* Size of file, in bytes.  */
    var st_blksize : __blksize_t;	/* Optimal block size for I/O.  */
    var st_blocks : __blkcnt_t;		/* Number 512-byte blocks allocated. */
    /* Nanosecond resolution timestamps are stored in a format
       equivalent to 'struct timespec'.  This is the type used
       whenever possible but the Unix namespace rules do not allow the
       identifier 'timespec' to appear in the <sys/stat.h> header.
       Therefore we have to handle the use of this header in strictly
       standard-compliant sources special.  */
    var st_atime : timespec;		/* Time of last access.  */
    var st_mtime : timespec;		/* Time of last modification.  */
    var st_ctime : timespec;		/* Time of last status change.  */
    var __glibc_reserved : __syscall_slong_t[3];
};

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