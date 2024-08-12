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

}

func (calc : Calculator*) extension_div() : int {
    return calc.divideP();
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
            case 0 -> {
                return true;
            }
            case 1 -> {
                return false;
            }
            default -> {
                return false;
            }
       }
    });
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
    test("extension functions on interfaces also work", () => {
         var p = Point {
             x : 15,
             y : 5
         };
        return p.extension_div() == 3;
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
}

func declared_below() : int {
    return 1;
}