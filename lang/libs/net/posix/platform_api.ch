public namespace net {

    // POSIX declarations
    @extern protected func socket(af:int, typ:int, protocol:int): int;
    @extern protected func bind(s: int, name:*char, namelen:int): int;
    @extern protected func listen(s: int, backlog:int): int;
    @extern protected func accept(s: int, addr:*mut char, addrlen:*mut int): int;
    @extern protected func recv(s: int, buf:*mut char, len:int, flags:int): int;
    @extern protected func send(s: int, buf:*char, len:int, flags:int): int;
    @extern protected func close(fd: int): int;
    @extern protected func setsockopt(s: int, level:int, optname:int, optval:*char, optlen:int): int;
    @extern protected func getaddrinfo(node:*char, service:*char, hints:*mut char, res:*mut *mut char): int;
    @extern protected func freeaddrinfo(res:*mut char): void;


    // file I/O + sendfile
    @extern protected func open(path:*char, flags:int, mode:int): int;
    @extern protected func lseek(fd:int, offset: longlong, whence:int): longlong;
    // Linux sendfile: ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);
    @extern protected func sendfile(out_fd:int, in_fd:int, offset:*longlong, count:size_t) : longlong;

    // constants for open/lseek
    comptime const O_RDONLY = 0;
    comptime const SEEK_SET = 0;
    comptime const SEEK_END = 2;

    func startup() { }
    func cleanup() { }

    func sock_socket(af:int, typ:int, proto:int) : Socket { return socket(af, typ, proto) as Socket }
    func sock_bind(s:Socket, addr:*char, addrlen:int) : int { return bind(s as int, addr, addrlen) }
    func sock_listen(s:Socket, backlog:int) : int { return listen(s as int, backlog) }
    func sock_accept(s:Socket, addr:*mut char, addrlen:*mut int) : Socket { return accept(s as int, addr, addrlen) as Socket }
    func sock_recv(s:Socket, buf:*mut char, len:int) : int { return recv(s as int, buf, len, 0) }
    func sock_send(s:Socket, buf:*char, len:int) : int { return send(s as int, buf, len, 0) }
    func sock_close(s:Socket) { close(s as int) }
    func sock_setsockopt(s:Socket, level:int, optname:int, optval:*char, optlen:int) : int { return setsockopt(s as int, level, optname, optval, optlen) }
    func sock_getaddrinfo(node:*char, service:*char, hints:*mut char, res:*mut *mut char) : int { return getaddrinfo(node, service, hints, res) }
    func sock_freeaddrinfo(res:*mut char) { freeaddrinfo(res) }

}