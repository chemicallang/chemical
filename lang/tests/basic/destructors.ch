import "../test.ch"

struct Destructible {

    var lamb : []() => void;

    @destructor
    func delete(&self) {
        self.lamb();
    }

}

func destructor_times(ptr : int*) : []() => void {
    return [&ptr]() => {
        *ptr = *ptr + 1;
    }
}

func caller(callee : () => void) {
    callee();
}

func test_destructors() {
    test("test that var init struct value destructs", () => {
        var count = 0;
        if(count == 0){
            var d = Destructible {
                lamb : [&count]() => {
                    *count = *count + 1;
                }
            }
        }
        return count == 1;
    })
}