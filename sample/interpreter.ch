@lexer
struct html {
    func lex(provider : SourceProvider) {
        var tokens = vector();

        return tokens;
    }
}

func main() : int {
    var x = #html <html></html> #endhtml;
    print(x);
    return 0;
}

var __main__ = main();