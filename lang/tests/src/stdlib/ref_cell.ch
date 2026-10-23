// std::RefCell tests.

using std::Option;

@test
func ref_cell_allows_multiple_shared_borrows(env : &mut TestEnv) {
    var cell = std::ref_cell(1)
    {
        var first = cell.try_borrow()
        if(first is Option.None) {
            env.error("RefCell::try_borrow failed on an unborrowed cell")
            return
        }
        var Some(a) = first else unreachable
        if(*a.get() != 1) {
            env.error("RefCell shared borrow returned the wrong value")
        }
        var second = cell.try_borrow()
        if(second is Option.None) {
            env.error("RefCell rejected a second shared borrow")
        }
        if(cell.borrow_count() != 2) {
            env.error("RefCell did not count both shared borrows")
        }
        var blocked = cell.try_borrow_mut()
        if(blocked is Option.Some) {
            env.error("RefCell allowed a mutable borrow while shared borrows were live")
        }
    }
    if(cell.borrow_count() != 0) {
        env.error("RefCell did not release shared borrows at scope exit")
    }
}

@test
func ref_cell_mutable_borrow_is_exclusive(env : &mut TestEnv) {
    var cell = std::ref_cell(2)
    {
        var borrowed = cell.try_borrow_mut()
        if(borrowed is Option.None) {
            env.error("RefCell::try_borrow_mut failed on an unborrowed cell")
            return
        }
        var Some(m) = borrowed else unreachable
        *m.get_mut() = 7
        var blocked = cell.try_borrow()
        if(blocked is Option.Some) {
            env.error("RefCell allowed a shared borrow while mutable borrow was live")
        }
    }
    if(cell.borrow_count() != 0) {
        env.error("RefCell did not release a mutable borrow at scope exit")
    }
    var view = cell.try_borrow()
    if(view is Option.None) {
        env.error("RefCell remained borrowed after the mutable guard left scope")
        return
    }
    var Some(v) = view else unreachable
    if(*v.get() != 7) {
        env.error("RefCell mutation through RefMut was not visible")
    }
}

@test
func ref_cell_destructor_releases_stale_flags(env : &mut TestEnv) {
    var cell = std::ref_cell(3)
    {
        var guard = cell.try_borrow_mut()
        if(guard is Option.None) {
            env.error("RefCell::try_borrow_mut failed")
        }
    }
    if(cell.borrow_count() != 0) {
        env.error("RefCell borrow state leaked after RefMut destruction")
    }
    var guard = cell.try_borrow()
    if(guard is Option.None) {
        env.error("RefCell could not be borrowed after stale mutable flag")
    }
}
