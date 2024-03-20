struct Point {
    var x : int;
    var y : int;
}

func modify_point(point : Point) {
    point.x = 5;
    point.y = 6;
}

func main() : int {

    var p = Point {
        x : 1,
        y : 2
    }

    modify_point(p);

    printf("check x = ", p.x, " check y = ", p.y, '\n');

    return 0;

}

var z = main();