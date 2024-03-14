func printf(format : string, args : any...) : int

func add(a : int, b : int) : int {
    return a + b;
}

func main(argc : int) : int {
    printf("number of arguments : %d\n", argc);
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
        i += 1;
    }
    for(var j = 0; j < 5; j+=1) {
        printf("for loop : %d\n", j);
    }
    return 0;
}