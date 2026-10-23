using std::Option;

// std::Rc tests.

var rc_drops = 0

struct RcCounter {
    var value : int

    @delete
    func delete(&mut self) {
        rc_drops = rc_drops + 1
    }
}

func rc_reset() {
    rc_drops = 0
}

@test
func rc_clone_shares_value_and_drops_once(env : &mut TestEnv) {
    rc_reset()
    {
        var a = std::rc(RcCounter { value : 11 })
        var b = a.clone()
        if(a.strong_count() != 2) {
            env.error("Rc::clone did not increment the strong count")
            return
        }
        if(b.strong_count() != 2) {
            env.error("Rc clones do not share the strong count")
            return
        }
        if(a.get().value != 11 || b.get().value != 11) {
            env.error("Rc clones do not share the same value")
        }
    }
    if(rc_drops != 1) {
        env.error("Rc dropped the shared value more or less than once")
    }
}

@test
func rc_survives_original_owner(env : &mut TestEnv) {
    rc_reset()
    var b : std::Rc<RcCounter>
    {
        var a = std::rc(RcCounter { value : 12 })
        b = a.clone()
        if(b.strong_count() != 2) {
            env.error("Rc::clone count was wrong before the owner left scope")
        }
    }
    if(b.strong_count() != 1) {
        env.error("Rc did not keep the value alive after the original owner left scope")
    }
    if(b.get().value != 12) {
        env.error("Rc value was lost after the original owner left scope")
    }
    if(rc_drops != 0) {
        env.error("Rc dropped the value while a clone was still alive")
    }
    // `b` is dropped at function exit; this verifies the count reached zero.
}

@test
func rc_drops_after_last_owner(env : &mut TestEnv) {
    rc_reset()
    {
        var a = std::rc(RcCounter { value : 13 })
        var b = a.clone()
        var c = b.clone()
        if(c.strong_count() != 3) {
            env.error("Rc::clone count was wrong for three owners")
        }
    }
    if(rc_drops != 1) {
        env.error("Rc did not drop the value exactly once after the last owner")
    }
}

@test
func rc_try_unwrap_only_when_unique(env : &mut TestEnv) {
    rc_reset()
    {
        var a = std::rc(RcCounter { value : 21 })
        {
            var second = a.clone()
            var not_unique = a.try_unwrap()
            if(not_unique is Option.Some) {
                env.error("Rc::try_unwrap succeeded while another clone was alive")
            }
        }
        // The clone left scope above, so `a` is the sole owner.
        var unique = a.try_unwrap()
        if(unique is Option.None) {
            env.error("Rc::try_unwrap failed for the sole owner")
            return
        }
        var Some(out) = unique else unreachable
        if(out.value != 21) {
            env.error("Rc::try_unwrap returned the wrong value")
        }
    }
    if(rc_drops != 1) {
        env.error("Rc::try_unwrap did not transfer ownership exactly once")
    }
}

@test
func rc_get_mut_requires_unique_owner(env : &mut TestEnv) {
    rc_reset()
    {
        var a = std::rc(RcCounter { value : 31 })
        {
            var second = a.clone()
            var blocked = a.get_mut()
            if(blocked is Option.Some) {
                env.error("Rc::get_mut succeeded while another clone was alive")
            }
        }
        var allowed = a.get_mut()
        if(allowed is Option.None) {
            env.error("Rc::get_mut failed for the sole owner")
            return
        }
        var Some(value) = allowed else unreachable
        value.value = 32
        if(a.get().value != 32) {
            env.error("Rc::get_mut did not mutate the shared value")
        }
    }
    if(rc_drops != 1) {
        env.error("Rc::get_mut changed the drop count")
    }
}

@test
func rc_weak_upgrades_while_alive(env : &mut TestEnv) {
    rc_reset()
    {
        var a = std::rc(RcCounter { value : 41 })
        var w = a.downgrade()
        if(a.weak_count() != 1) {
            env.error("Rc::downgrade did not increment the weak count")
        }
        var up = w.upgrade()
        if(up is Option.None) {
            env.error("Weak::upgrade failed while a strong owner was alive")
            return
        }
        var Some(b) = up else unreachable
        if(b.get().value != 41) {
            env.error("Weak::upgrade returned the wrong value")
        }
        if(a.strong_count() != 2) {
            env.error("Weak::upgrade did not increment the strong count")
        }
    }
    if(rc_drops != 1) {
        env.error("Weak::upgrade changed the value's drop count")
    }
}

@test
func rc_weak_expires_with_last_strong_owner(env : &mut TestEnv) {
    rc_reset()
    var w : std::Weak<RcCounter>
    {
        var a = std::rc(RcCounter { value : 42 })
        w = a.downgrade()
    }
    if(rc_drops != 1) {
        env.error("Rc did not drop the value when the last strong owner left")
    }
    if(w.strong_count() != 0) {
        env.error("Weak still reports a strong owner after the value was dropped")
    }
    if(w.weak_count() != 1) {
        env.error("Weak lost its own count while still alive")
    }
    var up = w.upgrade()
    if(up is Option.Some) {
        env.error("Weak::upgrade succeeded after the last strong owner was gone")
    }
}

@test
func rc_weak_keeps_allocation_for_try_unwrap(env : &mut TestEnv) {
    rc_reset()
    {
        var a = std::rc(RcCounter { value : 43 })
        var w = a.downgrade()
        var u = a.try_unwrap()
        if(u is Option.None) {
            env.error("Rc::try_unwrap failed for the sole strong owner with a weak handle")
            return
        }
        var Some(out) = u else unreachable
        if(out.value != 43) {
            env.error("Rc::try_unwrap returned the wrong value with a weak handle")
        }
        if(rc_drops != 0) {
            env.error("Rc::try_unwrap dropped the moved value before it left scope")
        }
        var up = w.upgrade()
        if(up is Option.Some) {
            env.error("Weak::upgrade succeeded after Rc::try_unwrap")
        }
    }
    if(rc_drops != 1) {
        env.error("Rc::try_unwrap with a weak handle did not transfer ownership once")
    }
}
