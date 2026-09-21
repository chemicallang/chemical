public namespace net {

    public namespace iocp {
        public struct OVERLAPPED {
            var Internal: usize;
            var InternalHigh: usize;
            var Offset: u32;
            var OffsetHigh: u32;
            var hEvent: uintptr_t;
        }

        public struct WSABUF {
            var len: u32;
            var buf: *mut char;
        }

        @dllimport @stdcall @extern protected func CreateIoCompletionPort(FileHandle: uintptr_t, ExistingCompletionPort: uintptr_t, CompletionKey: usize, NumberOfConcurrentThreads: u32) : uintptr_t;
        @dllimport @stdcall @extern protected func GetQueuedCompletionStatus(CompletionPort: uintptr_t, lpNumberOfBytesTransferred: *mut u32, lpCompletionKey: *mut usize, lpOverlapped: *mut *mut OVERLAPPED, dwMilliseconds: u32) : int;
        @dllimport @stdcall @extern protected func PostQueuedCompletionStatus(CompletionPort: uintptr_t, dwNumberOfBytesTransferred: u32, dwCompletionKey: usize, lpOverlapped: *mut OVERLAPPED) : int;
        @dllimport @stdcall @extern protected func WSARecv(s: uintptr_t, lpBuffers: *WSABUF, dwBufferCount: u32, lpNumberOfBytesRecvd: *mut u32, lpFlags: *mut u32, lpOverlapped: *mut OVERLAPPED, lpCompletionRoutine: *void) : int;
        @dllimport @stdcall @extern protected func WSASend(s: uintptr_t, lpBuffers: *WSABUF, dwBufferCount: u32, lpNumberOfBytesSent: *mut u32, dwFlags: u32, lpOverlapped: *mut OVERLAPPED, lpCompletionRoutine: *void) : int;

        public struct AsyncContext {
            var overlapped: OVERLAPPED;
            var buffer: WSABUF;
            var callback: std::function<(ctx: *mut AsyncContext, bytes: u32, ok: bool) => void>;
            var accumulated: std::vector<u8>;
            var read_pos: usize;

            @constructor
            func constructor(buf: *mut char, len: u32) {
                return AsyncContext {
                    overlapped : OVERLAPPED {
                        Internal : 0,
                        InternalHigh : 0,
                        Offset : 0,
                        OffsetHigh : 0,
                        hEvent : 0 as uintptr_t,
                    }
                    buffer : WSABUF {
                        buf : buf,
                        len : len
                    }
                    callback : () => {}
                    accumulated : std::vector<u8>()
                    read_pos : 0u
                }
            }
        }

        public struct CompletionPort {
            var handle: uintptr_t;

            @constructor
            func constructor(threads: u32) {
                // INVALID_HANDLE_VALUE is -1
                return CompletionPort {
                    handle : CreateIoCompletionPort(-1 as uintptr_t, 0 as uintptr_t, 0, threads);
                }
            }

            func register(&self, s: Socket, key: usize) : bool {
                var res = CreateIoCompletionPort(s as uintptr_t, handle, key, 0);
                return res == handle;
            }

            func poll(&self, timeout_ms: u32) : bool {
                var bytes: u32 = 0;
                var key: usize = 0;
                var ov_ptr: *mut OVERLAPPED = null;

                var ok = GetQueuedCompletionStatus(handle, &raw mut bytes, &raw mut key, &raw mut ov_ptr, timeout_ms);
                if (ov_ptr != null) {
                    // Cast back to AsyncContext
                    // AsyncContext has overlapped as first member, so pointers are same
                    var ctx = ov_ptr as *mut AsyncContext;

                    // Pass ctx to callback so it can manage itself
                    ctx.callback(ctx, bytes, ok != 0);

                    return true;
                }
                return false;
            }
        }

        public func async_recv(s: Socket, ctx: *mut AsyncContext) : int {
            var bytes: u32 = 0;
            var flags: u32 = 0;
            return WSARecv(s as uintptr_t, &raw mut ctx.buffer, 1, &raw mut bytes, &raw mut flags, &raw mut ctx.overlapped, null);
        }

        public func async_send(s: Socket, ctx: *mut AsyncContext) : int {
            var bytes: u32 = 0;
            return WSASend(s as uintptr_t, &raw mut ctx.buffer, 1, &raw mut bytes, 0, &raw mut ctx.overlapped, null);
        }

        // ---- async IOCP: overlapped recv / send / accept -------------------
        //
        // `net`'s async socket path drives sockets through a process-wide
        // completion port rather than offloading each call to the thread pool.
        // Every operation is a single overlapped WinSock call; a dedicated
        // dispatcher thread drains the port and wakes the awaiting coroutine.
        //
        // `IocpOp` stores `OVERLAPPED` first so the `lpOverlapped` pointer the
        // kernel returns is the `IocpOp` pointer itself. The op carries the
        // task's `Waker` so a completion on the dispatcher thread wakes the
        // executor immediately instead of waiting for the next poll tick.

        comptime const WSA_IO_PENDING : int = 997
        comptime const SO_UPDATE_ACCEPT_CONTEXT : int = 0x700B

        comptime const IOCP_OP_RECV : int = 1
        comptime const IOCP_OP_SEND : int = 2
        comptime const IOCP_OP_ACCEPT : int = 3

        // AcceptEx needs a buffer large enough for both sockaddr_in structs plus
        // 16 bytes of padding each: 2 * (sizeof(sockaddr_in) + 16).
        comptime const ACCEPT_ADDR_LEN : usize = 32
        comptime const ACCEPT_BUF_LEN : usize = 64

        // NOTE: `AcceptEx` must be exported by the bundled mswsock import
        // library (see the `chemicallang/tcclib` repo's Windows `mswsock.def`).
        @dllimport @stdcall @extern protected func AcceptEx(
            sListenSocket: uintptr_t,
            sAcceptSocket: uintptr_t,
            lpOutputBuffer: *mut void,
            dwReceiveDataLength: u32,
            dwLocalAddressLength: u32,
            dwRemoteAddressLength: u32,
            lpdwBytesReceived: *mut u32,
            lpOverlapped: *mut OVERLAPPED
        ) : int;

        public struct IocpOp {
            var overlapped : OVERLAPPED
            var buffer : WSABUF
            var kind : int
            var socket : Socket
            var listen_socket : Socket
            var accept_buffer : *mut char
            var m : std::mutex
            var done : bool
            var consumed : bool
            var abandoned : bool
            var ok : bool
            var bytes : int
            var waker : core::async::Waker
            var vtbl : *mut core::async::FutureTable<int>
        }

        @retained
        public func iocp_op_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<int> {
            var op = frame as *mut IocpOp
            op.m.lock()
            if(op.done) {
                var okv = op.ok
                var b = op.bytes
                op.consumed = true
                op.m.unlock()
                if(okv) {
                    return core::async::Poll.Ready<int>(b)
                }
                return core::async::Poll.Ready<int>(-1)
            }
            op.waker = cx.waker.clone()
            op.m.unlock()
            return core::async::Poll.Pending<int>()
        }

        @retained
        public func iocp_op_drop(frame : *mut void) {
            var op = frame as *mut IocpOp
            var vtbl = op.vtbl
            var free_it = false
            op.m.lock()
            if(op.done || op.consumed) {
                free_it = true
            } else {
                // the kernel still owns this OVERLAPPED; the dispatcher frees it
                // when the completion arrives
                op.abandoned = true
            }
            op.m.unlock()
            if(free_it) {
                if(op.accept_buffer != null) {
                    unsafe {
                        dealloc op.accept_buffer
                    }
                }
                unsafe {
                    delete op
                }
            }
            if(vtbl != null) {
                unsafe {
                    dealloc vtbl
                }
            }
        }

        func iocp_op_free(op : *mut IocpOp) {
            if(op.accept_buffer != null) {
                unsafe {
                    dealloc op.accept_buffer
                }
            }
            unsafe {
                delete op
            }
        }

        // Runs on the dispatcher thread. The executor thread only holds the op
        // mutex for the duration of a poll.
        func iocp_op_complete(op : *mut IocpOp, bytes : u32, ok : bool) {
            op.m.lock()
            if(op.kind == IOCP_OP_ACCEPT) {
                if(ok) {
                    var lsn = op.listen_socket
                    sock_setsockopt(op.socket, 0xFFFF as int, SO_UPDATE_ACCEPT_CONTEXT as int, (&raw mut lsn) as *char, sizeof(Socket) as int)
                    op.bytes = op.socket as int
                } else {
                    sock_close(op.socket)
                    op.bytes = -1
                }
            } else {
                op.bytes = bytes as int
            }
            op.ok = ok
            op.done = true
            var ab = op.abandoned
            var w = op.waker.clone()
            op.m.unlock()
            if(ab) {
                iocp_op_free(op)
                return
            }
            if(w.vtbl != null) {
                w.wake()
            }
        }

        @never_destructed
        var g_async_iocp : CompletionPort
        var g_async_iocp_ready : bool = false
        var g_async_iocp_lock : std::mutex
        var g_async_iocp_started : bool = false

        func async_iocp_dispatch_entry(arg : *mut void) : *mut void {
            var cp = arg as *mut CompletionPort
            while(true) {
                var bytes : u32 = 0
                var key : usize = 0
                var ov_ptr : *mut OVERLAPPED = null
                var ok = GetQueuedCompletionStatus(cp.handle, &raw mut bytes, &raw mut key, &raw mut ov_ptr, 0xFFFFFFFFu)
                if(ov_ptr != null) {
                    iocp_op_complete(ov_ptr as *mut IocpOp, bytes, ok != 0)
                }
            }
            return null
        }

        // Lazily create the process-wide completion port and start its single
        // dispatcher thread.
        public func async_iocp_port() : *mut CompletionPort {
            g_async_iocp_lock.lock()
            if(!g_async_iocp_ready) {
                new(&raw mut g_async_iocp) CompletionPort(1u)
                g_async_iocp_ready = true
            }
            if(!g_async_iocp_started) {
                g_async_iocp_started = true
                std::concurrent.spawn(async_iocp_dispatch_entry, &raw mut g_async_iocp)
            }
            g_async_iocp_lock.unlock()
            return &raw mut g_async_iocp
        }

        func iocp_op_make(s : Socket, kind : int) : *mut IocpOp {
            var op = malloc(sizeof(IocpOp)) as *mut IocpOp
            var vtbl = malloc(sizeof(core::async::FutureTable<int>)) as *mut core::async::FutureTable<int>
            vtbl.poll = iocp_op_poll
            vtbl.drop = iocp_op_drop
            new(op) IocpOp {
                overlapped : OVERLAPPED {
                    Internal : 0,
                    InternalHigh : 0,
                    Offset : 0,
                    OffsetHigh : 0,
                    hEvent : 0 as uintptr_t
                },
                buffer : WSABUF { len : 0u, buf : null },
                kind : kind,
                socket : s,
                listen_socket : 0 as Socket,
                accept_buffer : null,
                m : std::mutex(),
                done : false,
                consumed : false,
                abandoned : false,
                ok : false,
                bytes : 0,
                waker : core::async::Waker { data : null, vtbl : null },
                vtbl : vtbl
            }
            return op
        }

        func iocp_op_failed(s : Socket, kind : int) : *mut IocpOp {
            var op = iocp_op_make(s, kind)
            op.m.lock()
            op.done = true
            op.ok = false
            op.bytes = -1
            op.m.unlock()
            return op
        }

        func iocp_op_handle(op : *mut IocpOp) : core::async::FutureHandle<int> {
            var v = op.vtbl
            return core::async::FutureHandle<int> { frame : op as *mut void, vtbl : v }
        }

        // Post one overlapped WSARecv. Resolves with the bytes read (0 = EOF,
        // -1 = error).
        public func async_iocp_recv(s : Socket, buf : *mut u8, cap : usize) : core::async::FutureHandle<int> {
            var op = iocp_op_make(s, IOCP_OP_RECV)
            op.buffer.len = cap as u32
            op.buffer.buf = buf as *mut char
            var cp = async_iocp_port()
            cp.register(s, 0 as usize)
            var bytes : u32 = 0
            var flags : u32 = 0
            var rc = WSARecv(s as uintptr_t, &raw mut op.buffer, 1u, &raw mut bytes, &raw mut flags, &raw mut op.overlapped, null)
            if(rc != 0 && WSAGetLastError() != WSA_IO_PENDING) {
                op.m.lock()
                op.done = true
                op.ok = false
                op.bytes = -1
                op.m.unlock()
            }
            return iocp_op_handle(op)
        }

        // Post one overlapped WSASend. Resolves with the bytes written (may be
        // fewer than `len`) or -1 on error; the caller loops to send it all.
        public func async_iocp_send(s : Socket, data : *char, len : int) : core::async::FutureHandle<int> {
            var op = iocp_op_make(s, IOCP_OP_SEND)
            op.buffer.len = len as u32
            op.buffer.buf = data as *mut char
            var cp = async_iocp_port()
            cp.register(s, 0 as usize)
            var bytes : u32 = 0
            var rc = WSASend(s as uintptr_t, &raw mut op.buffer, 1u, &raw mut bytes, 0u, &raw mut op.overlapped, null)
            if(rc != 0 && WSAGetLastError() != WSA_IO_PENDING) {
                op.m.lock()
                op.done = true
                op.ok = false
                op.bytes = -1
                op.m.unlock()
            }
            return iocp_op_handle(op)
        }

        // Post one overlapped AcceptEx. Resolves with the accepted socket, or -1.
        public func async_iocp_accept(listen_s : Socket) : core::async::FutureHandle<int> {
            var accept_sock = sock_socket(AF_INET as int, SOCK_STREAM as int, IPPROTO_TCP as int)
            if(accept_sock == 0 as Socket || (accept_sock as longlong) < 0) {
                return iocp_op_handle(iocp_op_failed(0 as Socket, IOCP_OP_ACCEPT))
            }
            var abuf = malloc(ACCEPT_BUF_LEN) as *mut char
            var op = iocp_op_make(accept_sock, IOCP_OP_ACCEPT)
            op.listen_socket = listen_s
            op.accept_buffer = abuf
            var cp = async_iocp_port()
            cp.register(listen_s, 0 as usize)
            var bytes : u32 = 0
            var rc = AcceptEx(
                listen_s as uintptr_t,
                accept_sock as uintptr_t,
                abuf as *mut void,
                0u,
                ACCEPT_ADDR_LEN as u32,
                ACCEPT_ADDR_LEN as u32,
                &raw mut bytes,
                &raw mut op.overlapped
            )
            if(rc != 0 && WSAGetLastError() != WSA_IO_PENDING) {
                sock_close(accept_sock)
                op.m.lock()
                op.done = true
                op.ok = false
                op.bytes = -1
                op.m.unlock()
            }
            return iocp_op_handle(op)
        }
    }

}