public const SameStringContentsSame1 = "This string has same contents in same module"
public const SameStringContentsSame2 = "This string has same contents in same module"

public const SameStringContents2 = "This string has same contents in two modules"

struct Str {
    var value : *char
}

func test_char_ptr_strings() {
    test("check string indexing works", () => {
        var x = "true";
        return x[0] == 't' && x[1] == 'r' && x[2] == 'u' && x[3] == 'e';
    })
    test("string indexing inside struct", () => {
        var str = Str {
            value : "false"
        }
        return str.value[0] == 'f' && str.value[1] == 'a' && str.value[2] == 'l';
    })
    test("escaped characters in strings work", () => {
        var str = "\n\t";
        return str[0] == '\n' && str[1] == '\t';
    })
    test("string arrays work too", () => {
        var str : char[] = "hello"
        return str[0] == 'h' && str[4] == 'o' && str[5] == '\0'
    })
    test("string arrays over sized work too", () => {
        var str : char[10] = "hello"
        return str[0] == 'h' && str[4] == 'o' && str[5] == '\0' && str[6] == '\0' && str[9] == '\0';
    })
    test("check multiline strings work", () => {
        var first = """abcdefghij
klmnopqrstuvwxyz"""
        return strlen(first) == 28
    })
    test("same contents of strings in same module have same char ptr", () => {
        return SameStringContentsSame1 == SameStringContentsSame2
    })
    test("same contents of strings in two modules have same char ptr", () => {
        return SameStringContents == SameStringContents2
    })
    test("single line strings end with a null terminator", () => {
        var myStr = "abc"
        return myStr[3] == '\0'
    })
    test("multi line strings end with a null terminator", () => {
        var myStr = """abc"""
        return myStr[3] == '\0'
    })
}