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

                var ok = GetQueuedCompletionStatus(handle, &mut bytes, &mut key, &mut ov_ptr, timeout_ms);
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
            return WSARecv(s as uintptr_t, &mut ctx.buffer, 1, &mut bytes, &mut flags, &mut ctx.overlapped, null);
        }

        public func async_send(s: Socket, ctx: *mut AsyncContext) : int {
            var bytes: u32 = 0;
            return WSASend(s as uintptr_t, &mut ctx.buffer, 1, &mut bytes, 0, &mut ctx.overlapped, null);
        }
    }

}