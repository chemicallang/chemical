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
