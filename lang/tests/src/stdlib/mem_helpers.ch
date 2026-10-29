// std::MaybeUninit / std::ManuallyDrop ownership tests.

var mud_drops = 0

struct MudCounter {
    var value : int

    @delete
    func delete(&mut self) {
        unsafe {
            mud_drops = mud_drops + 1
    
        }
    }
}

func mud_reset() {
        unsafe {
        mud_drops = 0

        }
    }

@test
func maybe_uninit_write_and_assume_init(env : &mut TestEnv) {
        unsafe {
        mud_reset()
        {
            var m = std::maybe_uninit<MudCounter>()
            m.write(MudCounter { value : 7 })
            var out = unsafe(m.assume_init())
            if(out.value != 7) {
                env.error("MaybeUninit::assume_init returned the wrong value")
                return
            }
        }
        // Only the value moved into `out` is dropped; the raw storage is not.
        if(mud_drops != 1) {
            env.error("MaybeUninit dropped its storage in addition to the moved value")
        }

        }
    }

@test
func maybe_uninit_unwritten_is_not_dropped(env : &mut TestEnv) {
        unsafe {
        mud_reset()
        {
            var m = std::maybe_uninit<MudCounter>()
            if(m.as_ptr() == null) {
                env.error("MaybeUninit::as_ptr returned null")
            }
        }
        if(mud_drops != 0) {
            env.error("MaybeUninit dropped an unwritten value")
        }

        }
    }

@test
func maybe_uninit_overwrite_does_not_drop_old_value(env : &mut TestEnv) {
        unsafe {
        mud_reset()
        {
            var m = std::maybe_uninit<MudCounter>()
            m.write(MudCounter { value : 1 })
            m.write(MudCounter { value : 2 })
            var out = unsafe(m.assume_init())
            if(out.value != 2) {
                env.error("MaybeUninit::write did not overwrite the previous bytes")
                return
            }
        }
        // `write` does not drop the previous value (like Rust), so only `out` drops.
        if(mud_drops != 1) {
            env.error("MaybeUninit::write dropped the overwritten value")
        }

        }
    }

@test
func manually_drop_suppresses_destructor(env : &mut TestEnv) {
        unsafe {
        mud_reset()
        {
            var d = std::manually_drop(MudCounter { value : 3 })
            if(d.as_ptr().value != 3) {
                env.error("ManuallyDrop::as_ptr returned the wrong value")
            }
        }
        if(mud_drops != 0) {
            env.error("ManuallyDrop ran the contained value's destructor")
        }

        }
    }

@test
func manually_drop_take_transfers_ownership(env : &mut TestEnv) {
        unsafe {
        mud_reset()
        {
            var d = std::manually_drop(MudCounter { value : 4 })
            var out = unsafe(d.take())
            if(out.value != 4) {
                env.error("ManuallyDrop::take returned the wrong value")
                return
            }
        }
        if(mud_drops != 1) {
            env.error("ManuallyDrop::take did not transfer ownership exactly once")
        }

        }
    }

@no_drop
struct MudLeak {
    var value : int

    @delete
    func delete(&mut self) {
        unsafe {
            mud_drops = mud_drops + 1
    
        }
    }
}

@test
func no_drop_ignores_explicit_destructor(env : &mut TestEnv) {
        unsafe {
        mud_reset()
        {
            var leak = MudLeak { value : 5 }
            if(leak.value != 5) {
                env.error("@no_drop changed the container layout or fields")
            }
        }
        if(mud_drops != 0) {
            env.error("@no_drop did not suppress the explicit destructor")
        }

        }
    }
