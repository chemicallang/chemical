@lexer
struct html {
    var i : int
}

func main() : int {
    var v = vector();
    v.push(11);
    v.push(22);
    v.push(33);
    var i = 0;
    while(i < v.size()) {
        print(i, " = ", v[i]);
        if(i < (v.size() - 1)) {
            print(',');
        }
        i++;
    }
    return 0;
}

var __main__ = main();