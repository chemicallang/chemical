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
    var poll : (frame : *mut void, cx : *mut core::async::Context) => core::async::Poll<core::async::Unit>
    var drop : (frame : *mut void) => void
}

// A queued, type-erased task. Non-generic on purpose (B20).
public struct RawTask {
    var frame : *mut void
    var vtbl : *mut TaskVTable
}

// Invoked (outside the executor lock) every time a waker fires. A native event
// loop installs one so an off-thread completion wakes the loop immediately
// instead of waiting for the next poll tick (Tier 5, `window_run_async`).
public type ExecWakeHook = (arg : *mut void) => void

public struct Executor {
    var m : std::mutex
    var cv : std::condvar
    var queue : std::vector<RawTask>
    var woken : bool
    var wake_hook : ExecWakeHook
    var wake_hook_arg : *mut void
}

@never_destructed
var g_exec : Executor
var g_exec_ready : bool = false

@never_destructed
var g_exec_waker_vtbl : core::async::WakerVTable
var g_exec_waker_vtbl_ready : bool = false

func exec_wake(data : *mut void) {
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

func exec_clone_waker(data : *mut void) : core::async::Waker {
    return core::async::Waker { data : data, vtbl : &raw mut g_exec_waker_vtbl }
}

func exec_drop_waker(data : *mut void) { }

// The process-wide executor. Created lazily; intentionally never torn down.
public func executor() : *mut Executor {
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
    if(!g_exec_waker_vtbl_ready) {
        g_exec_waker_vtbl = core::async::WakerVTable {
            wake : exec_wake,
            clone : exec_clone_waker,
            drop : exec_drop_waker
        }
        g_exec_waker_vtbl_ready = true
    }
    return &raw mut g_exec
}

public func exec_make_waker(e : *mut Executor) : core::async::Waker {
    return core::async::Waker { data : e as *mut void, vtbl : &raw mut g_exec_waker_vtbl }
}

// Install a hook that runs whenever the executor is woken. Used by native event
// loops (see `window_run_async`) to post a wake to their own queue so a task
// completing on a worker thread is re-polled promptly. Pass `null` to clear.
public func executor_set_wake_hook(e : *mut Executor, hook : ExecWakeHook, arg : *mut void) {
    e.m.lock()
    e.wake_hook = hook
    e.wake_hook_arg = arg
    e.m.unlock()
}

public func exec_enqueue_task(e : *mut Executor, frame : *mut void, vtbl : *mut TaskVTable) {
    e.m.lock()
    e.queue.push_back(RawTask { frame : frame, vtbl : vtbl })
    e.cv.notify_all()
    e.m.unlock()
}

// Poll up to `max` queued tasks once each. Tasks that stay `Pending` are moved
// to the back of the queue so that a single never-ready task cannot starve the
// others. Returns the number of tasks that completed.
public func executor_poll_tasks(e : *mut Executor, max : uint) : uint {
    var done : uint = 0u
    var i : uint = 0u
    while(i < max) {
        e.m.lock()
        if(e.queue.size() == 0u) {
            e.m.unlock()
            break
        }
        var pe = e.queue.get_ptr(0)
        var t = std::replace(&mut *pe, RawTask { frame : null, vtbl : null })
        e.queue.remove(0)
        e.m.unlock()
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

// Park the executor thread until a waker fires or `millis` elapse. The timeout
// is what keeps futures that do not use their waker (timers, `spawn_blocking`)
// making progress.
public func executor_wait(e : *mut Executor, millis : ulong) {
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

// `spawn` join state, shared (refcounted) by the join handle and the task.
public struct JoinState<T> {
    var m : std::mutex
    var refs : int
    var done : bool
    var consumed : bool
    var cancelled : bool
    // The task stores its whole `Poll<T>` here rather than an `Option<T>`:
    // moving a `T` payload bound by a variant pattern is rejected in generic
    // code (B18), while moving the whole local `Poll<T>` is allowed.
    var result : core::async::Poll<T>
    var waker : core::async::Waker
    var vtbl : *mut core::async::FutureTable<T>
}

// The executor-owned, type-erased task frame.
public struct SpawnTask<T> {
    var state : *mut JoinState<T>
    var inner : core::async::FutureHandle<T>
    var vtbl : *mut TaskVTable
}

@retained
public func <T> spawn_join_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<T> {
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
    if(cx.waker.vtbl != null) {
        st.waker = cx.waker.clone()
    }
    st.m.unlock()
    return core::async::Poll.Pending<T>()
}

@retained
public func <T> spawn_join_drop(frame : *mut void) {
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

@retained
public func <T> spawn_task_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<core::async::Unit> {
    var t = frame as *mut SpawnTask<T>
    var st = t.state
    st.m.lock()
    var cancelled = st.cancelled
    st.m.unlock()
    if(cancelled) {
        return core::async::Poll.Ready<core::async::Unit>(core::async::Unit { })
    }
    var r = t.inner.vtbl.poll(t.inner.frame, cx)
    if(r is core::async::Poll.Ready) {
        st.m.lock()
        var cancelled_now = st.cancelled
        if(!cancelled_now) {
            st.result = r
            st.done = true
            if(st.waker.vtbl != null) {
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

@retained
public func <T> spawn_task_drop(frame : *mut void) {
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

// Submit `handle` to the executor and return a join handle. Await the result
// with `await spawn(...)` or read it with `async::block_on(spawn(...))`.
public func <T> spawn(handle : core::async::FutureHandle<T>) : core::async::FutureHandle<T> {
    var e = executor()
    var st = malloc(sizeof(JoinState<T>)) as *mut JoinState<T>
    var join_vtbl = malloc(sizeof(core::async::FutureTable<T>)) as *mut core::async::FutureTable<T>
    join_vtbl.poll = spawn_join_poll<T>
    join_vtbl.drop = spawn_join_drop<T>
    new(st) JoinState<T> {
        m : std::mutex(),
        refs : 2,
        done : false,
        consumed : false,
        cancelled : false,
        result : core::async::Poll.Pending<T>(),
        waker : core::async::Waker { data : null, vtbl : null },
        vtbl : join_vtbl
    }
    var task = malloc(sizeof(SpawnTask<T>)) as *mut SpawnTask<T>
    var task_vtbl = malloc(sizeof(TaskVTable)) as *mut TaskVTable
    task_vtbl.poll = spawn_task_poll<T>
    task_vtbl.drop = spawn_task_drop<T>
    new(task) SpawnTask<T> {
        state : st,
        inner : handle,
        vtbl : task_vtbl
    }
    exec_enqueue_task(e, task as *mut void, task_vtbl)
    return core::async::FutureHandle<T> { frame : st as *mut void, vtbl : join_vtbl }
}

// Submit a task to the executor for a UI/event loop to drive. The executor is
// process-wide, so this is currently the same submission as `spawn`; the
// distinction is the driver. `window_run_async` (Tier 5) drives this executor
// from the native event loop on the UI thread and posts off-thread wakeups
// back to that loop, so a task submitted here is polled on the UI thread and
// its continuation after an `await` runs there too.
public func <T> spawn_local(handle : core::async::FutureHandle<T>) : core::async::FutureHandle<T> {
    return spawn<T>(handle)
}

}
