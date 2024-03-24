@lexer
struct html {
    func lex(provider : SourceProvider) {
        var tokens = vector();
        var aToken = UserToken {
            line : 5,
            character : 5,
            length : 5
        }
        tokens.push(aToken);
        return tokens;
    }
}

func main() : int {
    var x = #html <html></html> #endhtml;
    print(x);
    return 0;
}

var __main__ = main();