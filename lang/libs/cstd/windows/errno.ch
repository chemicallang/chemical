@extern
public func _errno() : *mut int;

public func get_errno() : int {
    return *_errno();
}

public func set_errno(value : int) {
    *_errno() = value;
}

public comptime const EPERM =           1
public comptime const ENOENT =          2
public comptime const ESRCH =           3
public comptime const EINTR =           4
public comptime const EIO =             5
public comptime const ENXIO =           6
public comptime const E2BIG =           7
public comptime const ENOEXEC =         8
public comptime const EBADF =           9
public comptime const ECHILD =          10
public comptime const EAGAIN =          11
public comptime const ENOMEM =          12
public comptime const EACCES =          13
public comptime const EFAULT =          14
public comptime const EBUSY =           16
public comptime const EEXIST =          17
public comptime const EXDEV =           18
public comptime const ENODEV =          19
public comptime const ENOTDIR =         20
public comptime const EISDIR =          21
public comptime const ENFILE =          23
public comptime const EMFILE =          24
public comptime const ENOTTY =          25
public comptime const EFBIG =           27
public comptime const ENOSPC =          28
public comptime const ESPIPE =          29
public comptime const EROFS =           30
public comptime const EMLINK =          31
public comptime const EPIPE =           32
public comptime const EDOM =            33
public comptime const EDEADLK =         36
public comptime const ENAMETOOLONG =    38
public comptime const ENOLCK =          39
public comptime const ENOSYS =          40
public comptime const ENOTEMPTY =       41

public comptime const EINVAL =          22
public comptime const ERANGE =          34
public comptime const EILSEQ =          42
public comptime const STRUNCATE =       80

public comptime const EADDRINUSE =      100
public comptime const EADDRNOTAVAIL =   101
public comptime const EAFNOSUPPORT =    102
public comptime const EALREADY =        103
public comptime const EBADMSG =         104
public comptime const ECANCELED =       105
public comptime const ECONNABORTED =    106
public comptime const ECONNREFUSED =    107
public comptime const ECONNRESET =      108
public comptime const EDESTADDRREQ =    109
public comptime const EHOSTUNREACH =    110
public comptime const EIDRM =           111
public comptime const EINPROGRESS =     112
public comptime const EISCONN =         113
public comptime const ELOOP =           114
public comptime const EMSGSIZE =        115
public comptime const ENETDOWN =        116
public comptime const ENETRESET =       117
public comptime const ENETUNREACH =     118
public comptime const ENOBUFS =         119
public comptime const ENODATA =         120
public comptime const ENOLINK =         121
public comptime const ENOMSG =          122
public comptime const ENOPROTOOPT =     123
public comptime const ENOSR =           124
public comptime const ENOSTR =          125
public comptime const ENOTCONN =        126
public comptime const ENOTRECOVERABLE = 127
public comptime const ENOTSOCK =        128
public comptime const ENOTSUP =         129
public comptime const EOPNOTSUPP =      130
public comptime const EOTHER =          131
public comptime const EOVERFLOW =       132
public comptime const EOWNERDEAD =      133
public comptime const EPROTO =          134
public comptime const EPROTONOSUPPORT = 135
public comptime const EPROTOTYPE =      136
public comptime const ETIME =           137
public comptime const ETIMEDOUT =       138
public comptime const ETXTBSY =         139
public comptime const EWOULDBLOCK =     140