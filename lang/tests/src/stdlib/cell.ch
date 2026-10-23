// std::UnsafeCell / std::Cell tests.

@test
func unsafe_cell_interior_mutation(env : &mut TestEnv) {
    var c = std::unsafe_cell(1)
    var p = c.get()
    unsafe { p[0] = 2 }
    if(*c.get_ref() != 2) {
        env.error("UnsafeCell write through shared reference was not visible")
    }
    var m = c.get_mut()
    *m = 3
    if(c.into_inner() != 3) {
        env.error("UnsafeCell get_mut/into_inner mismatch")
    }
}

@test
func cell_get_set(env : &mut TestEnv) {
    var c = std::cell(1)
    if(c.get() != 1) {
        env.error("Cell initial value mismatch")
    }
    c.set(7)
    if(c.get() != 7) {
        env.error("Cell set/get mismatch")
    }
    var m = c.get_mut()
    *m = 9
    if(c.get() != 9) {
        env.error("Cell get_mut mismatch")
    }
}
