struct Point {
    var x : int;
    var y : int;
}

func main() : int {

    var p = Point {
        x : 1,
        y : 2
    }

    printf("check x = ", p.x, " check y = ", p.y, '\n');

    return 0;

}

var z = main();