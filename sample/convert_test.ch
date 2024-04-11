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
        continue;
        printf("%d = %s\n", i, argv[i]);
        i++;
    }
}

func main(argc : int, argv : string*) {
    x.y += (7 + 1) * (8 + 2);
}