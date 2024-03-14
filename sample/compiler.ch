func printf(format : string, args : any...) : int

func add(a : int, b : int) : int {
    return a + b;
}

func main(argc : int) : int {
    printf("number of arguments : %d\n", argc);
    printf("function sum : %d\n", add(5, 4))
    printf("check this char '%c'\n", 'x');
    var arr = [2,4,6,8,10];
    var i = 0;
    while(i < 5) {
        if(i == 4) {
            break;
        }
        printf("current : %d\n", arr[i]);
        i += 1;
    }
    return 0;
}