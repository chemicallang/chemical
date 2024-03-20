struct Point {
    var x : int;
    var y : int;
}

func create_point() {
    return Point {
       x : 1,
       y : 2
    };
}

func main() : int {

    var p = create_point();

    p.x = 5;
    p.y = 6;

    printf("check x = ", p.x, " check y = ", p.y, '\n');

    return 0;

}

var z = main();