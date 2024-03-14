func printf(format : string, args : any...) : int

func main(argc : int) : int {
    printf("number of arguments : %d\n", argc);
    var i = 0;
    while(i < 10){
        printf("i = %d", i);
        i += 1;
    }
    return 0;
}