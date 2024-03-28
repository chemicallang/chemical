func printf(format : string, args : any...) : int

func add(a : int, b : int) : int {
    return a + b;
}

struct Point {
    var x : int
    var y : int
}

func main(argc : int, argv : string*) : int {
    printf("number of arguments : %d\n", argc);
    printf("printing args : \n");
    // for(var a = 0; a < argc; a++){
    //    printf("%d = %s\n", a, argv + a);
    // }
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
    return 0;
}