
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

struct Deletable {
    var counter : *mut int
    @delete
    func delete(&self) { unsafe {
        *counter = *counter + 1
     }}
}

func test_optional_type() { unsafe {
    test("check option type works - 1", () => {
        var o = get_optional_int(true);
        return get_opt_value(o) == 32
    })
    test("check option type works - 2", () => {
        var o = get_optional_int(false);
        return get_opt_value(o) == -1
    })
    test("safe take returns Option and leaves None", () => {
        var o = Option.Some<int>(7)
        var taken = o.take()
        if(taken is Option.None) { return false }
        if(o !is Option.None) { return false }
        var empty = Option.None<int>()
        return empty.take() is Option.None
    })

    test("taking the option works - 1", () => {
        var o = get_optional_int(true)
        var result = o.take_unchecked()
        return result == 32 && o is Option.None
    })
    test("taking the option destructs once only", () => {
        var counter = 0
        if(counter == 0) {
            var o = Option.Some<Deletable>(Deletable { counter : &raw mut counter })
            var taken = o.take_unchecked()
            if(o !is Option.None) {
                return false;
            }
        }
        return counter == 1
    })
    test("pattern matching on option, doesn't destroy or move it", () => {
        var counter = 0
        var s = Option.Some<Deletable>(Deletable { counter : &raw mut counter })
        if(counter == 0) {
            var Some(value) = s else unsafe { unreachable }
            if(*value.counter != 0) return false;
        }
        if(counter != 0) return false;
        if(counter == 0) {
            var taken = s.take_unchecked()
            // destructs here
        }
        if(counter != 1) return false;
        if(s !is Option.None) return false;
        return true;
    })
 }}