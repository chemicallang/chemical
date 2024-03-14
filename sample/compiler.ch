func printf(format : string, args : any...) : int

func main(argc : int) : int {
    printf("number of arguments : %d\n", argc);
    var i = 0;
    do {
        printf("i = %d", i);
        i += 1;
    } while(i < 10);
    return 0;
}