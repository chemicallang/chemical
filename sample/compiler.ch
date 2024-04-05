import "std.ch"

func add(a : int, b : int) : int {
    return a + b;
}

func check_lambda(lamb : (a : int, b : int) => int) {
    printf("check lambda result : %d\n", lamb(2, 2));
}

func print_args(argc : int, argv : string*) {
    printf("number of arguments : %d\n", argc);
    printf("printing args : \n");
    var i = 0;
    while(i < argc){
        printf("%d = %s\n", i, argv[i]);
        i++;
    }
}

struct Point {
    var x : int
    var y : int
}

struct Nested {
    var j : int
    var nested : Point
    var d : int
}

func test_structs() {
    var p = Point {
        x : 5,
        y : 6
    };
    printf("Point : x = %d, y = %d\n", p.x, p.y);
    var n = Nested {
        j : 1,
        nested : Point {
            x : 33,
            y : 55
        }
        d : 2
    };
    printf("j = %d, Nested Point : x = %d, y = %d, d = %d\n", n.j, n.nested.x, n.nested.y, n.d);
}

func test_macro() {
    var html = #html <html></html> #endhtml;
    printf("check my html : %s\n", html);
}

const global_int = 444;
const my_global_str = "something ain't right";

func main(argc : int, argv : string*) : int {
    print_args(argc, argv);
    printf("check global works : %d\n", global_int);
    printf("global string works : %s\n", my_global_str);
    printf("function sum : %d\n", add(5, 4))
    printf("check this char '%c'\n", 'x');
    printf("check this float : %f\n", 1.12345);
    var arr = [2,4,6,8,10];
    var i = 0;
    while(i < 5) {
        if(i == 4) {
            break;
        }
        printf("current : %d\n", arr[i]);
        i++;
    }
    for(var j = 0; j < 5; j++) {
        printf("for loop : %d\n", j);
    }
    var j = 10;
    printf("for loop end : %d\n", j);
    var c : int;
    c = 155;
    printf("check c : %d\n", c);
    test_structs();
    test_macro();
    var tt = 3;
    switch(tt) {
        case 3 -> {
            printf("it is 3\n");
        }
        case 5 -> {
            printf("it is 5\n");
        }
    }
    check_lambda(add);
    return 0;
}