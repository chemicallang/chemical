import "/test.ch"
import "@std/hashing/fnv1.ch"
import "@std/hashing/murmur.ch"

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
        comptime { fnv1_hash("first") } => {
            return HashingResult.First
        }
        comptime { fnv1_hash("second") } => {
            return HashingResult.Second
        }
        comptime { fnv1_hash("third") } => {
            return HashingResult.Third
        }
        comptime { fnv1_hash("hello") } => {
            return HashingResult.Hello
        }
        comptime { fnv1_hash("wello") } => {
            return HashingResult.Wello
        }
        comptime { fnv1_hash("dello") } => {
            return HashingResult.Dello
        }
        comptime_fnv1_hash("jello") => {
            return HashingResult.Jello
        }
        comptime { fnv1_hash("nello") } => {
            return HashingResult.Nello
        }
        comptime { fnv1_hash("xello") } => {
            return HashingResult.Xello
        }
        default => {
            return HashingResult.Unknown
        }
    }
}

func check_str_hash32(s : *char) : HashingResult {
    switch(fnv1a_hash_32(s)) {
        comptime { fnv1a_hash_32("first") } => {
            return HashingResult.First
        }
        comptime { fnv1a_hash_32("second") } => {
            return HashingResult.Second
        }
        comptime { fnv1a_hash_32("third") } => {
            return HashingResult.Third
        }
        comptime { fnv1a_hash_32("hello") } => {
            return HashingResult.Hello
        }
        comptime { fnv1a_hash_32("wello") } => {
            return HashingResult.Wello
        }
        comptime { fnv1a_hash_32("dello") } => {
            return HashingResult.Dello
        }
        comptime { fnv1a_hash_32("jello") } => {
            return HashingResult.Jello
        }
        comptime { fnv1a_hash_32("nello") } => {
            return HashingResult.Nello
        }
        comptime { fnv1a_hash_32("xello") } => {
            return HashingResult.Xello
        }
        default => {
            return HashingResult.Unknown
        }
    }
}

@comptime
func comptime_murmur(s : *char) : uint32_t {
    return murmurhash(s, compiler::size(s), 0)
}

/**
func check_murmur_hash(s : *char) : HashingResult {
    switch(murmurhash(s, strlen(s), 0)) {
        comptime { comptime_murmur("first") } => {
            return HashingResult.First
        }
        comptime { comptime_murmur("second") } => {
            return HashingResult.Second
        }
        comptime { comptime_murmur("third") } => {
            return HashingResult.Third
        }
        comptime { comptime_murmur("hello") } => {
            return HashingResult.Hello
        }
        comptime { comptime_murmur("wello") } => {
            return HashingResult.Wello
        }
        comptime { comptime_murmur("dello") } => {
            return HashingResult.Dello
        }
        comptime { comptime_murmur("jello") } => {
            return HashingResult.Jello
        }
        comptime { comptime_murmur("nello") } => {
            return HashingResult.Nello
        }
        comptime { comptime_murmur("xello") } => {
            return HashingResult.Xello
        }
        default => {
            return HashingResult.Unknown
        }
    }
}
**/

func test_hashing() {
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
    /** TODO murmur hashing algorithm doesn't work in comptime
    test("hashing algorithm murmur works in both runtime and comptime - 1", () => {
        return check_murmur_hash("hello") == HashingResult.Hello;
    })
    test("hashing algorithm murmur works in both runtime and comptime - 2", () => {
        return check_murmur_hash("wello") == HashingResult.Wello;
    })
    test("hashing algorithm murmur works in both runtime and comptime - 3", () => {
        return check_murmur_hash("dello") == HashingResult.Dello;
    })
    test("hashing algorithm murmur works in both runtime and comptime - 4", () => {
        return check_murmur_hash("jello") == HashingResult.Jello;
    })
    test("hashing algorithm murmur works in both runtime and comptime - 5", () => {
        return check_murmur_hash("nello") == HashingResult.Nello;
    })
    test("hashing algorithm murmur works in both runtime and comptime - 6", () => {
        return check_murmur_hash("xello") == HashingResult.Xello;
    })
    test("hashing algorithm murmur works in both runtime and comptime - 7", () => {
        return check_murmur_hash("xcvcx") == HashingResult.Unknown;
    })
    test("hashing algorithm murmur works in both runtime and comptime - 8", () => {
        return check_murmur_hash("first") == HashingResult.First;
    })
    test("hashing algorithm murmur works in both runtime and comptime - 9", () => {
        return check_murmur_hash("second") == HashingResult.Second;
    })
    test("hashing algorithm murmur works in both runtime and comptime - 10", () => {
        return check_murmur_hash("third") == HashingResult.Third;
    })
    **/
}