import "test.ch"

struct Point {
    var x : int
    var y : int
}

func test_nodes() {
    test("address of works", []() => {
        var x = 5;
        printf("checkout the address %p\n", &x);
        return true;
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
       }
    });
    test("struct value initialization", []() => {
        var p = Point {
            x : 5,
            y : 6
        };
        return p.x == 5 && p.y == 6;
    });
}