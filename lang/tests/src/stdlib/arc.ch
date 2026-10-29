// std::Arc tests.

using std::Option;

var arc_drops = 0

struct ArcCounter {
    var value : int

    @delete
    func delete(&mut self) {
        unsafe {
            arc_drops = arc_drops + 1
    
        }
    }
}

func arc_reset() {
        unsafe {
        arc_drops = 0

        }
    }

@test
func arc_clone_shares_value_and_drops_once(env : &mut TestEnv) {
        unsafe {
        arc_reset()
        {
            var a = std::arc(ArcCounter { value : 11 })
            var b = a.clone()
            if(a.strong_count() != 2 || b.strong_count() != 2) {
                env.error("Arc::clone did not share the strong count")
                return
            }
            if(a.get().value != 11 || b.get().value != 11) {
                env.error("Arc clones do not share the same value")
            }
        }
        if(arc_drops != 1) {
            env.error("Arc dropped the shared value more or less than once")
        }

        }
    }

@test
func arc_weak_upgrades_while_alive(env : &mut TestEnv) {
        unsafe {
        arc_reset()
        {
            var a = std::arc(ArcCounter { value : 12 })
            var w = a.downgrade()
            if(a.weak_count() != 1) {
                env.error("Arc::downgrade did not increment the weak count")
            }
            var up = w.upgrade()
            if(up is Option.None) {
                env.error("ArcWeak::upgrade failed while a strong owner was alive")
                return
            }
            var Some(b) = up else unreachable
            if(b.get().value != 12) {
                env.error("ArcWeak::upgrade returned the wrong value")
            }
            if(a.strong_count() != 2) {
                env.error("ArcWeak::upgrade did not increment the strong count")
            }
        }
        if(arc_drops != 1) {
            env.error("ArcWeak::upgrade changed the value's drop count")
        }

        }
    }

@test
func arc_weak_expires_with_last_strong_owner(env : &mut TestEnv) {
        unsafe {
        arc_reset()
        var w : std::ArcWeak<ArcCounter>
        {
            var a = std::arc(ArcCounter { value : 13 })
            w = a.downgrade()
        }
        if(arc_drops != 1) {
            env.error("Arc did not drop the value when the last strong owner left")
        }
        if(w.strong_count() != 0) {
            env.error("ArcWeak still reports a strong owner after the value was dropped")
        }
        var up = w.upgrade()
        if(up is Option.Some) {
            env.error("ArcWeak::upgrade succeeded after the last strong owner was gone")
        }

        }
    }

@test
func arc_get_mut_requires_unique_owner(env : &mut TestEnv) {
        unsafe {
        arc_reset()
        {
            var a = std::arc(ArcCounter { value : 31 })
            {
                var second = a.clone()
                var blocked = a.get_mut()
                if(blocked is Option.Some) {
                    env.error("Arc::get_mut succeeded while another clone was alive")
                }
            }
            var allowed = a.get_mut()
            if(allowed is Option.None) {
                env.error("Arc::get_mut failed for the sole owner")
                return
            }
            var Some(value) = allowed else unreachable
            value.value = 32
            if(a.get().value != 32) {
                env.error("Arc::get_mut did not mutate the shared value")
            }
        }
        if(arc_drops != 1) {
            env.error("Arc::get_mut changed the drop count")
        }

        }
    }

@test
func arc_get_mut_blocked_by_weak_handle(env : &mut TestEnv) {
        unsafe {
        arc_reset()
        {
            var a = std::arc(ArcCounter { value : 41 })
            var w = a.downgrade()
            var blocked = a.get_mut()
            if(blocked is Option.Some) {
                env.error("Arc::get_mut succeeded while a weak handle was alive")
            }
        }
        if(arc_drops != 1) {
            env.error("Arc::get_mut weak-handle check changed the drop count")
        }

        }
    }

@test
func arc_try_unwrap_with_weak_handle(env : &mut TestEnv) {
        unsafe {
        arc_reset()
        {
            var a = std::arc(ArcCounter { value : 33 })
            var w = a.downgrade()
            var u = a.try_unwrap()
            if(u is Option.None) {
                env.error("Arc::try_unwrap failed for the sole strong owner")
                return
            }
            var Some(out) = u else unreachable
            if(out.value != 33) {
                env.error("Arc::try_unwrap returned the wrong value")
            }
            if(arc_drops != 0) {
                env.error("Arc::try_unwrap dropped the moved value early")
            }
            var up = w.upgrade()
            if(up is Option.Some) {
                env.error("ArcWeak::upgrade succeeded after Arc::try_unwrap")
            }
        }
        if(arc_drops != 1) {
            env.error("Arc::try_unwrap did not transfer ownership exactly once")
        }

        }
    }
