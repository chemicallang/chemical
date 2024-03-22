struct Point {
    var x : int
    var y : int
    func print() {
        printf("x = ", x, ", y = ", y);
    }
}

func main() : int {
    var p = Point {
        x : 1,
        y : 2
    }
    p.print();
    return 0;
}

var __main__ = main();