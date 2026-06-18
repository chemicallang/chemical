// non destructible if, switch, loop int container
struct nd_isl_int_container {
    var data : int
}

comptime const isl_int_container_sel_first = true
comptime const isl_int_container_sel_second = true

@direct_init
struct if_val_view {
    var d : int
    @make
    func make() {
        return if_val_view { d : 10 }
    }
}

func if_val_view_param_conditional_if(cond : bool, v : if_val_view) : if_val_view {
    var second = if(cond) v else if_val_view()
    return second;
}

struct if_val_fake_string_view_33 {
    func to_string(&self, ptr : *mut int) : if_val_fake_string_33 {
        return { d_count : ptr }
    }
    func empty(&self) : bool {
        return false;
    }
}

struct if_val_fake_string_33 {
    var d_count : *mut int
    func c_str(&self) : *char {
        return null;
    }
    @delete
    func delete(&mut self) {
        *d_count = *d_count + 1
    }
}

func if_val_fake_atoi(a : *char) : int { return 0; }

func test_if_switch_loop_value() {
    test("constructible values inside if conditionals work - 1", () => {
        var x = if_val_view_param_conditional_if(true, if_val_view{ d : 98 })
        return x.d == 98
    })
    test("constructible values inside if conditionals work - 2", () => {
        var x = if_val_view_param_conditional_if(false, if_val_view{ d : 98 })
        return x.d == 10
    })
    test("different integer types in if value works", () => {
        var first = 234324u64
        var condition = true
        condition = false
        condition = true
        var i = if(condition) first else 123 // 123 is i32
        return i == first
    })
    test("different integer types in if value works - 2", () => {
        var first = 2343u64
        var condition = true
        condition = false
        condition = true
        var i = if (first < 1u) 1u else first // 123 is i32
        return i == first
    })
    test("nested if in if value statements - 1", () => {
        var i = 2;
        var j = if(i > 0) if(i < 2) 10 else 20 else 30
        return j == 20
    })
    test("nested if in if value statements - 2", () => {
        var i = 1;
        var j = if(i > 0) if(i < 2) 10 else 20 else 30
        return j == 10
    })
    test("nested if in if value statements - 3", () => {
        var i = 0;
        var j = if(i > 0) if(i < 2) 10 else 20 else 30
        return j == 30
    })
    test("nested if in if braced value statements - 1", () => {
        var i = 2;
        var j = if(i > 0) { if(i < 2) 10 else 20 } else 30
        return j == 20
    })
    test("nested if in if braced value statements - 2", () => {
        var i = 1;
        var j = if(i > 0) { if(i < 2) 10 else 20 } else 30
        return j == 10
    })
    test("nested if in if braced value statements - 3", () => {
        var i = 0;
        var j = if(i > 0) { if(i < 2) 10 else 20 } else 30
        return j == 30
    })
    test("nested switch in if value statements - 1", () => {
        var i = 2;
        var j = if(i > 0) switch(i) {
            1 => 10
            2 => 20
            default => 40
        } else 0
        return j == 20
    })
    test("nested switch in if value statements - 2", () => {
        var i = 0;
        var j = if(i > 0) switch(i) {
            1 => 10
            2 => 20
            default => 40
        } else 50
        return j == 50
    })
    test("nested switch in if value statements - 3", () => {
        var i = 1;
        var j = if(i > 0) switch(i) {
            1 => 10
            2 => 20
            default => 40
        } else 0
        return j == 10
    })
    test("nested switch in if value statements - 4", () => {
        var i = 5;
        var j = if(i > 0) switch(i) {
            1 => 10
            2 => 20
            default => 40
        } else 0
        return j == 40
    })
    test("nested switch in braced if value statements - 1", () => {
        var i = 2;
        var j = if(i > 0) switch(i) {
            1 => 10
            2 => 20
            default => 40
        } else 0
        return j == 20
    })
    test("nested switch in braced if value statements - 2", () => {
        var i = 0;
        var j = if(i > 0) {
            switch(i) {
                1 => 10
                2 => 20
                default => 40
            }
        } else 50
        return j == 50
    })
    test("nested switch in braced if value statements - 3", () => {
        var i = 1;
        var j = if(i > 0) {
            switch(i) {
                1 => 10
                2 => 20
                default => 40
            }
        } else 0
        return j == 10
    })
    test("nested switch in braced if value statements - 4", () => {
        var i = 5;
        var j = if(i > 0) {
            switch(i) {
                1 => 10
                2 => 20
                default => 40
            }
        } else 0
        return j == 40
    })
    test("nested switch in braced if value statement with additional statement", () => {
        var i = 5;
        var j = if(i > 0) {
            i = 2;
            switch(i) {
                1 => 10
                2 => 20
                default => 40
            }
        } else 0
        return j == 20
    })
    test("nested if in switch value statements - 1", () => {
        var i = 2;
        var tens = true;
        var j = switch(i) {
            1 => if(tens) 10 else 100
            2 => if(tens) 20 else 200
            default => 0
        }
        return j == 20
    })
    test("nested if in switch value statements - 2", () => {
        var i = 1;
        var tens = false;
        var j = switch(i) {
            1 => if(tens) 10 else 100
            2 => if(tens) 20 else 200
            default => 0
        }
        return j == 100
    })
    test("nested if in switch value statements - 3", () => {
        var i = 3;
        var tens = false;
        var j = switch(i) {
            1 => if(tens) 10 else 100
            2 => if(tens) 20 else 200
            default => 0
        }
        return j == 0
    })
    test("nested if in switch value statements - 4", () => {
        var i = 2;
        var tens = false;
        var j = switch(i) {
            1 => if(tens) 10 else 100
            2 => if(tens) 20 else 200
            default => 0
        }
        return j == 200
    })
    test("nested if in braced switch value statements - 1", () => {
        var i = 2;
        var tens = true;
        var j = switch(i) {
            1 => { if(tens) 10 else 100 }
            2 => { if(tens) 20 else 200 }
            default => 0
        }
        return j == 20
    })
    test("nested if in braced switch value statements - 2", () => {
        var i = 1;
        var tens = false;
        var j = switch(i) {
            1 => { if(tens) 10 else 100 }
            2 => { if(tens) 20 else 200 }
            default => 0
        }
        return j == 100
    })
    test("nested if in braced switch value statements - 3", () => {
        var i = 3;
        var tens = false;
        var j = switch(i) {
            1 => { if(tens) 10 else 100 }
            2 => { if(tens) 20 else 200 }
            default => 0
        }
        return j == 0
    })
    test("nested if in braced switch value statements - 4", () => {
        var i = 2;
        var tens = false;
        var j = switch(i) {
            1 => { if(tens) 10 else 100 }
            2 => { if(tens) 20 else 200 }
            default => 0
        }
        return j == 200
    })
    test("nested if in braced switch value statement with additional statement", () => {
        var i = 2;
        var tens = false;
        var j = switch(i) {
            1 => { if(tens) 10 else 100 }
            2 => {
                i = 90;
                if(tens) 20 else 200
            }
            default => 0
        }
        return i == 90 && j == 200
    })
    test("loop block works", () => {
        var i = 0;
        loop {
            if(i == 5) {
                break;
            }
            i++;
        }
        return i == 5;
    })
    test("loop block works as a value", () => {
        var i = 0;
        var j = loop {
            if(i == 5) {
                break i;
            }
            i++;
        }
        return j == i && i == 5;
    })
    test("if value can be used as a condition to select struct based values - 1", () => {
        var first = nd_isl_int_container { data : 10 }
        var second = nd_isl_int_container { data : 20 }
        var s = if(isl_int_container_sel_first) first else second
        return s.data == 10
    })
    test("if value can be used as a condition to select struct based values - 2", () => {
        var first = nd_isl_int_container { data : 10 }
        var second = nd_isl_int_container { data : 20 }
        var s = if(!isl_int_container_sel_first) first else second
        return s.data == 20
    })
    test("nested if value can be used as a condition to select struct based values - 1", () => {
        var first = nd_isl_int_container { data : 10 }
        var second = nd_isl_int_container { data : 20 }
        var s = if(isl_int_container_sel_first) if(isl_int_container_sel_second) second else first else if(isl_int_container_sel_second) second else first
        return s.data == 20
    })
    test("nested if value can be used as a condition to select struct based values - 2", () => {
        var first = nd_isl_int_container { data : 10 }
        var second = nd_isl_int_container { data : 20 }
        var s = if(!isl_int_container_sel_first) if(isl_int_container_sel_second) first else second else if(!isl_int_container_sel_second) first else second
        return s.data == 20
    })
    test("nested if value can be used as a condition to select struct based values - 3", () => {
        var first = nd_isl_int_container { data : 10 }
        var second = nd_isl_int_container { data : 20 }
        var s = if(isl_int_container_sel_first) if(!isl_int_container_sel_second) second else first else if(!isl_int_container_sel_second) second else first
        return s.data == 10
    })
    test("nested if value can be used as a condition to select struct based values - 4", () => {
        var first = nd_isl_int_container { data : 10 }
        var second = nd_isl_int_container { data : 20 }
        var s = if(!isl_int_container_sel_first) if(!isl_int_container_sel_second) first else second else if(isl_int_container_sel_second) first else second
        return s.data == 10
    })
    test("if value in switch works with struct based values - 1", () => {
        var first = nd_isl_int_container { data : 10 }
        var second = nd_isl_int_container { data : 100 }
        var i = 1;
        var tens = false;
        var j = switch(i) {
            1 => { if(tens) first else second }
            2 => { if(tens) first else second }
            default => nd_isl_int_container { data : 0 }
        }
        return j.data == 100
    })
    test("if value in switch works with struct based values - 2", () => {
        var first = nd_isl_int_container { data : 10 }
        var second = nd_isl_int_container { data : 100 }
        var i = 1;
        var tens = true;
        var j = switch(i) {
            1 => { if(tens) first else second }
            2 => { if(tens) first else second }
            default => nd_isl_int_container { data : 0 }
        }
        return j.data == 10
    })
    test("if value in switch works with struct based values - 3", () => {
        var first = nd_isl_int_container { data : 10 }
        var second = nd_isl_int_container { data : 100 }
        var i = 2;
        var tens = false;
        var j = switch(i) {
            1 => { if(tens) first else second }
            2 => { if(tens) first else second }
            default => nd_isl_int_container { data : 0 }
        }
        return j.data == 100
    })
    test("if value in switch works with struct based values - 4", () => {
        var first = nd_isl_int_container { data : 10 }
        var second = nd_isl_int_container { data : 100 }
        var i = 2;
        var tens = true;
        var j = switch(i) {
            1 => { if(tens) first else second }
            2 => { if(tens) first else second }
            default => nd_isl_int_container { data : 0 }
        }
        return j.data == 10
    })
    test("correctly destructs struct in if value block", () => {
        // this syntax used to fail because destruction took place inside the '({' c block
        // it generated invalid c
        var limit_str = if_val_fake_string_view_33();
        var converted_d_count : int = 0
        var limit = if(limit_str.empty()) 10 else if_val_fake_atoi(limit_str.to_string(&raw mut converted_d_count).c_str())
        if(converted_d_count != 1) return false;
        var final_d_count : int = 0
        if(true) {
            var s = if_val_fake_string_33 { d_count : &raw mut final_d_count }
        }
        if(final_d_count != 1) return false;
        return true;
    })
    test("method chain without &self compiles in c translation", () => {
        var limit_str = chain_no_self_view();
        var limit = if(limit_str.empty()) 10 else chain_no_self_atoi(limit_str.to_string().c_str())
        return true;
    })
}

// helpers for testing method chain without &self on c translation backends
struct chain_no_self_view {
    func to_string() : chain_no_self_str {
        return {}
    }
    func empty() : bool {
        return true;
    }
}
struct chain_no_self_str {
    func c_str() : *char {
        return null;
    }
    @delete
    func delete(&mut self) {
    }
}
func chain_no_self_atoi(a : *char) : int { return 0; }