import "std.ch"

func add(a : int, b : int) : int {
    return a + b;
}

func check_lambda(lamb : (a : int, b : int) => int) {
    printf("check lambda result : %d\n", lamb(2, 2));
}

func main(argc : int, argv : string*) {
    var x = 5;
    x.y += 7 + 1;
}