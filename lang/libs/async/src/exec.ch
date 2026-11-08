// The task executor and `spawn` (design Section 12, D7).
//
// `block_on` is the bootstrap executor: it polls its own handle and, between
// polls, drains a process-wide task queue. `spawn(h)` submits a runtime future
// to that queue and returns a join handle that becomes `Ready` with the task's
// result.
//
// This is a cooperative "poll everything" executor, not a waker-scheduled one:
// every `block_on` iteration polls each outstanding task once and then parks
// the thread briefly (or until a waker notifies it). That is enough for the
// futures the runtime authors today (`yield_now`, `sleep`, `spawn_blocking`,
// `timeout`, `select`) and requires no per-task waker plumbing. A readiness
// reactor (Tier 1) will replace the park with real wakeups.
//
// Two shapes here are deliberately non-generic, to avoid a compiler bug (B20)
// where accessing a field of a *concretely instantiated* generic type from
// inside a generic function resolves to the master's field type:
//   * `TaskVTable` / `RawTask` type-erase a task without `FutureTable<Unit>`.
//   * the join result is pulled out with `memcpy`, not `std::replace`.
//
// Ownership: the executor owns the type-erased task frame (`SpawnTask<T>`) and
// frees it after it completes. The join handle and the task share a
// refcounted `JoinState<T>`; the last one out frees it. Dropping the join
// handle cancels the task (best effort): the task observes `cancelled`, reports
// `Ready`, and is reclaimed on the next executor pass.
public namespace async {

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

}
