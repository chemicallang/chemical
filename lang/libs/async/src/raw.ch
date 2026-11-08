// Raw async-runtime state and frame hooks.
//
// Ordinary code uses the safe `async` surface (`block_on`, `spawn_send`,
// `executor_drive`, `Channel`, `spawn_blocking`). The executor/reactor/frame
// internals live here because they exchange process-global pointers, type-erased
// frames, or waiter queues that safe code must not forge.
public namespace async {

public namespace raw {

// Type-erased poll/drop table for a spawned task. Non-generic on purpose (B20).
public struct TaskVTable {
    internal var poll : (frame : *mut void, cx : *mut core::async::Context) => core::async::Poll<core::async::Unit>
    internal var drop : (frame : *mut void) => void
}

// A queued, type-erased task. Non-generic on purpose (B20).
public struct RawTask {
    internal var frame : *mut void
    internal var vtbl : *mut TaskVTable
}

// Invoked (outside the executor lock) every time a waker fires. A native event
// loop installs one so an off-thread completion wakes the loop immediately
// instead of waiting for the next poll tick (Tier 5, `window_run_async`).
public type ExecWakeHook = (arg : *mut void) => void

public struct Executor {
    // Raw executor state is private to this module. Native loops drive it
    // through `executor_drive`; the unchecked entry points remain `unsafe`.
    internal var m : std::mutex
    internal var cv : std::condvar
    internal var queue : std::vector<RawTask>
    internal var woken : bool
    internal var wake_hook : ExecWakeHook
    internal var wake_hook_arg : *mut void
}

// Runtime-owned state. These values are not exchanged through the safe
// cross-thread API; keep them !Send/!Sync even if their representation later
// stops containing a raw pointer.
impl !Send for TaskVTable { }
impl !Sync for TaskVTable { }
impl !Send for RawTask { }
impl !Sync for RawTask { }
impl !Send for Executor { }
impl !Sync for Executor { }

@never_destructed
var g_exec : Executor
var g_exec_ready : bool = false

@never_destructed
var g_exec_waker_vtbl : core::async::WakerVTable
var g_exec_waker_vtbl_ready : bool = false

func exec_wake(data : *mut void) {
        unsafe {
        var e = data as *mut Executor
        e.m.lock()
        e.woken = true
        var hook = e.wake_hook
        var hook_arg = e.wake_hook_arg
        e.cv.notify_all()
        e.m.unlock()
        // Run the hook outside the lock: it posts to a native event loop, which
        // must not be able to re-enter the executor while the lock is held.
        if(hook != null) {
            hook(hook_arg)
        }

        }
    }

func exec_clone_waker(data : *mut void) : core::async::Waker {
        unsafe {
        return unsafe(core::async::Waker.from_raw(data, &raw mut g_exec_waker_vtbl))

        }
    }

func exec_drop_waker(data : *mut void) { }

// Populate the waker vtable. Split out of executor() because exec_make_waker
// hands out a pointer to this global and must guarantee it is usable: the
// struct is zero-initialized, so a Waker built before this runs carries a
// vtable of null function pointers and the first waker.clone() dereferences
// one (observed as "invalid memory access" inside core::async::Waker::clone,
// non-deterministically, depending on which entry point touched the executor
// first in a given process).
func ensure_exec_waker_vtbl() {
        unsafe {
        if(!g_exec_waker_vtbl_ready) {
            g_exec_waker_vtbl = core::async::WakerVTable {
                wake : exec_wake,
                clone : exec_clone_waker,
                drop : exec_drop_waker
            }
            g_exec_waker_vtbl_ready = true
        }

        }
    }

// The process-wide executor. Created lazily; intentionally never torn down.
public func executor() : *mut Executor {
        unsafe {
        if(!g_exec_ready) {
            g_exec = Executor {
                m : std::mutex(),
                cv : std::CondVar(),
                queue : std::vector<RawTask>(),
                woken : false,
                wake_hook : null,
                wake_hook_arg : null
            }
            g_exec_ready = true
        }
        ensure_exec_waker_vtbl()
        return &raw mut g_exec

        }
    }

public unsafe func exec_make_waker(e : *mut Executor) : core::async::Waker {
        if(e == null) {
            return core::async::Waker.empty()
        }
        unsafe {
        ensure_exec_waker_vtbl()
        return unsafe(core::async::Waker.from_raw(e as *mut void, &raw mut g_exec_waker_vtbl))

        }
    }

// Install a hook that runs whenever the executor is woken. Used by native event
// loops (see `window_run_async`) to post a wake to their own queue so a task
// completing on a worker thread is re-polled promptly. Pass `null` to clear.
public unsafe func executor_set_wake_hook(e : *mut Executor, hook : ExecWakeHook, arg : *mut void) {
        if(e == null) {
            return
        }
        unsafe {
        e.m.lock()
        e.wake_hook = hook
        e.wake_hook_arg = arg
        e.m.unlock()

        }
    }

public unsafe func exec_enqueue_task(e : *mut Executor, frame : *mut void, vtbl : *mut TaskVTable) {
        if(e == null || frame == null || vtbl == null) {
            return
        }
        unsafe {
        e.m.lock()
        e.queue.push_back(RawTask { frame : frame, vtbl : vtbl })
        e.cv.notify_all()
        e.m.unlock()

        }
    }

// Poll up to `max` queued tasks once each. Tasks that stay `Pending` are moved
// to the back of the queue so that a single never-ready task cannot starve the
// others. Returns the number of tasks that completed.
public unsafe func executor_poll_tasks(e : *mut Executor, max : uint) : uint {
        if(e == null) {
            return 0u
        }
        unsafe {
        var done : uint = 0u
        var i : uint = 0u
        while(i < max) {
            e.m.lock()
            if(e.queue.size() == 0u) {
                e.m.unlock()
                break
            }
            var pe = e.queue.get_ptr(0)
            var t = std::replace(unsafe(&mut *pe), RawTask { frame : null, vtbl : null })
            e.queue.remove(0)
            e.m.unlock()
            if(t.frame == null || t.vtbl == null) {
                i = i + 1u
                continue
            }
            var cx = core::async::Context { waker : exec_make_waker(e) }
            var r = t.vtbl.poll(t.frame, &raw mut cx)
            if(r is core::async::Poll.Ready) {
                t.vtbl.drop(t.frame)
                done = done + 1u
            } else {
                e.m.lock()
                e.queue.push_back(RawTask { frame : t.frame, vtbl : t.vtbl })
                e.m.unlock()
            }
            i = i + 1u
        }
        return done

        }
    }

// Safe driver for native event loops: poll up to `max` queued tasks without
// exposing the executor's queue or synchronization state. The raw entry points
// (`executor_poll_tasks`, `exec_enqueue_task`, ...) stay `unsafe`.
public func executor_drive(max : uint) : uint {
    unsafe {
        return executor_poll_tasks(executor(), max)
    }
}

// Park the executor thread until a waker fires or `millis` elapse. The timeout
// is what keeps futures that do not use their waker (timers, `spawn_blocking`)
// making progress.
public unsafe func executor_wait(e : *mut Executor, millis : ulong) {
        if(e == null) {
            return
        }
        unsafe {
        var wait_ms = millis
        if(reactor_has_fds()) {
            // block on the fd reactor (it wakes tasks via their wakers); the
            // condvar is then used only to drain non-reactor wakers
            reactor_poll(millis as int)
            wait_ms = 0u
        }
        e.m.lock()
        e.woken = false
        e.cv.timed_wait(&mut e.m, wait_ms)
        e.m.unlock()

        }
    }

// `spawn` join state, shared (refcounted) by the join handle and the task.
public struct JoinState<T> {
    internal var m : std::mutex
    internal var refs : int
    internal var done : bool
    internal var consumed : bool
    internal var cancelled : bool
    // The task stores its whole `Poll<T>` here rather than an `Option<T>`:
    // moving a `T` payload bound by a variant pattern is rejected in generic
    // code (B18), while moving the whole local `Poll<T>` is allowed.
    internal var result : core::async::Poll<T>
    internal var waker : core::async::Waker
    internal var vtbl : *mut core::async::FutureTable<T>
}

// The executor-owned, type-erased task frame.
public struct SpawnTask<T> {
    internal var state : *mut JoinState<T>
    internal var inner : core::async::FutureHandle<T>
    internal var vtbl : *mut TaskVTable
}

impl<T> !Send for JoinState<T> { }
impl<T> !Sync for JoinState<T> { }
impl<T> !Send for SpawnTask<T> { }
impl<T> !Sync for SpawnTask<T> { }

@retained
public func <T> spawn_join_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<T> {
        unsafe {
        var st = frame as *mut JoinState<T>
        st.m.lock()
        if(st.done) {
            // move the stored `Poll<T>` out without calling `std::replace` (B20),
            // then leave a valid `Pending` in its place
            var r : core::async::Poll<T>
            unsafe {
                memcpy(&raw mut r, &raw mut st.result, sizeof(core::async::Poll<T>))
                new(&raw mut st.result) core::async::Poll.Pending<T>()
            }
            st.done = false
            st.consumed = true
            st.m.unlock()
            return r
        }
        if(!cx.waker.is_empty()) {
            st.waker = cx.waker.clone()
        }
        st.m.unlock()
        return core::async::Poll.Pending<T>()

        }
    }

@retained
public func <T> spawn_join_drop(frame : *mut void) {
        unsafe {
        var st = frame as *mut JoinState<T>
        var vtbl = st.vtbl
        st.m.lock()
        st.cancelled = true
        st.refs = st.refs - 1
        var free_state = st.refs == 0
        st.m.unlock()
        if(free_state) {
            unsafe {
                delete st
            }
        }
        if(vtbl != null) {
            unsafe {
                dealloc vtbl
            }
        }

        }
    }

@retained
public func <T> spawn_task_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<core::async::Unit> {
        unsafe {
        var t = frame as *mut SpawnTask<T>
        var st = t.state
        st.m.lock()
        var cancelled = st.cancelled
        st.m.unlock()
        if(cancelled) {
            return core::async::Poll.Ready<core::async::Unit>(core::async::Unit { })
        }
        var r = t.inner.vtbl_ptr().poll(t.inner.frame_ptr(), cx)
        if(r is core::async::Poll.Ready) {
            st.m.lock()
            var cancelled_now = st.cancelled
            if(!cancelled_now) {
                st.result = r
                st.done = true
                if(!st.waker.is_empty()) {
                    var w = st.waker.clone()
                    st.m.unlock()
                    w.wake()
                } else {
                    st.m.unlock()
                }
            } else {
                st.m.unlock()
            }
            return core::async::Poll.Ready<core::async::Unit>(core::async::Unit { })
        }
        return core::async::Poll.Pending<core::async::Unit>()

        }
    }

@retained
public func <T> spawn_task_drop(frame : *mut void) {
        unsafe {
        var t = frame as *mut SpawnTask<T>
        var st = t.state
        var vtbl = t.vtbl
        unsafe {
            delete t
        }
        st.m.lock()
        st.refs = st.refs - 1
        var free_state = st.refs == 0
        st.m.unlock()
        if(free_state) {
            unsafe {
                delete st
            }
        }
        if(vtbl != null) {
            unsafe {
                dealloc vtbl
            }
        }

        }
    }

// Submit `handle` to the executor and return a join handle. Await the result
// with `await spawn(...)` or read it with `async::block_on(spawn(...))`.
//
// The executor is process-wide: the task may be polled on another thread. The
// frame's captures are type-erased, so their `Send`/`'static` properties cannot
// be checked here; submission is an unsafe boundary until erased-capture Send
// tracking lands (S6.5).
public unsafe func <'a, T> spawn(handle : core::async::FutureHandle<'a, T>) : core::async::FutureHandle<'a, T> {
        unsafe {
        var e = executor()
        var st = unsafe(malloc(sizeof(JoinState<T>))) as *mut JoinState<T>
        if(st == null) {
            panic("failed to allocate spawn join state")
        }
        var join_vtbl = unsafe(malloc(sizeof(core::async::FutureTable<T>))) as *mut core::async::FutureTable<T>
        if(join_vtbl == null) {
            unsafe { dealloc st }
            panic("failed to allocate spawn join vtable")
        }
        join_vtbl.poll = spawn_join_poll<T>
        join_vtbl.drop = spawn_join_drop<T>
        new(st) JoinState<T> {
            m : std::mutex(),
            refs : 2,
            done : false,
            consumed : false,
            cancelled : false,
            result : core::async::Poll.Pending<T>(),
            waker : core::async::Waker.empty(),
            vtbl : join_vtbl
        }
        var task = unsafe(malloc(sizeof(SpawnTask<T>))) as *mut SpawnTask<T>
        if(task == null) {
            unsafe {
                dealloc join_vtbl
                dealloc st
            }
            panic("failed to allocate spawn task")
        }
        var task_vtbl = unsafe(malloc(sizeof(TaskVTable))) as *mut TaskVTable
        if(task_vtbl == null) {
            unsafe {
                dealloc task
                dealloc join_vtbl
                dealloc st
            }
            panic("failed to allocate spawn task vtable")
        }
        task_vtbl.poll = spawn_task_poll<T>
        task_vtbl.drop = spawn_task_drop<T>
        new(task) SpawnTask<T> {
            state : st,
            inner : handle,
            vtbl : task_vtbl
        }
        exec_enqueue_task(e, task as *mut void, task_vtbl)
        return unsafe(core::async::future_handle_raw<T>(st as *mut void, join_vtbl))

        }
    }

// Safe submission for a runtime future whose frame the compiler can inspect.
// The call-site check (`SAFE-SEND-001`) requires the operand to be a direct
// async function call and verifies every frame local/parameter is Send and
// 'static. The implementation then crosses the existing unsafe boundary.
public func <T> spawn_send(handle : core::async::FutureHandle<'static, T>) : core::async::SendFutureHandle<T> where T : Send {
        var joined = unsafe(spawn<T>(handle))
        return unsafe(core::async.future_handle_into_send<T>(&mut joined))
}

// Submit a future that already crossed the safe Send boundary (for example the
// result of `spawn_send`). The frame witness is carried by SendFutureHandle, so
// this is safe without re-inspecting an erased FutureHandle. This is the stored
// future form of `spawn_send`.
public func <T> spawn_send_handle(handle : core::async::SendFutureHandle<T>) : core::async::SendFutureHandle<T> where T : Send {
        var owned = handle
        var joined = unsafe(spawn<T>(unsafe(owned.into_future())))
        return unsafe(core::async.future_handle_into_send<T>(&mut joined))
}

// Submit a task to the executor for a UI/event loop to drive. The executor is
// process-wide, so this is currently the same submission as `spawn`; the
// distinction is the driver. `window_run_async` (Tier 5) drives this executor
// from the native event loop on the UI thread and posts off-thread wakeups
// back to that loop, so a task submitted here is polled on the UI thread and
// its continuation after an `await` runs there too.
//
// Until the executor actually enforces that affinity and captures are checked,
// this has the same unchecked-capture contract as `spawn` and is unsafe.
public unsafe func <'a, T> spawn_local(handle : core::async::FutureHandle<'a, T>) : core::async::FutureHandle<'a, T> {
        unsafe {
        return unsafe(spawn<T>(handle))

        }
    }

public struct ReactorFd {
    var fd : int
    var events : short
    var read_waker : core::async::Waker
    var write_waker : core::async::Waker
    var next : *mut ReactorFd
}

public struct Reactor {
    var m : std::mutex
    var head : *mut ReactorFd
}

@never_destructed
var g_reactor : Reactor
var g_reactor_ready : bool = false

public func reactor() : *mut Reactor {
        unsafe {
        if(!g_reactor_ready) {
            g_reactor = Reactor {
                m : std::mutex(),
                head : null
            }
            g_reactor_ready = true
        }
        return &raw mut g_reactor

        }
    }

public func reactor_register(fd : int, events : short, waker : core::async::Waker) {
        unsafe {
        var e = reactor()
        e.m.lock()
        var fe = e.head
        while(fe != null) {
            if(fe.fd == fd) {
                if((events & posix::POLLIN) != 0) {
                    fe.read_waker = waker.clone()
                }
                if((events & posix::POLLOUT) != 0) {
                    fe.write_waker = waker.clone()
                }
                var previous = fe.events
                fe.events = fe.events | events
                var merged = fe.events
                if(merged != previous) {
                    posix::reactor_platform_register(fd, merged, false)
                }
                e.m.unlock()
                return
            }
            fe = fe.next
        }
        var rw = core::async::Waker.empty()
        var ww = core::async::Waker.empty()
        if((events & posix::POLLIN) != 0) {
            rw = waker.clone()
        }
        if((events & posix::POLLOUT) != 0) {
            ww = waker.clone()
        }
        var node = unsafe(malloc(sizeof(ReactorFd))) as *mut ReactorFd
        if(node == null) {
            e.m.unlock()
            panic("failed to allocate reactor node")
        }
        new(node) ReactorFd {
            fd : fd,
            events : events,
            read_waker : rw,
            write_waker : ww,
            next : e.head
        }
        e.head = node
        posix::reactor_platform_register(fd, events, true)
        e.m.unlock()

        }
    }

public func reactor_has_fds() : bool {
        unsafe {
        var e = reactor()
        e.m.lock()
        var h = e.head
        e.m.unlock()
        return h != null

        }
    }

// Poll the registered fds once, waking any task whose fd became ready. Blocks
// up to `timeout_ms` (0 = non-blocking). Returns the number of ready fds.
public func reactor_poll(timeout_ms : int) : int {
        unsafe {
        var e = reactor()
        e.m.lock()
        var n = 0u
        var it = e.head
        while(it != null) {
            n = n + 1u
            it = it.next
        }
        if(n == 0u) {
            e.m.unlock()
            return 0
        }
        var pfd = unsafe(malloc(n * sizeof(posix::PollFd))) as *mut posix::PollFd
        if(pfd == null) {
            e.m.unlock()
            panic("failed to allocate reactor poll set")
        }
        var idx = 0u
        it = e.head
        while(it != null) {
            pfd[idx].fd = it.fd
            pfd[idx].events = it.events
            pfd[idx].revents = 0
            it = it.next
            idx = idx + 1u
        }
        e.m.unlock()
        var rc = posix::reactor_platform_poll(pfd, n as ulong, timeout_ms)
        if(rc > 0) {
            e.m.lock()
            idx = 0u
            it = e.head
            while(it != null) {
                var rev = pfd[idx].revents
                if((rev & (posix::POLLIN | posix::POLLERR | posix::POLLHUP)) != 0 && !it.read_waker.is_empty()) {
                    var w = it.read_waker.clone()
                    w.wake()
                }
                if((rev & posix::POLLOUT) != 0 && !it.write_waker.is_empty()) {
                    var w2 = it.write_waker.clone()
                    w2.wake()
                }
                it = it.next
                idx = idx + 1u
            }
            e.m.unlock()
        }
        unsafe {
            dealloc pfd
        }
        return rc

        }
    }

// An awaitable readiness future for a raw fd.
public struct ReadyState {
    var fd : int
    var events : short
    var parked : bool
    var vtbl : *mut core::async::FutureTable<core::async::Unit>
}

@retained
public func ready_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<core::async::Unit> {
        unsafe {
        var st = frame as *mut ReadyState
        // Register and yield once before reporting readiness. Awaiting a future that
        // is `Ready` on the very first poll currently mis-lowers on LLVM (a coroutine
        // that never suspends is destroyed through an `unreachable` cleanup block —
        // B22). One `Pending` pass also matches how `yield_now` behaves.
        if(st.parked && posix::reactor_is_ready(st.fd, st.events)) {
            return core::async::Poll.Ready<core::async::Unit>(core::async::Unit { })
        }
        st.parked = true
        if(!cx.waker.is_empty()) {
            reactor_register(st.fd, st.events, cx.waker.clone())
        }
        return core::async::Poll.Pending<core::async::Unit>()

        }
    }

@retained
public func ready_drop(frame : *mut void) {
        unsafe {
        var st = frame as *mut ReadyState
        var vtbl = st.vtbl
        unsafe {
            delete st
        }
        if(vtbl != null) {
            unsafe {
                dealloc vtbl
            }
        }

        }
    }

func make_ready(fd : int, events : short) : core::async::FutureHandle<'static, core::async::Unit> {
        unsafe {
        var st = unsafe(malloc(sizeof(ReadyState))) as *mut ReadyState
        if(st == null) {
            panic("failed to allocate ready state")
        }
        var vtbl = unsafe(malloc(sizeof(core::async::FutureTable<core::async::Unit>))) as *mut core::async::FutureTable<core::async::Unit>
        if(vtbl == null) {
            unsafe { dealloc st }
            panic("failed to allocate ready vtable")
        }
        vtbl.poll = ready_poll
        vtbl.drop = ready_drop
        new(st) ReadyState {
            fd : fd,
            events : events,
            parked : false,
            vtbl : vtbl
        }
        return unsafe(core::async::future_handle_raw<core::async::Unit>(st as *mut void, vtbl))

        }
    }

public struct BlockingState<T> {
    internal var m : std::mutex
    internal var ready : bool
    internal var consumed : bool
    internal var abandoned : bool
    internal var value : std::Option<T>
    internal var task : std::function<() => T>
    internal var vtbl : *mut core::async::FutureTable<T>
}

impl<T> !Send for BlockingState<T> { }
impl<T> !Sync for BlockingState<T> { }

@retained
public func <T> blocking_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<T> {
        unsafe {
        var st = frame as *mut BlockingState<T>
        st.m.lock()
        if(!st.ready) {
            st.m.unlock()
            return core::async::Poll.Pending<T>()
        }
        var taken = st.value.take()
        st.ready = false
        st.consumed = true
        st.m.unlock()
        return core::async::Poll.Ready<T>(taken)

        }
    }

@retained
public func <T> blocking_drop(frame : *mut void) {
        unsafe {
        var st = frame as *mut BlockingState<T>
        var vtbl = st.vtbl
        var free_state = false
        st.m.lock()
        if(st.consumed) {
            // consumer already took the value; nothing left to destroy here
            free_state = true
        } else if(st.abandoned) {
            // worker is still running and will free the state itself
            free_state = false
        } else if(st.ready) {
            // worker finished but the result was never consumed; destroy it here
            st.ready = false
            free_state = true
        } else {
            // worker has not finished yet; hand ownership of the state to it
            st.abandoned = true
            free_state = false
        }
        st.m.unlock()
        if(free_state) {
            unsafe {
                delete st
            }
        }
        if(vtbl != null) {
            unsafe {
                dealloc vtbl
            }
        }

        }
    }

// Lazily created, process-wide blocking pool. `ThreadPool` owns native worker
// threads and frees them in its destructor; it is intentionally never torn down
// before process exit.
@never_destructed
var g_blocking_pool : std::concurrent.ThreadPool
var g_blocking_pool_ready : bool = false

@retained
public func blocking_pool() : *mut std::concurrent.ThreadPool {
        unsafe {
        if(!g_blocking_pool_ready) {
            var n = std::concurrent.hardware_threads()
            if(n < 1u) {
                n = 1u
            }
            g_blocking_pool = std::concurrent.create_pool(n as uint)
            g_blocking_pool_ready = true
        }
        return &raw mut g_blocking_pool

        }
    }

    @no_mangle
    public func chemical_async_frame_alloc(size : u64, align : u64) : *mut void {
        var frame = unsafe(malloc(size))
        if(frame == null) {
            panic("failed to allocate async frame")
        }
        return frame
    }

    @no_mangle
    public func chemical_async_frame_free(ptr : *mut void, size : u64, align : u64) {
        unsafe { free(ptr) }
    }

} // namespace raw

} // namespace async
