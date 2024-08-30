import "../test.ch"

interface UnInheritedInterface {
    // this method should be removed from final code_gen
    func check() : int
}

interface Calculator {

    func sum(x : int, y : int) : int

    func divide(x : int, y : int) : int;

    func multiply(x : int, y : int) : int {
        return x * y;
    }

    func multiplyP(&self) : int;

    func divideP(&self) : int;

    func avg(&self) : int;

}

impl Calculator {
    func divide(x : int, y : int) : int {
        return x / y;
    }
}

struct Point : Calculator {

    var x : int
    var y : int

    // add override keyword to indicate its overriding function present above
    @override
    func sum(x : int, y : int) : int {
        return x + y;
    }

    func call_divide(x : int, y : int) : int {
        return divide(x, y);
    }

    func call_multiply_p(&self) : int {
        return multiplyP();
    }

    func sumP(&self) : int {
        return self.x + self.y;
    }

    @override
    func multiplyP(&self) : int {
        return self.x * self.y;
    }

}

func (point : Point*) double_sum() : int {
    return 2 * (point.x + point.y);
}

struct Container {
    var point : int[2]
    var is_cool : bool
}

impl Calculator for Point {
    func divideP(&self) : int {
        return self.x / self.y;
    }
    func avg(&self) : int {
        return sumP() / 2;
    }
}

interface Summer {
    func summer_sum(&self) : int;
}

impl Summer for Point {
    func summer_sum(&self) : int {
        return x + y;
    }
}

const MyInt = 5;

func test_nodes() {
    test("global constant int", () => {
        return MyInt == 5;
    })
    test("address of works", () => {
        var x = 5;
        printf("checkout the address %p\n", &x);
        return true;
    });
    test("dereferencing works", () => {
        var x = 5;
        var y = &x;
        return *y == 5;
    });
    test("for loop", () => {
       var j = 0;
       for(var i = 0;i < 5; i++) {
           j++;
       }
       return j == 5;
    });
    test("while loop", () => {
       var j = 0;
       while(j != 5) {
            j++;
       }
       return j == 5;
    });
    test("do while loop", () => {
       var j = 0;
       do {
            j++;
       } while(j != 5);
       return j == 5;
    });
    test("switch statement", () => {
       var j = 0;
       switch(j) {
            case 0 => {
                return true;
            }
            case 1 => {
                return false;
            }
            default => {
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
            case 0 => {
                j += 1;
            }
            case 1 => {
                j += 1;
            }
            default => {
                j += 1
            }
        }
        return j == 1;
    })
    test("struct value initialization", () => {
        var p = Point {
            x : 5,
            y : 6
        };
        return p.x == 5 && p.y == 6;
    });
    test("struct functions without self ref", () => {
         var p = Point {
             x : 0,
             y : 0
         };
        return p.sum(5, 6) == 11;
    });
    test("direct calls to struct functions", () => {
        return Point.sum(6, 6) == 12;
    });
    test("struct functions with self ref", () => {
         var p = Point {
             x : 7,
             y : 6
         };
        return p.sumP() == 13;
    });
    test("test array", () => {
        var arr = {2,4,6,8,10};
        return arr[0] == 2 && arr[1] == 4 && arr[2] == 6;
    })
    test("test uninitialized array", () => {
        var arr = {}int(5);
        arr[0] = 2;
        arr[1] = 4;
        arr[2] = 6;
        return arr[0] == 2 && arr[1] == 4 && arr[2] == 6;
    })
    test("test multidimensional uninitialized array", () => {
        var arr = {}int(2, 2);
        arr[0][0] = 2;
        arr[0][1] = 4;
        arr[1][0] = 6;
        arr[1][1] = 8;
        return arr[0][0] == 2 && arr[0][1] == 4 && arr[1][0] == 6 && arr[1][1] == 8;
    })
    test("array inside a struct", () => {
        var ct = Container {
            point : {10,20},
            is_cool : true
        }
        return ct.is_cool && ct.point[0] == 10 && ct.point[1] == 20;
    })
    test("test multi dimensional array", () => {
        var arr = {{1, 2}, {3, 4}};
        return arr[0][0] == 1 && arr[0][1] == 2 && arr[1][0] == 3 && arr[1][1] == 4;
    })
    test("typealias statement", () => {
        typealias myint = int;
        var y : myint = 5;
        return y == 5;
    })
    test("can call functions declared below call", () => {
        return declared_below() == 1;
    })
    test("can call interface defined functions directly", () => {
        return Calculator.multiply(5, 5) == 25;
    })
    test("can call interface declared functions directly", () => {
        return Calculator.sum(5, 5) == 10;
    })
    test("call interface method from overridden struct value", () => {
         var p = Point {
             x : 7,
             y : 6
         };
        return p.multiply(5, 5) == 25;
    });
    test("can call implemented impl functions directly", () => {
        return Calculator.divide(5, 5) == 1;
    })
    test("can call implemented impl functions using struct value", () => {
        var p = Point {
            x : 7,
            y : 6
        };
        return p.divide(10, 5) == 2;
    })
    test("functions inside struct can call functions inherited directly", () => {
        var p = Point {
            x : 7,
            y : 6
        };
        return p.call_divide(10, 5) == 2;
    })
    test("overridden interface struct functions implemented inside struct with self ref", () => {
         var p = Point {
             x : 5,
             y : 5
         };
        return p.multiplyP() == 25;
    });
    test("overridden interface struct functions implemented inside struct with self ref", () => {
        var p = Point {
            x : 5,
            y : 5
        };
        return p.call_multiply_p() == 25;
    });
    test("overridden interface struct functions implemented using impl keyword with self ref", () => {
         var p = Point {
             x : 5,
             y : 5
         };
        return p.divideP() == 1;
    });
    test("impl block can call functions in the struct", () => {
         var p = Point {
             x : 15,
             y : 5
         };
        return p.avg() == 10;
    });
    test("supports null value - 1", () => {
        var x = 1;
        var y = &x;
        y = null;
        return y == null;
    })
    test("supports null value - 2", () => {
        var x = 1;
        var y = &x;
        return y != null;
    })
    test("can store struct in an array", () => {
        var arr = {
            Point {
                x : 3,
                y : 4
            },
            Point {
                x : 5,
                y : 6
            }
        }
        return arr[0].x == 3 && arr[0].y == 4 && arr[1].x == 5 && arr[1].y == 6;
    })
    test("extension functions work", () => {
        var p = Point {
            x : 10,
            y : 20
        }
        return p.double_sum() == 60;
    })
    test("functions of interface implemented outside struct", () => {
        var p = Point {
            x : 10,
            y : 20
        }
        return p.summer_sum() == 30;
    })
    test("single statement if statement works - 1", () => {
        if(true) return true else return false
    })
    test("single statement if statement works - 2", () => {
        if(true) return true; else return false;
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
    test("if statement can be used as a value - 1", () => {
        var val = true;
        var i = if(val) 5 else 6
        return i == 5;
    })
    test("if statement can be used as a value - 1", () => {
        var val = false;
        var i = if(val) 5 else 6
        return i == 6;
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
}

func declared_below() : int {
    return 1;
}