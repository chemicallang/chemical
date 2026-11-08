// M6 safe thread submission: the entry is a non-capturing function pointer and
// the argument must satisfy `Send + 'static`. Capturing closures remain on the
// explicit unsafe `std::concurrent.spawn` path until erased capture tracking
// lands.

func thread_worker_add(x : int) : int {
    return x + 1
}

func thread_worker_noop(x : int) {
}

func thread_worker_len(s : std::string) : int {
    return s.size() as int
}

@test
func thread_spawn_join_result(env : &mut TestEnv) {
    var h = std::thread.spawn<int, int>(thread_worker_add, 41)
    var value = h.join()
    if(value != 42) {
        env.error("safe thread did not return its joined result")
    }
}

@test
func thread_spawn_void_joins(env : &mut TestEnv) {
    var t = std::thread.spawn_void<int>(thread_worker_noop, 7)
    t.join()
}

@test
func thread_spawn_moves_send_argument(env : &mut TestEnv) {
    var text = std::string("thread")
    var h = std::thread.spawn<std::string, int>(thread_worker_len, text)
    var length = h.join()
    if(length != 6) {
        env.error("safe thread did not receive the moved string")
    }
}

@test
func thread_join_handle_drop_joins(env : &mut TestEnv) {
    var h = std::thread.spawn<int, int>(thread_worker_add, 1)
    // The JoinHandle destructor joins; leaving the scope must not leak a
    // runnable thread or double-free the result cell.
}

func thread_worker_vec_len(v : std::vector<int>) : int {
    return v.size() as int
}

@test
func thread_spawn_moves_send_container(env : &mut TestEnv) {
    var values = std::vector<int>()
    values.push(1)
    values.push(2)
    values.push(3)
    var h = std::thread.spawn<std::vector<int>, int>(thread_worker_vec_len, values)
    if(h.join() != 3) {
        env.error("safe thread did not receive the moved vector")
    }
}

func thread_increment(shared : std::Arc<std::Mutex<int> >) : int {
    var guard = shared.get().lock()
    var value = guard.get()
    *value = *value + 1
    return 0
}

@test
func thread_arc_mutex_shared_mutation(env : &mut TestEnv) {
    var shared = std::arc(std::new_mutex(0))
    var first = std::thread.spawn<std::Arc<std::Mutex<int> >, int>(
        thread_increment, shared.clone())
    var second = std::thread.spawn<std::Arc<std::Mutex<int> >, int>(
        thread_increment, shared.clone())
    first.join()
    second.join()
    var guard = shared.get().lock()
    if(*guard.get() != 2) {
        env.error("Arc<Mutex<int>> did not serialize both increments")
    }
}

func thread_worker_echo(s : std::string) : std::string {
    return s
}

@test
func thread_spawn_returns_send_value(env : &mut TestEnv) {
    var h = std::thread.spawn<std::string, std::string>(
        thread_worker_echo, std::string("echo"))
    var out = h.join()
    if(out.size() != 4) {
        env.error("safe thread result was not moved back to the caller")
    }
}
func scoped_increment(shared : &std::Arc<std::Mutex<int> >) {
    var guard = shared.get().lock()
    var value = guard.get()
    *value = *value + 1
}

@test
func thread_scoped_spawn_borrows_shared(env : &mut TestEnv) {
    var shared = std::arc(std::new_mutex(0))
    // The borrow is valid for the duration of the joining call; no 'static
    // escape is needed.
    std::thread.scoped_spawn<std::Arc<std::Mutex<int> > >(scoped_increment, &shared)
    var guard = shared.get().lock()
    if(*guard.get() != 1) {
        env.error("scoped shared thread did not run")
    }
}

func scoped_increment_mut(value : &mut int) {
    *value = *value + 1
}

@test
func thread_scoped_spawn_mut_borrows_local(env : &mut TestEnv) {
    var value = 41
    std::thread.scoped_spawn_mut<int>(scoped_increment_mut, &mut value)
    if(value != 42) {
        env.error("scoped mutable thread did not update its borrow")
    }
}

func <T> thread_needs_sync(v : &T) : int where T : Sync { return 0 }

@test
func thread_join_handle_is_sync(env : &mut TestEnv) {
    var h = std::thread.spawn<int, int>(thread_worker_add, 1)
    if(thread_needs_sync(&h) != 0) {
        env.error("JoinHandle should be Sync when T is Send")
    }
    h.join()
}
func scoped_scope_worker(scope : &mut std::thread.ScopedScope<std::Arc<std::Mutex<int> > >) {
    scope.spawn(scoped_increment)
    scope.spawn(scoped_increment)
    scope.spawn(scoped_increment)
}

@test
func thread_scoped_scope_joins_all_workers(env : &mut TestEnv) {
    var shared = std::arc(std::new_mutex(0))
    std::thread.scoped_run<std::Arc<std::Mutex<int> > >(&shared, scoped_scope_worker)
    var guard = shared.get().lock()
    if(*guard.get() != 3) {
        env.error("scoped scope did not join every worker")
    }
}

// ---- P2.6 safe handle / guard / pool / condvar coverage --------------------

func thread_pool_return_42() : int {
    return 42
}

@test
func thread_pool_submit_fn_safe(env : &mut TestEnv) {
    var pool = std::concurrent.create_pool(2u)
    var future = pool.submit_fn<int>(thread_pool_return_42)
    if(future.get() != 42) {
        env.error("safe ThreadPool.submit_fn did not return its result")
    }
}

@test
func thread_pool_submit_closure_safe(env : &mut TestEnv) {
    var pool = std::concurrent.create_pool(2u)
    var value = 41
    var future = std::concurrent.submit_closure(|value|() => value + 1, &pool)
    if(future.get() != 42) {
        env.error("safe ThreadPool submit_closure did not move its capture")
    }
}

var thread_void_fn_ran : int = 0

func thread_void_fn_mark() {
    unsafe {
        thread_void_fn_ran = thread_void_fn_ran + 1
    }
}

@test
func thread_pool_submit_void_fn_safe(env : &mut TestEnv) {
    unsafe {
        thread_void_fn_ran = 0
    }
    var pool = std::concurrent.create_pool(1u)
    pool.submit_void_fn(thread_void_fn_mark)
    var spins = 0
    while(unsafe(thread_void_fn_ran) == 0 && spins < 1000) {
        std::concurrent.sleep_ms(1u)
        spins = spins + 1
    }
    if(unsafe(thread_void_fn_ran) != 1) {
        env.error("safe ThreadPool submit_void_fn did not run its task")
    }
}

@test
func thread_spawn_closure_owned_capture(env : &mut TestEnv) {
    var value = 41
    var handle = std::thread.spawn_closure(|value|() => value + 1)
    if(handle.join() != 42) {
        env.error("safe spawn_closure did not move its capture")
    }
}

@test
func thread_mutex_try_lock_conflict(env : &mut TestEnv) {
    var m = std::new_mutex(1)
    var guard = m.lock()
    var second = m.try_lock()
    if(second is std::Option.Some) {
        env.error("try_lock must return None while another guard is live")
    }
}

@test
func thread_mutex_guard_drop_unlocks(env : &mut TestEnv) {
    var m = std::new_mutex(0)
    {
        var guard = m.lock()
        *guard.get() = 5
    }
    {
        var guard = m.lock()
        if(*guard.get() != 5) {
            env.error("MutexGuard destructor did not release the mutex")
        }
    }
}

@test
func thread_rwlock_read_write(env : &mut TestEnv) {
    var lock = std::new_rwlock(10)
    {
        var writer = lock.write()
        *writer.get() = 20
    }
    {
        var reader = lock.read()
        if(*reader.get() != 20) {
            env.error("RwLock writer did not publish its value")
        }
        var blocked = lock.try_write()
        if(blocked is std::Option.Some) {
            env.error("try_write must fail while a reader guard is live")
        }
    }
}

// A value protected by a mutex must be destroyed exactly once when the mutex
// is dropped, and never by a guard.
var thread_drop_count : int = 0

struct ThreadDropTracked {
    var value : int

    @delete
    func delete(&mut self) {
        unsafe {
            thread_drop_count = thread_drop_count + 1
        }
    }
}

@test
func thread_mutex_drops_value_once(env : &mut TestEnv) {
    unsafe {
        thread_drop_count = 0
    }
    {
        var m = std::new_mutex(ThreadDropTracked { value : 1 })
        var guard = m.lock()
        if(guard.get().value != 1) {
            env.error("Mutex did not expose the protected value")
        }
    }
    if(unsafe(thread_drop_count) != 1) {
        env.error("Mutex must drop its protected value exactly once")
    }
}

// ---- Condvar ---------------------------------------------------------------

struct ThreadSignal {
    var m : std::Mutex<int>
    var cv : std::Condvar
}

func thread_condvar_waiter(sig : std::Arc<ThreadSignal>) : int {
    var guard = sig.get().m.lock()
    while(*guard.get() == 0) {
        guard.wait(&sig.get().cv)
    }
    return *guard.get()
}

@test
func thread_condvar_notify_wakes_waiter(env : &mut TestEnv) {
    var sig = std::arc(ThreadSignal {
        m : std::new_mutex(0),
        cv : std::new_condvar()
    })
    var handle = std::thread.spawn<std::Arc<ThreadSignal>, int>(
        thread_condvar_waiter, sig.clone())
    std::concurrent.sleep_ms(20u)
    {
        var guard = sig.get().m.lock()
        *guard.get() = 9
        sig.get().cv.notify_one()
    }
    if(handle.join() != 9) {
        env.error("Condvar waiter did not observe notify_one")
    }
}

func thread_condvar_timeout_waiter(sig : std::Arc<ThreadSignal>) : bool {
    var guard = sig.get().m.lock()
    if(*guard.get() != 0) {
        return true
    }
    return guard.wait_timeout(&sig.get().cv, 5u)
}

@test
func thread_condvar_wait_timeout_returns(env : &mut TestEnv) {
    var sig = std::arc(ThreadSignal {
        m : std::new_mutex(0),
        cv : std::new_condvar()
    })
    var handle = std::thread.spawn<std::Arc<ThreadSignal>, bool>(
        thread_condvar_timeout_waiter, sig.clone())
    if(handle.join()) {
        env.error("Condvar wait_timeout must report a timeout without notify")
    }
}

// The Rust-shaped free helper infers `T` from the guard and times out.
@test
func thread_condvar_free_wait_timeout(env : &mut TestEnv) {
    var m = std::new_mutex(0)
    var cv = std::new_condvar()
    var guard = m.lock()
    if(std::condvar_wait_timeout(&cv, &mut guard, 5u)) {
        env.error("condvar_wait_timeout must return false without a notify")
    }
}


// Normal scoped work uses a generic containing closure and needs neither
// `@scoped_callback` nor an unsafe boundary; the scope joins before returning.
@test
func thread_scoped_run_closure_without_annotation(env : &mut TestEnv) {
    var value = 1
    std::thread.scoped_run_closure(|&mut value|(scope : &mut std::thread.ScopedClosureScope) => {
        std::thread.scoped_spawn_closure_in(scope, |&mut value|() => {
            *value = *value + 1
        })
    })
    if(value != 2) {
        env.error("generic scoped closure did not borrow caller storage")
    }
}
