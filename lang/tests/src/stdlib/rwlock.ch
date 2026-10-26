// std::RwLock / read-write guard tests.

using std::Option;

@test
func rwlock_readers_share(env : &mut TestEnv) {
    var rw = std::new_rwlock(1)
    {
        var first = rw.read()
        var second = rw.read()
        if(unsafe(*first.get()) != 1 || unsafe(*second.get()) != 1) {
            env.error("RwLock readers did not observe the protected value")
        }
        var blocked = rw.try_write()
        if(blocked is Option.Some) {
            env.error("RwLock::try_write succeeded while readers were live")
        }
    }
    var write = rw.try_write()
    if(write is Option.None) {
        env.error("RwLock::try_write failed after the readers left scope")
        return
    }
    var Some(guard) = write else unreachable
    unsafe { *guard.get() = 2 }
}

@test
func rwlock_writer_is_exclusive(env : &mut TestEnv) {
    var rw = std::new_rwlock(3)
    {
        var write = rw.write()
        unsafe { *write.get() = 9 }
        var read = rw.try_read()
        if(read is Option.Some) {
            env.error("RwLock::try_read succeeded while the writer was live")
        }
        var second = rw.try_write()
        if(second is Option.Some) {
            env.error("RwLock::try_write allowed a second writer")
        }
    }
    var read = rw.read()
    if(unsafe(*read.get()) != 9) {
        env.error("RwLock writer mutation was lost")
    }
}

@test
func rwlock_get_mut_reaches_value(env : &mut TestEnv) {
    var rw = std::new_rwlock(4)
    unsafe { *rw.get_mut() = 11 }
    var read = rw.read()
    if(unsafe(*read.get()) != 11) {
        env.error("RwLock::get_mut did not reach the protected value")
    }
}
