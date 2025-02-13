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
}