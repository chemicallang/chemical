import "../../test.ch"
import "@std/option.ch"

using namespace std;

func get_optional_int(some : bool) : Option<int> {
    if(some) {
        return Option.Some(32)
    } else {
        return Option.None()
    }
}

func get_opt_value(o : Option<int>) : int {
    switch(o) {
        Some(value) => {
            return value;
        }
        None => {
            return -1
        }
    }
}

func test_optional_type() {
    test("check option type works - 1", () => {
        var o = get_optional_int(true);
        return get_opt_value(o) == 32
    })
    test("check option type works - 2", () => {
        var o = get_optional_int(false);
        return get_opt_value(o) == -1
    })
}