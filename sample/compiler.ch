func printf(format : string, args : any...) : int

func main() : int {
    var x = 1;
    x += 5;
    if(x == 6){
        printf("congrats x == 6 : %d\n", x);
    } else {
        printf("nuts! x != 6");
    }
    printf("hello world : %d", 2 + 5 * 2);
    return 0;
}