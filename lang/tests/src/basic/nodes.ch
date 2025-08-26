
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

    func dividePDefault(&self) : int {
        return divideP();
    }

    func dividePOverride(&self) : int {
        return divideP();
    }

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

    @override
    func dividePOverride(&self) : int {
        return divideP() + 2;
    }

}

func give_point_ptr_sum(p : *Point) : int {
    return p.x + p.y;
}

func (point : &Point) double_sum() : int {
    return 2 * (point.x + point.y);
}

struct Container {
    var point : [2]int
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

@direct_init
struct DefaultInitStruct {

    var a : int = 43
    var b : int = 98

    @constructor
    func make() {

    }

    @constructor
    func make2(check : bool) {
        init {
            a(20)
            b(30)
        }
    }

}

variant IfValVariantTest1 {
    Thing1(value : int)
    Thing2(value : int, value2 : int)
}

const MyInt = 5;

func give_nine_hundred_two() : int {
    return 902;
}

func give_two_hundred_one() : int {
    return 201;
}

func create_pair_point(x : int, y : int) : Point {
    return Point { x : x, y : y }
}

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
    test("for loop works with assignment statement", () => {
        var i = 0;
        var j = 0;
        for(i = 3; i < 7; i++) {
            j++
        }
        return j == 4
    })
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
    test_switch_statement();
    test("struct value initialization", () => {
        var p = Point {
            x : 5,
            y : 6
        };
        return p.x == 5 && p.y == 6;
    });
    test("struct value pointer can be passed using direct struct", () => {
        return give_point_ptr_sum(&Point { x : 10, y : 23 }) == 33;
    })
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
    test("array", () => {
        var arr = [2,4,6,8,10];
        return arr[0] == 2 && arr[1] == 4 && arr[2] == 6;
    })
    test("uninitialized array", () => {
        var arr : [5]int = [];
        arr[0] = 2;
        arr[1] = 4;
        arr[2] = 6;
        return arr[0] == 2 && arr[1] == 4 && arr[2] == 6;
    })
    test("multidimensional uninitialized array", () => {
        var arr : [2][2]int = [];
        arr[0][0] = 2;
        arr[0][1] = 4;
        arr[1][0] = 6;
        arr[1][1] = 8;
        return arr[0][0] == 2 && arr[0][1] == 4 && arr[1][0] == 6 && arr[1][1] == 8;
    })
    test("array inside a struct", () => {
        var ct = Container {
            point : [10,20],
            is_cool : true
        }
        return ct.is_cool && ct.point[0] == 10 && ct.point[1] == 20;
    })
    test("multi dimensional array", () => {
        var arr = [[1, 2], [3, 4]];
        return arr[0][0] == 1 && arr[0][1] == 2 && arr[1][0] == 3 && arr[1][1] == 4;
    })
    test("typealias statement", () => {
        type myint = int;
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
    test("default implementation in interface works when not overridden", () => {
        var p = Point { x : 20, y : 10 }
        return p.dividePDefault() == 2
    })
    test("overridden implementation in interface works when overridden", () => {
        var p = Point { x : 20, y : 10 }
        return p.dividePOverride() == 4
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
        unsafe {
            y = null;
            return y == null;
        }
    })
    test("supports null value - 2", () => {
        var x = 1;
        var y = &x;
        unsafe {
            return y != null;
        }
    })
    test("can store struct in an array", () => {
        var arr = [
            Point {
                x : 3,
                y : 4
            },
            Point {
                x : 5,
                y : 6
            }
        ]
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
    test("if as value works with function calls - 1", () => {
        var val = true;
        var i = if(val) give_nine_hundred_two() else give_two_hundred_one();
        return i == 902;
    })
    test("if as value works with function calls - 1", () => {
        var val = false;
        var i = if(val) give_nine_hundred_two() else give_two_hundred_one();
        return i == 201;
    })
    test("if as a value works with function calls that return struct - 1", () => {
        var val = true;
        var i = if(val) create_pair_point(10, 20) else create_pair_point(5, 2);
        return i.x == 10 && i.y == 20;
    })
    test("if as a value works with function calls that return struct - 2", () => {
        var val = false;
        var i = if(val) create_pair_point(10, 20) else create_pair_point(5, 2);
        return i.x == 5 && i.y == 2;
    })
    test("if as a value works with struct values - 1", () => {
        var val = true;
        var i = if(val) Point { x : 10, y : 20 } else Point { x : 5, y : 2 };
        return i.x == 10 && i.y == 20;
    })
    test("if as a value works with struct values - 2", () => {
        var val = false;
        var i = if(val) Point { x : 10, y : 20 } else Point { x : 5, y : 2 };
        return i.x == 5 && i.y == 2;
    })
    test("if as a value can be passed to function calls as argument - 1", () => {
        var val = true
        var i = create_pair_point(if(val) 10 else 5, if(val) 20 else 2)
        return i.x == 10 && i.y == 20
    })
    test("if as a value can be passed to function calls as argument - 1", () => {
        var val = false
        var i = create_pair_point(if(val) 10 else 5, if(val) 20 else 2)
        return i.x == 5 && i.y == 2
    })
    test("switch as a value works with function calls - 1", () => {
        var val = 45;
        var i = switch(val) {
             45 => give_nine_hundred_two();
             default => give_two_hundred_one();
        }
        return i == 902;
    })
    test("switch as a value works with function calls - 2", () => {
        var val = 32;
        var i = switch(val) {
             45 => give_nine_hundred_two();
             default => give_two_hundred_one();
        }
        return i == 201;
    })
    test("switch as a value works with function calls that return struct - 1", () => {
        var val = 45;
        var i = switch(val) {
             45 => create_pair_point(10, 20);
             default => create_pair_point(5, 2);
        }
        return i.x == 10 && i.y == 20;
    })
    test("switch as a value works with function calls that return struct - 2", () => {
        var val = 32;
        var i = switch(val) {
             45 => create_pair_point(10, 20);
             default => create_pair_point(5, 2);
        }
        return i.x == 5 && i.y == 2;
    })
    test("switch as a value works with struct values - 1", () => {
        var val = 45;
        var i = switch(val) {
             45 => Point { x : 10, y : 20 };
             default => Point { x : 5, y : 2 };
        }
        return i.x == 10 && i.y == 20;
    })
    test("switch as a value works with struct values - 2", () => {
        var val = 32;
        var i = switch(val) {
             45 => Point { x : 10, y : 20 };
             default => Point { x : 5, y : 2 };
        }
        return i.x == 5 && i.y == 2;
    })
    test("switch as a value can be passed to function calls as argument - 1", () => {
        var val = 45;
        var i = create_pair_point(if(val == 45) 10 else 5, if(val == 45) 20 else 2)
        return i.x == 10 && i.y == 20;
    })
    test("switch as a value can be passed to function calls as argument - 2", () => {
        var val = 32;
        var i = create_pair_point(if(val == 45) 10 else 5, if(val == 45) 20 else 2)
        return i.x == 5 && i.y == 2;
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
    test("struct is initialized with default values", () => {
        var d = DefaultInitStruct {}
        return d.a == 43 && d.b == 98
    })
    test("struct is initialized with default values when using constructor", () => {
        var d = DefaultInitStruct();
        return d.a == 43 && d.b == 98
    })
    test("struct is initialized with values inside init block when using constructor", () => {
        var d = DefaultInitStruct(true);
        return d.a == 20 && d.b == 30
    })
    test("pattern matching inside if value works - 1", () => {
        var t = IfValVariantTest1.Thing1(10)
        var j = if(var Thing1(value) = t) value else -1
        return j == 10
    })
    test("pattern matching inside if value works - 2", () => {
        var t = IfValVariantTest1.Thing2(10, 20)
        var j = if(var Thing1(value) = t) value else -1
        return j == -1
    })
    test("pattern matching inside if value works - 3", () => {
        var t = IfValVariantTest1.Thing2(10, 20)
        var j = if(var Thing2(value, value2) = t) value + value2 else -1
        return j == 30
    })
    test("switch as value works with variant cases - 1", () => {
        var t = IfValVariantTest1.Thing1(10)
        var j = switch(t) {
            Thing1(value) => value
            Thing2(value) => -1
        }
        return j == 10;
    })
    test("switch as value works with variant cases - -2", () => {
        var t = IfValVariantTest1.Thing2(20, 30)
        var j = switch(t) {
            Thing1(value) => value
            Thing2(value) => value
        }
        return j == 20;
    })
    test("loop as a value can be passed to function calls as argument", () => {
        var j = 0
        var k = 0
        var i = create_pair_point(loop {
            if(j == 32) {
                break j;
            }
            j++
        }, loop {
            if(k == 12) {
                break k;
            }
            k++
        })
        return i.x == 32 && i.y == 12 && j == 32 && k == 12;
    })
    test("if value inside loop break works - 1", () => {
        var x = 9
        var y = 0
        var z = loop {
            if(y == 12) {
               break if(x == 9) 87 else 98
            }
            y++
        }
        return y == 12 && z == 87
    })
    test("if value inside loop break works - 2", () => {
        var x = 3
        var y = 0
        var z = loop {
            if(y == 12) {
               break if(x == 9) 87 else 98
            }
            y++
        }
        return y == 12 && z == 98
    })
    test("switch value inside loop break works - 1", () => {
        var x = 9
        var y = 0
        var z = loop {
            if(y == 12) {
               break switch(x) {
                    9 => 87
                    default => 98
               }
            }
            y++
        }
        return y == 12 && z == 87
    })
    test("switch value inside loop break works - 2", () => {
        var x = 3
        var y = 0
        var z = loop {
            if(y == 12) {
               break switch(x) {
                    9 => 87
                    default => 98
               }
            }
            y++
        }
        return y == 12 && z == 98
    })
    test("loop value inside if value works - 1", () => {
        var j = 23
        var x = 0
        var i = if(j == 23) loop {
            if(x == 5) {
                break x
            }
            x++
        } else 98
        return i == 5
    })
    test("loop value inside if value works - 2", () => {
        var j = 24
        var x = 0
        var i = if(j == 23) loop {
            if(x == 5) {
                break x
            }
            x++
        } else 98
        return i == 98
    })
    test("loop value inside switch value works - 1", () => {
        var j = 23
        var x = 0
        var i = switch(j) {
            23 => loop {
                if(x == 5) {
                  break x
                }
                x++
            }
            default => 98
        }
        return i == 5
    })
    test("loop value inside switch value works - 2", () => {
        var j = 25
        var x = 0
        var i = switch(j) {
            23 => loop {
                if(x == 5) {
                  break x
                }
                x++
            }
            default => 98
        }
        return i == 98
    })
}

func declared_below() : int {
    return 1;
}