
using namespace std;

func test_strings() {
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
}