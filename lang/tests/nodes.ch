import "test.ch"

interface Calculator {

    func sum(x : int, y : int) : int

    func multiply(x : int, y : int) : int {
        return x * y;
    }

}

struct Point : Calculator {

    var x : int
    var y : int

    func sum(x : int, y : int) : int {
        return x + y;
    }

    func sumP(&self) : int {
        return self.x + self.y;
    }

}

func test_nodes() {
    test("address of works", []() => {
        var x = 5;
        printf("checkout the address %p\n", &x);
        return true;
    });
    test("dereferencing works", []() => {
        var x = 5;
        var y = &x;
        return *y == 5;
    });
    test("for loop", []() => {
       var j = 0;
       for(var i = 0;i < 5; i++) {
           j++;
       }
       return j == 5;
    });
    test("while loop", []() => {
       var j = 0;
       while(j != 5) {
            j++;
       }
       return j == 5;
    });
    test("do while loop", []() => {
       var j = 0;
       do {
            j++;
       } while(j != 5);
       return j == 5;
    });
    test("switch statement", []() => {
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
    test("struct value initialization", []() => {
        var p = Point {
            x : 5,
            y : 6
        };
        return p.x == 5 && p.y == 6;
    });
    test("struct functions without self ref", []() => {
         var p = Point {
             x : 0,
             y : 0
         };
        return p.sum(5, 6) == 11;
    });
    test("direct calls to struct functions", []() => {
        return Point.sum(6, 6) == 12;
    });
    test("struct functions with self ref", []() => {
         var p = Point {
             x : 7,
             y : 6
         };
        return p.sumP() == 13;
    });
    test("test array", []() => {
        var arr = {2,4,6,8,10};
        return arr[0] == 2 && arr[1] == 4 && arr[2] == 6;
    })
    test("typealias statement", []() => {
        typealias myint = int;
        var y : myint = 5;
        return y == 5;
    })
    test("can call functions declared below call", []() => {
        return declared_below() == 1;
    })
    test("can call interface defined functions directly", []() => {
        return Calculator.multiply(5, 5) == 25;
    })
    test("can call interface declared functions directly", []() => {
        return Calculator.sum(5, 5) == 10;
    })
}

func declared_below() : int {
    return 1;
}