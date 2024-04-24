import "test.ch"

struct Point {

    var x : int
    var y : int

    func sum(x : int, y : int) : int {
        return x + y;
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
    test("struct functions without self ref work", []() => {
         var p = Point {
             x : 0,
             y : 0
         };
        return p.sum(5, 6) == 11;
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
}