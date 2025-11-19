// non destructible if, switch, loop int container
struct nd_isl_int_container {
    var data : int
}

comptime const isl_int_container_sel_first = true
comptime const isl_int_container_sel_second = true

func test_if_switch_loop_value() {
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
    test("loop continue and break work as needed", () => {
        var i = 0;
        for(var j = 0; j < 10; j++) {
            for(var x = 0; x < 5; x++) {
                if(x == 3) {
                    break;
                }
                i++;
            }
            if(j == 7) {
                break;
            }
        }
        return i == 24;
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
}