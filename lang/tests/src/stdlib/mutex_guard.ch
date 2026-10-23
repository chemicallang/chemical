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
    var Some(guard) = next else unreachable
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
