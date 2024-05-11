import "../../test.ch"

struct Str {
    var value : string
}

func test_strings() {
    test("check string indexing works", () => {
        var x = "true";
        return x[0] == 't' && x[1] == 'r' && x[2] == 'u' && x[3] == 'e';
    })
    test("test string indexing inside struct", () => {
        var str = Str {
            value : "false"
        }
        return str.value[0] == 'f' && str.value[1] == 'a' && str.value[2] == 'l';
    })
    test("test escaped characters in strings work", () => {
        var str = "\n\t";
        return str[0] == '\n' && str[1] == '\t';
    })
}