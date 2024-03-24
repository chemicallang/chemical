struct html {
    func value() {
        return "<html></html>";
    }
}

func main() : int {
    var x = html {};
    print(x.value());
    return 0;
}

var __main__ = main();