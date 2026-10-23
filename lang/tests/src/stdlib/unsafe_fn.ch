// `unsafe func` parsing and call-site enforcement.

unsafe func unsafe_add(a : int, b : int) : int {
    return a + b
}

struct UnsafeReader {
    var value : int

    unsafe func read(&self) : int {
        return self.value
    }
}

@test
func unsafe_function_can_be_called_in_unsafe_expr(env : &mut TestEnv) {
    var sum = unsafe(unsafe_add(2, 3))
    if(sum != 5) {
        env.error("unsafe function returned the wrong value")
    }
}

@test
func unsafe_function_can_be_called_in_unsafe_block(env : &mut TestEnv) {
    var sum = 0
    unsafe {
        sum = unsafe_add(4, 5)
    }
    if(sum != 9) {
        env.error("unsafe function in a block returned the wrong value")
    }
}

@test
func unsafe_method_can_be_called_in_unsafe_expr(env : &mut TestEnv) {
    var reader = UnsafeReader { value : 42 }
    var value = unsafe(reader.read())
    if(value != 42) {
        env.error("unsafe method returned the wrong value")
    }
}
