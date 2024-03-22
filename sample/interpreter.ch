struct Point {
    var x : int
    var y : int
    func modify() {
        this.x = 5;
        this.y = 6;
    }
    func print() {
        printf("x = ", this.x, ", y = ", this.y);
    }
}

func main() : int {
    var p = Point {
        x : 1,
        y : 2
    }
    p.modify();
    p.print();
    return 0;
}

var __main__ = main();