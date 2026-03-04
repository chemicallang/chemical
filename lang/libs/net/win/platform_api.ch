public namespace net {

    // WinSock2 minimal declarations. You might need to link -lws2_32 when building.
    @dllimport @stdcall @extern protected func WSAStartup(wVersionRequested: ushort, lpWSAData: *mut char): int;
    @dllimport @stdcall @extern protected func WSACleanup(): int;
    @dllimport @stdcall @extern protected func WSAGetLastError(): int;
    // @dllimport @stdcall @extern protected func FormatMessageA(dwFlags: uint, lpSource:*void, dwMessageId:uint, dwLanguageId:uint, lpBuffer:*mut char, nSize:uint, Arguments:*void): uint;
    @dllimport @stdcall @extern protected func socket(af:int, typ:int, protocol:int): uintptr_t;
    @dllimport @stdcall @extern protected func bind(s: uintptr_t, name:*char, namelen:int): int;
    @dllimport @stdcall @extern protected func listen(s: uintptr_t, backlog:int): int;
    @dllimport @stdcall @extern protected func accept(s: uintptr_t, addr:*mut char, addrlen:*mut int): uintptr_t;
    @dllimport @stdcall @extern protected func recv(s: uintptr_t, buf:*mut char, len:int, flags:int): int;
    @dllimport @stdcall @extern protected func send(s: uintptr_t, buf:*char, len:int, flags:int): int;
    @dllimport @stdcall @extern protected func closesocket(s: uintptr_t): int;
    @dllimport @stdcall @extern protected func setsockopt(s: uintptr_t, level:int, optname:int, optval:*char, optlen:int): int;
    @dllimport @stdcall @extern protected func getaddrinfo(node:*char, service:*char, hints:*mut char, res:*mut *mut char): int;
    @dllimport @stdcall @extern protected func freeaddrinfo(res:*mut char): void;

    @dllimport @stdcall @extern protected func TransmitFile(
        hSocket: uintptr_t,
        hFile: uintptr_t,
        nNumberOfBytesToWrite: u32,
        nNumberOfBytesPerSend: u32,
        lpOverlapped:*void,
        lpTransmitBuffers:*void,
        dwFlags: u32
    ) : int;

    // constants used
    // comptime const GENERIC_READ = 0x80000000u;
    // comptime const FILE_SHARE_READ = 0x00000001u;
    // comptime const OPEN_EXISTING = 3u;

    func startup() { var dummy : [32]char; WSAStartup(0x202 as ushort, &mut dummy[0]); }
    func cleanup() { WSACleanup() }

    // wrappers
    func sock_socket(af:int, typ:int, proto:int) : Socket { return socket(af, typ, proto) as Socket }
    func sock_bind(s:Socket, addr:*char, addrlen:int) : int { return bind(s as uintptr_t, addr, addrlen) }
    func sock_listen(s:Socket, backlog:int) : int { return listen(s as uintptr_t, backlog) }
    func sock_accept(s:Socket, addr:*mut char, addrlen:*mut int) : Socket { return accept(s as uintptr_t, addr, addrlen) as Socket }
    func sock_recv(s:Socket, buf:*mut char, len:int) : int { return recv(s as uintptr_t, buf, len, 0) }
    func sock_send(s:Socket, buf:*char, len:int) : int { return send(s as uintptr_t, buf, len, 0) }
    func sock_close(s:Socket) { closesocket(s as uintptr_t) }
    func sock_setsockopt(s:Socket, level:int, optname:int, optval:*char, optlen:int) : int { return setsockopt(s as uintptr_t, level, optname, optval, optlen) }
    func sock_getaddrinfo(node:*char, service:*char, hints:*mut char, res:*mut *mut char) : int { return getaddrinfo(node, service, hints, res) }
    func sock_freeaddrinfo(res:*mut char) { freeaddrinfo(res) }

}