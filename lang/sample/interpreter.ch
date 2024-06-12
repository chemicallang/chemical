

@scope:lexer
struct UserToken {
    var line : int
    var character : int
    var length : int
    var data : int
}

enum

@scope:lexer
@lexer
struct html {

    var tokens : vector = vector()

    func lex(provider : SourceProvider*) {
        var aToken = UserToken {
            line : provider.getLineNumber(),
            character : provider.getLineCharNumber(),
            length : 0,
            data : 0
        }
        this.tokens.push(aToken);
    }

}

func main() : int {
    var x = #html <html></html> #endhtml;
    print(x);
    return 0;
}

var __main__ = main();