// std::Arc tests.

using std::Option;

var arc_drops = 0

struct ArcCounter {
    var value : int

    @delete
    func delete(&mut self) {
        arc_drops = arc_drops + 1
    }
}

func arc_reset() {
    arc_drops = 0
}

@test
func arc_clone_shares_value_and_drops_once(env : &mut TestEnv) {
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

@test
func arc_weak_upgrades_while_alive(env : &mut TestEnv) {
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

@test
func arc_weak_expires_with_last_strong_owner(env : &mut TestEnv) {
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
