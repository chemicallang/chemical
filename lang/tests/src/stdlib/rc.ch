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
