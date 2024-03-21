@lexer
struct html : Lexer {
    override func lexTokens(provider : SourceProvider) : vector<UserToken> {
        var tokens = vector<UserToken>();

        return tokens;
    }
}

func main() : int {
    var str = "something I'd like to say";
    if(str[0] == 's') {
        printf("hoola hoo");
    }
    return 0;
}

var __main__ = main();