func printf(format : string, args : any...) : int

func add(a : int, b : int) : int {
    return a + b;
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

func main(argc : int, argv : string*) : int {
    print_args(argc, argv);
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
    var p = Point {
        x : 5,
        y : 6
    };
    printf("Point : x = %d, y = %d", p.x, p.y);
    return 0;
}