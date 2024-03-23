struct wow {
    var i : int
}

func modify(x : wow) {
    x.i = 6;
}

func lex() {
    var x = wow {
        i : 5
    };
    // pass by reference
    modify(x);
    // move
    var y = x;
    // x is invalid, since moved
    return y;
}

func main() : int {
    print(lex());
    return 0;
}

var __main__ = main();