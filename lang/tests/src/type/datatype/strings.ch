import "/test.ch"
import "@std/string.ch"
import "@std/hashing/fnv1.ch"

using namespace std;

struct Str {
    var value : *char
}

enum HashingResult {
    First,
    Second,
    Third,
    Hello,
    Wello,
    Dello,
    Jello,
    Nello,
    Xello,
    Unknown
}

func check_str_hash(s : *char) : HashingResult {
    switch(fnv1_hash(s)) {
        comptime_fnv1_hash("first") => {
            return HashingResult.First
        }
        comptime_fnv1_hash("second") => {
            return HashingResult.Second
        }
        comptime_fnv1_hash("third") => {
            return HashingResult.Third
        }
        comptime_fnv1_hash("hello") => {
            return HashingResult.Hello
        }
        comptime_fnv1_hash("wello") => {
            return HashingResult.Wello
        }
        comptime_fnv1_hash("dello") => {
            return HashingResult.Dello
        }
        comptime_fnv1_hash("jello") => {
            return HashingResult.Jello
        }
        comptime_fnv1_hash("nello") => {
            return HashingResult.Nello
        }
        comptime_fnv1_hash("xello") => {
            return HashingResult.Xello
        }
        default => {
            return HashingResult.Unknown
        }
    }
}

func check_str_hash32(s : *char) : HashingResult {
    switch(fnv1a_hash_32(s)) {
        comptime_fnv1a_hash_32("first") => {
            return HashingResult.First
        }
        comptime_fnv1a_hash_32("second") => {
            return HashingResult.Second
        }
        comptime_fnv1a_hash_32("third") => {
            return HashingResult.Third
        }
        comptime_fnv1a_hash_32("hello") => {
            return HashingResult.Hello
        }
        comptime_fnv1a_hash_32("wello") => {
            return HashingResult.Wello
        }
        comptime_fnv1a_hash_32("dello") => {
            return HashingResult.Dello
        }
        comptime_fnv1a_hash_32("jello") => {
            return HashingResult.Jello
        }
        comptime_fnv1a_hash_32("nello") => {
            return HashingResult.Nello
        }
        comptime_fnv1a_hash_32("xello") => {
            return HashingResult.Xello
        }
        default => {
            return HashingResult.Unknown
        }
    }
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
    test("string arrays work too", () => {
        var str : char[] = "hello"
        return str[0] == 'h' && str[4] == 'o' && str[5] == '\0'
    })
    test("string arrays over sized work too", () => {
        var str : char[10] = "hello"
        return str[0] == 'h' && str[4] == 'o' && str[5] == '\0' && str[6] == '\0' && str[9] == '\0';
    })
    test("test two std::strings are equal", () => {
        var first = string("hello world");
        var second = string("hello world");
        return first.equals(&second)
    })
    test("test two std::strings are not equal", () => {
        var first = string("hello world");
        var second = string("not hello world");
        return !first.equals(&second)
    })
    test("test two std::strings are not equal", () => {
        var first = string("hello world");
        var second = string("not hello world");
        return !first.equals(&second)
    })
    test("test can append in std::string", () => {
        var first = string("hello world");
        var second = string("hello worldwo");
        first.append('w');
        first.append('o')
        return first.equals(&second)
    })
    test("test std::string has correct size", () => {
        var first = string("hello world");
        var second = string("hello worldwo");
        return first.size() == 11 && second.size() == 13;
    })
    test("test can verify each character in a data pointer of string", () => {
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
    test("test can append in std::string", () => {
        var first = string("");
        for(var i = 1; i <= 102; i++) {
            first.append('x');
        }
        var second = string("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
        return first.equals(&second);
    })
    test("check multiline strings work", () => {
        var first = "abcdefghij
klmnopqrstuvwxyz"
        return strlen(first) == 27
    })
    test("hashing algorithm fnv1 results in same hash in comptime and runtime mode", () => {
        return comptime_fnv1_hash("next") == fnv1_hash("next")
    })
    test("hashing algorithm fnv1 works in both runtime and comptime - 1", () => {
        return check_str_hash("hello") == HashingResult.Hello;
    })
    test("hashing algorithm fnv1 works in both runtime and comptime - 2", () => {
        return check_str_hash("wello") == HashingResult.Wello;
    })
    test("hashing algorithm fnv1 works in both runtime and comptime - 3", () => {
        return check_str_hash("dello") == HashingResult.Dello;
    })
    test("hashing algorithm fnv1 works in both runtime and comptime - 4", () => {
        return check_str_hash("jello") == HashingResult.Jello;
    })
    test("hashing algorithm fnv1 works in both runtime and comptime - 5", () => {
        return check_str_hash("nello") == HashingResult.Nello;
    })
    test("hashing algorithm fnv1 works in both runtime and comptime - 6", () => {
        return check_str_hash("xello") == HashingResult.Xello;
    })
    test("hashing algorithm fnv1 works in both runtime and comptime - 7", () => {
        return check_str_hash("xcvcx") == HashingResult.Unknown;
    })
    test("hashing algorithm fnv1 works in both runtime and comptime - 8", () => {
        return check_str_hash("first") == HashingResult.First;
    })
    test("hashing algorithm fnv1 works in both runtime and comptime - 9", () => {
        return check_str_hash("second") == HashingResult.Second;
    })
    test("hashing algorithm fnv1 works in both runtime and comptime - 10", () => {
        return check_str_hash("third") == HashingResult.Third;
    })
    test("hashing algorithm fnv1a-32 works in both runtime and comptime - 1", () => {
        return check_str_hash32("hello") == HashingResult.Hello;
    })
    test("hashing algorithm fnv1a-32 works in both runtime and comptime - 2", () => {
        return check_str_hash32("wello") == HashingResult.Wello;
    })
    test("hashing algorithm fnv1a-32 works in both runtime and comptime - 3", () => {
        return check_str_hash32("dello") == HashingResult.Dello;
    })
    test("hashing algorithm fnv1a-32 works in both runtime and comptime - 4", () => {
        return check_str_hash32("jello") == HashingResult.Jello;
    })
    test("hashing algorithm fnv1a-32 works in both runtime and comptime - 5", () => {
        return check_str_hash32("nello") == HashingResult.Nello;
    })
    test("hashing algorithm fnv1a-32 works in both runtime and comptime - 6", () => {
        return check_str_hash32("xello") == HashingResult.Xello;
    })
    test("hashing algorithm fnv1a-32 works in both runtime and comptime - 7", () => {
        return check_str_hash32("xcvcx") == HashingResult.Unknown;
    })
    test("hashing algorithm fnv1a-32 works in both runtime and comptime - 8", () => {
        return check_str_hash32("first") == HashingResult.First;
    })
    test("hashing algorithm fnv1a-32 works in both runtime and comptime - 9", () => {
        return check_str_hash32("second") == HashingResult.Second;
    })
    test("hashing algorithm fnv1a-32 works in both runtime and comptime - 10", () => {
        return check_str_hash32("third") == HashingResult.Third;
    })
}