// std::Mutex / MutexGuard tests.

using std::Option;

@test
func mutex_lock_is_exclusive(env : &mut TestEnv) {
    var m = std::new_mutex(1)
    {
        var guard = m.lock()
        unsafe { *guard.get() = 5 }
        var busy = m.try_lock()
        if(busy is Option.Some) {
            env.error("Mutex::try_lock succeeded while the guard was live")
        }
    }
    var next = m.try_lock()
    if(next is Option.None) {
        env.error("Mutex::try_lock failed after the guard left scope")
        return
    }
    var Some(guard) = next else unsafe { unreachable }
    if(unsafe(*guard.get()) != 5) {
        env.error("Mutex value did not survive the first lock")
    }
}

@test
func mutex_guard_releases_on_scope_exit(env : &mut TestEnv) {
    var m = std::new_mutex(2)
    {
        var guard = m.lock()
        unsafe { *guard.get() = 9 }
    }
    var guard = m.lock()
    if(unsafe(*guard.get()) != 9) {
        env.error("Mutex guard did not release or mutation was lost")
    }
}

@test
func mutex_get_mut_bypasses_lock_only_with_exclusive_owner(env : &mut TestEnv) {
    var m = std::new_mutex(3)
    unsafe { *m.get_mut() = 4 }
    var guard = m.lock()
    if(unsafe(*guard.get()) != 4) {
        env.error("Mutex::get_mut did not reach the protected value")
    }
}

// A read-only critical section must be expressible without `&mut`: `get()`
// hands out `&mut T`, so two reads through one guard were two exclusive
// borrows of the same place and the second one tripped SAFE-BORROW-001. These
// tests pin both the API and that a *read* borrow cannot be used to write.

@test
func mutex_guard_get_ref_reads_the_guarded_value(env : &mut TestEnv) {
    var m = std::new_mutex(42)
    var guard = m.lock()
    if(*guard.get_ref() != 42) {
        env.error("MutexGuard::get_ref did not read the guarded value")
    }
}

@test
func mutex_guard_get_ref_allows_two_live_reads(env : &mut TestEnv) {
    // The regression: a second read while the first is still live. This is the
    // shape every "scan the value twice" critical section has.
    var m = std::new_mutex(std::string("hello"))
    var guard = m.lock()
    var first = guard.get_ref()
    var second = guard.get_ref()
    if(first.size() != second.size()) {
        env.error("two live reads of one guard disagreed")
    }
    if(!first.equals(second)) {
        env.error("two live reads of one guard saw different values")
    }
}

@test
func mutex_guard_get_ref_survives_a_write_from_get_then_a_read(env : &mut TestEnv) {
    // A write through `get()` followed by a read through `get_ref()` — the
    // exclusive borrow ends when the write statement does.
    var m = std::new_mutex(7)
    {
        var guard = m.lock()
        unsafe { *guard.get() = 11 }
        if(*guard.get_ref() != 11) {
            env.error("get_ref did not observe the value written through get")
        }
    }
}

struct GuardedSnapshot {
    var title : std::string
    var count : int
}

@test
func mutex_guard_get_ref_reads_a_struct_field(env : &mut TestEnv) {
    // Field-level reads: this is what a page snapshot does under a lock.
    var m = std::new_mutex(GuardedSnapshot { title : std::string("board"), count : 3 })
    var guard = m.lock()
    var title_size = guard.get_ref().title.size()
    var count = guard.get_ref().count
    if(title_size != 5u || count != 3) {
        env.error("get_ref did not read the guarded struct fields")
    }
}
