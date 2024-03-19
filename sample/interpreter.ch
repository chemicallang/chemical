func printf(format : string, args : any...) : int

struct Point {
    var x : int;
    var y : int;
}

func main() : int {
    var x = Vox {
        x : 1,
        y : 2
    }
    printf("check its 1 -> ", x.x);
    return 0;
}

var x = main();