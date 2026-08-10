// a switch whose cases all return and that has no default case, placed as the
// last statement inside a nested scope (an if block) — the LLVM backend used to
// reject this pattern (switch codegen required a default destination), while the
// C backend handled it by falling through to the code after the switch
func switch_no_default_all_return_last_stmt(kind : int, enabled : bool) : bool {
    if(enabled) {
        switch(kind) {
            1 => {
                return true
            }
            2 => {
                return true
            }
        }
    }
    return false
}

// same pattern, but as the last statement of a loop body — the fall-through
// continues the loop iteration
func switch_no_default_all_return_loop_last_stmt(kind : int, count : int) : int {
    var total = 0
    for(var i = 0; i < count; i++) {
        switch(kind) {
            1 => {
                return 100
            }
            2 => {
                return 200
            }
        }
    }
    return total
}

func test_switch_no_default_all_return_fallthrough() {
    test("switch with all-returning cases and no default as last statement in nested scope falls through correctly", () => {
        // matching cases return true
        if(!switch_no_default_all_return_last_stmt(1, true)) {
            return false
        }
        if(!switch_no_default_all_return_last_stmt(2, true)) {
            return false
        }
        // a non-matching value falls through to the return after the if block
        if(switch_no_default_all_return_last_stmt(3, true)) {
            return false
        }
        // disabled path skips the switch entirely
        if(switch_no_default_all_return_last_stmt(1, false)) {
            return false
        }
        return true
    })
    test("switch with all-returning cases and no default as last statement in loop body falls through correctly", () => {
        // matching case returns on the first iteration
        if(switch_no_default_all_return_loop_last_stmt(1, 3) != 100) {
            return false
        }
        if(switch_no_default_all_return_loop_last_stmt(2, 3) != 200) {
            return false
        }
        // a non-matching value falls through the loop body every iteration
        if(switch_no_default_all_return_loop_last_stmt(3, 3) != 0) {
            return false
        }
        // zero iterations never enter the loop
        if(switch_no_default_all_return_loop_last_stmt(1, 0) != 0) {
            return false
        }
        return true
    })
}

func <T> generic_default_in_switch_works(value : T) : int {
    switch(value) {
        0 => {
            return 0
        }
        1 => {
            return 10;
        }
        2 => {
            return 20;
        }
        default => {
            return 30;
        }
    }
}

func test_switch_statement() {
    test("switch statement", () => {
       var j = 0;
       switch(j) {
            0 => {
                return true;
            }
            1 => {
                return false;
            }
            default => {
                return false;
            }
       }
    });
    test("switch statement with multiple cases - 1", () => {
       var j = 0;
       switch(j) {
            0 , 1 => {
                return true;
            }
            default => {
                return false;
            }
       }
    });
    test("switch statement with multiple cases - 2", () => {
       var j = 1;
       switch(j) {
            0 , 1 => {
                return true;
            }
            default => {
                return false;
            }
       }
    });
    test("switch statement with multiple cases - 3", () => {
       var j = 3;
       switch(j) {
            0 , 1 => {
                return false;
            }
            default => {
                return true;
            }
       }
    });
    test("switch statement with multiple cases with default - 1", () => {
       var j = 0;
       switch(j) {
            0 , 1 , default => {
                return true;
            }
            3 => {
                return false;
            }
       }
    });
    test("switch statement with multiple cases with default - 2", () => {
       var j = 3;
       switch(j) {
            0 , 1 , default => {
                return false;
            }
            3 => {
                return true;
            }
       }
    });
    test("switch statement with multiple cases with default - 3", () => {
       var j = 4;
       switch(j) {
            0 , 1 , default => {
                return true;
            }
            3 => {
                return false;
            }
       }
    });
    test("switch statement case keyword is optional", () => {
        var j = 0;
        switch(j) {
            0 => {
                return true;
            }
            1 => {
                return false;
            }
            default => {
                return false;
            }
        }
    })
    test("switch doesn't fallthrough by default", () => {
        var j = 0;
        switch(j) {
            0 => {
                j += 1;
            }
            1 => {
                j += 1;
            }
            default => {
                j += 1
            }
        }
        return j == 1;
    })
    test("switch statement can have single statement instead of block - 1", () => {
        var i = 0;
        switch(i) {
            0 => return true
            default => return false
        }
    })
    test("switch statement can have single statement instead of block - 2", () => {
        var i = 0;
        switch(i) {
            0 => return true;
            default => return false;
        }
    })
    test("switch statement can be used as a value - 1", () => {
        var val = 45;
        var i = switch(val) {
             45 => 5
             default => 6
        }
        return i == 5;
    })
    test("switch statement can be used as a value - 2", () => {
        var val = 50;
        var i = switch(val) {
             45 => 5
             default => 6
        }
        return i == 6;
    })
    test("switch with default works in generics - 1", () => {
        var value = 0
        return generic_default_in_switch_works(value) == 0
    })
    test("switch with default works in generics - 2", () => {
        var value = 1
        return generic_default_in_switch_works(value) == 10
    })
    test("switch with default works in generics - 3", () => {
        var value = 2
        return generic_default_in_switch_works(value) == 20
    })
    test("switch with default works in generics - 4", () => {
        var value = 8
        return generic_default_in_switch_works(value) == 30
    })
}
