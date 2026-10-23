// Container Clone tests: Option<T> and vector<T> with `impl<T : Clone>`.

using std::Option;
using std::vector;
using std::string;
using core::ops::Clone;

var container_clone_drops = 0

struct ContainerCloneCounter {
    var value : int

    @delete
    func delete(&mut self) {
        container_clone_drops = container_clone_drops + 1
    }
}

impl Clone for ContainerCloneCounter {
    func clone(&self) : ContainerCloneCounter {
        return ContainerCloneCounter { value : self.value }
    }
}

func container_clone_reset() {
    container_clone_drops = 0
}

@test
func option_clone_roundtrip(env : &mut TestEnv) {
    var some = Option.Some<int>(7)
    var some_copy = some.clone()
    if(some_copy is Option.Some) {
        var Some(value) = some_copy else unreachable
        if(value != 7) {
            env.error("Option<int>::clone copied the wrong value")
        }
    } else {
        env.error("Option<int>::clone changed Some into None")
    }

    var none = Option.None<int>()
    var none_copy = none.clone()
    if(none_copy is Option.Some) {
        env.error("Option<int>::clone changed None into Some")
    }
}

@test
func option_clone_user_type_drops_once(env : &mut TestEnv) {
    container_clone_reset()
    {
        var original = Option.Some<ContainerCloneCounter>(ContainerCloneCounter { value : 9 })
        var copy = original.clone()
        if(copy is Option.Some) {
            var Some(value) = copy else unreachable
            if(value.value != 9) {
                env.error("Option<Clone>::clone copied the wrong value")
            }
        } else {
            env.error("Option<Clone>::clone changed Some into None")
        }
    }
    if(container_clone_drops != 2) {
        env.error("Option<T>::clone did not drop exactly the original and the copy")
    }
}

@test
func vector_clone_roundtrip(env : &mut TestEnv) {
    var source = vector<int>()
    source.push(1)
    source.push(2)
    source.push(3)
    var copy = source.clone()
    if(copy.size() != 3) {
        env.error("vector<int>::clone returned the wrong size")
        return
    }
    if(copy.get(0) != 1 || copy.get(1) != 2 || copy.get(2) != 3) {
        env.error("vector<int>::clone copied the wrong elements")
    }
    if(source.size() != 3 || source.get(0) != 1) {
        env.error("vector<int>::clone mutated the source")
    }
}

@test
func vector_clone_empty(env : &mut TestEnv) {
    var source = vector<int>()
    var copy = source.clone()
    if(copy.size() != 0) {
        env.error("vector<int>::clone on an empty vector produced elements")
    }
}

@test
func vector_clone_string_is_deep(env : &mut TestEnv) {
    var source = vector<string>()
    source.push(string("hello"))
    var copy = source.clone()
    if(copy.size() != 1 || copy.get_ptr(0).size() != 5) {
        env.error("vector<string>::clone copied the wrong value")
        return
    }
    copy.get_ptr(0).append('!')
    if(source.get_ptr(0).size() != 5) {
        env.error("vector<string>::clone shared the source string")
    }
    if(copy.get_ptr(0).size() != 6) {
        env.error("vector<string>::clone did not produce a mutable copy")
    }
}

@test
func vector_clone_user_type_drops_once(env : &mut TestEnv) {
    container_clone_reset()
    {
        var source = vector<ContainerCloneCounter>()
        source.push(ContainerCloneCounter { value : 1 })
        source.push(ContainerCloneCounter { value : 2 })
        var copy = source.clone()
        if(copy.size() != 2) {
            env.error("vector<Clone>::clone returned the wrong size")
            return
        }
        if(copy.get_ptr(0).value != 1 || copy.get_ptr(1).value != 2) {
            env.error("vector<Clone>::clone copied the wrong values")
        }
    }
    if(container_clone_drops != 4) {
        env.error("vector<T>::clone did not drop exactly two originals and two copies")
    }
}
