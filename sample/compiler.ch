func printf(format : string, args : any...) : int

func main() : int {
    var x = 1;
    x += 5;
    if(x == 3){
        printf("congrats x == 6 : %d\n", x);
    } else if(x == 4) {
        printf("yo x = 6\n");
    } else {
        printf("nuts! x != 6\n");
    }
    printf("hello world : %d", 2 + 5 * 2);
    return 0;
}