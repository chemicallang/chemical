func printf(format : string, args : any...) : int

func main(argc : int) : int {
    printf("number of arguments : %d\n", argc);
    var arr = [2,4,6,8,10];
    var i = 0;
    while(i < 5) {
        printf("current : %d\n", arr[i]);
        i += 1;
    }
    return 0;
}