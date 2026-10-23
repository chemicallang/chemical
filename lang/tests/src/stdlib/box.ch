// std::Box tests.

var box_drops = 0

struct BoxCounter {
    var value : int

    @delete
    func delete(&mut self) {
        box_drops = box_drops + 1
    }
}

func box_reset() {
    box_drops = 0
}

func <T> box_requires_send(value : T) : T where T: Send {
    return value
}

@test
func box_drops_inner_value_once(env : &mut TestEnv) {
    box_reset()
    {
        var b = std::box(BoxCounter { value : 9 })
        if(b.get().value != 9) {
            env.error("Box::get returned the wrong value")
            return
        }
    }
    if(box_drops != 1) {
        env.error("Box did not drop its inner value exactly once")
    }
}

@test
func box_get_mut_writes_inner_value(env : &mut TestEnv) {
    box_reset()
    {
        var b = std::box(BoxCounter { value : 1 })
        b.get_mut().value = 2
        if(b.get().value != 2) {
            env.error("Box::get_mut did not reach the heap value")
        }
    }
    if(box_drops != 1) {
        env.error("Box dropped the inner value more than once after get_mut")
    }
}

@test
func box_moves_without_copying(env : &mut TestEnv) {
    box_reset()
    {
        var first = std::box(BoxCounter { value : 5 })
        var second = first
        if(second.get().value != 5) {
            env.error("moving a Box changed its value")
        }
    }
    if(box_drops != 1) {
        env.error("moving a Box did not preserve single ownership")
    }
}

@test
func box_owns_string_value(env : &mut TestEnv) {
    {
        var b = std::box(std::string("boxed"))
        if(b.get().size() != 5) {
            env.error("Box<string> did not own the string correctly")
            return
        }
        b.get_mut().append('!')
        if(b.get().size() != 6) {
            env.error("Box<string>::get_mut did not mutate the boxed string")
        }
    }
}

@test
func box_is_send_when_inner_is_send(env : &mut TestEnv) {
    var b = std::box(12)
    var v = box_requires_send(b)
    if(*v.get() != 12) {
        env.error("Box<T> lost its value at a Send bound")
    }
}

@test
func box_send_is_recursive(env : &mut TestEnv) {
    var b = std::box(std::box(21))
    var v = box_requires_send(b)
    if(*v.get().get() != 21) {
        env.error("nested Box<T> lost Send or its value")
    }
}

@test
func box_into_inner_transfers_ownership(env : &mut TestEnv) {
    box_reset()
    {
        var b = std::box(BoxCounter { value : 14 })
        var out = b.into_inner()
        if(out.value != 14) {
            env.error("Box::into_inner returned the wrong value")
            return
        }
    }
    if(box_drops != 1) {
        env.error("Box::into_inner did not transfer ownership exactly once")
    }
}
