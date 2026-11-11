public namespace net {

    // WinSock2 minimal declarations. You might need to link -lws2_32 when building.
    @dllimport @stdcall @extern protected func WSAStartup(wVersionRequested: ushort, lpWSAData: *mut char): int;
    @dllimport @stdcall @extern protected func WSACleanup(): int;
    @dllimport @stdcall @extern protected func WSAGetLastError(): int;
    // @dllimport @stdcall @extern protected func FormatMessageA(dwFlags: uint, lpSource:*void, dwMessageId:uint, dwLanguageId:uint, lpBuffer:*mut char, nSize:uint, Arguments:*void): uint;
    @dllimport @stdcall @extern protected func socket(af:int, typ:int, protocol:int): uintptr_t;
    @dllimport @stdcall @extern protected func bind(s: uintptr_t, name:*char, namelen:int): int;
    @dllimport @stdcall @extern protected func listen(s: uintptr_t, backlog:int): int;
    @dllimport @stdcall @extern protected func connect(s: uintptr_t, name:*char, namelen:int): int;
    @dllimport @stdcall @extern protected func accept(s: uintptr_t, addr:*mut char, addrlen:*mut int): uintptr_t;
    @dllimport @stdcall @extern protected func recv(s: uintptr_t, buf:*mut char, len:int, flags:int): int;
    @dllimport @stdcall @extern protected func send(s: uintptr_t, buf:*char, len:int, flags:int): int;
    @dllimport @stdcall @extern protected func closesocket(s: uintptr_t): int;
    @dllimport @stdcall @extern protected func setsockopt(s: uintptr_t, level:int, optname:int, optval:*char, optlen:int): int;
    @dllimport @stdcall @extern protected func getaddrinfo(node:*char, service:*char, hints:*mut char, res:*mut *mut char): int;
    @dllimport @stdcall @extern protected func freeaddrinfo(res:*mut char): void;
    @dllimport @stdcall @extern protected func getsockname(s: uintptr_t, name: *mut char, namelen: *mut int): int;
    @dllimport @stdcall @extern protected func getpeername(s: uintptr_t, name: *mut char, namelen: *mut int): int;
    @dllimport @stdcall @extern protected func shutdown(s: uintptr_t, how: int): int;

    @dllimport @stdcall @extern protected func TransmitFile(
        husize: uintptr_t,
        hFile: uintptr_t,
        nNumberOfBytesToWrite: u32,
        nNumberOfBytesPerSend: u32,
        lpOverlapped:*void,
        lpTransmitBuffers:*void,
        dwFlags: u32
    ) : int;

    @dllimport @stdcall @extern protected func ioctlsocket(s: uintptr_t, cmd: long, argp: *mut u32): int;
    comptime const FIONBIO : long = 0x8004667E

    // constants used
    // comptime const GENERIC_READ = 0x80000000u;
    // comptime const FILE_SHARE_READ = 0x00000001u;
    // comptime const OPEN_EXISTING = 3u;

    func startup() {
        unsafe {
        // WSADATA is ~400 bytes, [32]char is too small and causes stack corruption
        var wsaData : [512]char;
        WSAStartup(0x202 as ushort, &raw mut wsaData[0]);

        }}
    func cleanup() {
        unsafe { WSACleanup()
        }}

    // wrappers
    func sock_socket(af:int, typ:int, proto:int) : usize {
        unsafe { return socket(af, typ, proto) as usize
        }}
    func sock_bind(s:usize, addr:*char, addrlen:int) : int {
        unsafe { return bind(s as uintptr_t, addr, addrlen)
        }}
    func sock_listen(s:usize, backlog:int) : int {
        unsafe { return listen(s as uintptr_t, backlog)
        }}
    func sock_connect(s:usize, addr:*char, addrlen:int) : int {
        unsafe { return connect(s as uintptr_t, addr, addrlen)
        }}
    func sock_accept(s:usize, addr:*mut char, addrlen:*mut int) : usize {
        unsafe { return accept(s as uintptr_t, addr, addrlen) as usize
        }}
    func sock_recv(s:usize, buf:*mut char, len:int) : int {
        unsafe { return recv(s as uintptr_t, buf, len, 0)
        }}
    func sock_send(s:usize, buf:*char, len:int) : int {
        unsafe { return send(s as uintptr_t, buf, len, 0)
        }}
    func sock_close(s:usize) {
        unsafe { closesocket(s as uintptr_t)
        }}
    func sock_setsockopt(s:usize, level:int, optname:int, optval:*char, optlen:int) : int {
        unsafe { return setsockopt(s as uintptr_t, level, optname, optval, optlen)
        }}
    func sock_getaddrinfo(node:*char, service:*char, hints:*mut char, res:*mut *mut char) : int {
        unsafe { return getaddrinfo(node, service, hints, res)
        }}
    func sock_freeaddrinfo(res:*mut char) {
        unsafe { freeaddrinfo(res)
        }}

    // Windows poll stub (uses WSAPoll, but we accept with a simple timeout approach)
    public struct pollfd {
        var fd: uintptr_t
        var events: short
        var revents: short
    }
    func sock_poll(fds:*mut pollfd, nfds:ulong, timeout:int): int {
        unsafe {
        // Windows: just return 1 (data available) immediately since accept() blocks anyway
        // Proper implementation would use WSAPoll, but this is Windows-stub
        return 1

        }}

    func sock_set_nonblocking(s: usize) {
        unsafe {
        var argp: u32 = 1u;
        ioctlsocket(s as uintptr_t, FIONBIO, &raw mut argp);

        }}

    func sock_set_blocking(s: usize) {
        unsafe {
        var argp: u32 = 0u;
        ioctlsocket(s as uintptr_t, FIONBIO, &raw mut argp);

        }}


    func sock_local_addr(s : usize, out : *mut raw::sockaddr_in) : int {
        var len : int = sizeof(raw::sockaddr_in) as int
        return unsafe(getsockname(s as uintptr_t, out as *mut char, &raw mut len))
    }

    func sock_peer_addr(s : usize, out : *mut raw::sockaddr_in) : int {
        var len : int = sizeof(raw::sockaddr_in) as int
        return unsafe(getpeername(s as uintptr_t, out as *mut char, &raw mut len))
    }

    func sock_shutdown(s : usize, how : int) : int {
        return unsafe(shutdown(s as uintptr_t, how))
    }

    // `net` re-exports the POSIX names its cross-platform users call
    // (`http`'s `send_file` seeks the opened file through `net::lseek`). The
    // CRT exports the underscore-prefixed spelling on Windows.
    @extern protected func _lseek(fd : int, offset : longlong, whence : int) : longlong

    public func lseek(fd : int, offset : longlong, whence : int) : longlong {
        return unsafe(_lseek(fd, offset, whence))
    }

}
