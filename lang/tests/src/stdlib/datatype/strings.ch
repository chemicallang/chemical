
using namespace std;

struct Str {
    var value : *char
}

func test_strings() {
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
    test("two std::strings are equal", () => {
        var first = string("hello world");
        var second = string("hello world");
        return first.equals(second)
    })
    test("two std::strings are not equal", () => {
        var first = string("hello world");
        var second = string("not hello world");
        return !first.equals(second)
    })
    test("two std::strings are not equal", () => {
        var first = string("hello world");
        var second = string("not hello world");
        return !first.equals(second)
    })
    test("can append in std::string", () => {
        var first = string("hello world");
        var second = string("hello worldwo");
        first.append('w');
        first.append('o')
        return first.equals(second)
    })
    test("std::string has correct size", () => {
        var first = string("hello world");
        var second = string("hello worldwo");
        return first.size() == 11 && second.size() == 13;
    })
    test("can verify each character in a data pointer of string", () => {
        var first = string("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
        var second = string("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
        var first_data = first.data();
        var second_data = second.data();
        for(var i = 1; i <= 102; i++) {
            if(first_data[i] != second_data[i]) {
                return false;
            }
        }
        return true;
    })
    test("can append in std::string", () => {
        var first = string("");
        for(var i = 1; i <= 102; i++) {
            first.append('x');
        }
        var second = string("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
        return first.equals(second);
    })
    test("check multiline strings work", () => {
        var first = """abcdefghij
klmnopqrstuvwxyz"""
        return strlen(first) == 27
    })
}