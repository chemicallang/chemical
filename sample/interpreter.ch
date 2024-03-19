func add(a : int, b : int) : int {
    return a + b;
}

func main() : int {

    for(var x = 0; x < 5; x++) {
        printf("check : ", x, '\n');
    }

    printf("check 1 + 2 = ", add(1,2), '\n');

    return 0;
}

var x = main();