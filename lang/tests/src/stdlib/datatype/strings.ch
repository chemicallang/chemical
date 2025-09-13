
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
    test("appending expressive string into std::string, i32 - 1", () => {
        var first = string("")
        var count = 33
        first.append_expr("I have {count} apples")
        var second = string("I have 33 apples")
        return first.equals(second);
    })
    /**
    test("appending expressive string into std::string, i32 - 2", () => {
        var first = string("")
        var count = 65
        first.append_expr("{count} apples I have, said Yoda")
        var second = string("65 apples I have, said Yoda")
        return first.equals(second);
    })
    **/
    test("appending expressive string into std::string, *char - 3", () => {
        var first = string("")
        var my_str = "something of value"
        first.append_expr("start: {my_str} end")
        var second = string("start: something of value end")
        return first.equals(second);
    })
    test("appending expressive string into std::string, char - 4", () => {
        var first = string("")
        var letter = 'l'
        first.append_expr("letter : {letter} end")
        var second = string("letter : l end")
        return first.equals(second);
    })
    test("appending expressive string into std::string, float - 5", () => {
        var first = string("")
        var pi = 3.14f
        first.append_expr("pi value : {pi} end")
        printf("%s\n", first.data())
        var second = string("pi value : 3.14 end")
        return first.equals(second);
    })
    test("appending expressive string into std::string, double - 6", () => {
        var first = string("")
        var pi = 3.14
        first.append_expr("pi value : {pi} end")
        printf("%s\n", first.data())
        var second = string("pi value : 3.14 end")
        return first.equals(second);
    })
    test("appending expressive string into std::string, u32 - 6", () => {
        var first = string("")
        var val = 99u
        first.append_expr("value : {val} end")
        var second = string("value : 99 end")
        return first.equals(second);
    })
    test("appending expressive string into std::string, i64 - 7", () => {
        var first = string("")
        var val = 99i64
        first.append_expr("value : {val} end")
        var second = string("value : 99 end")
        return first.equals(second);
    })
    test("appending expressive string into std::string, u64 - 8", () => {
        var first = string("")
        var val = 99u64
        first.append_expr("value : {val} end")
        var second = string("value : 99 end")
        return first.equals(second);
    })
}