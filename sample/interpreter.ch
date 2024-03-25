@lexer
struct html {
    func lex(provider : SourceProvider) {
        var tokens = vector();
        var aToken = UserToken {
            line : provider.getLineNumber(),
            character : provider.getLineCharNumber(),
            length : 0
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