import "../test.ch"

struct Destructible {

    var data : int

    var lamb : []() => void;

    @destructor
    func delete(&self) {
        self.lamb();
    }

}

func create_destructible(count : int*, data : int) : Destructible {
    return Destructible {
       data : data,
       lamb : [count]() => {
           *count = *count + 1;
       }
    }
}

func test_destructors() {
    test("test that var init struct value destructs", () => {
        var count = 0;
        var data_usable = false;
        if(count == 0){
            var d = Destructible {
                data : 892,
                lamb : [&count]() => {
                    *count = *count + 1;
                }
            }
            data_usable = d.data == 892;
        }
        return count == 1 && data_usable;
    })
    test("test that functions returning struct don't destruct the struct", () => {
        var count = 0;
        var data_usable = false;
        if(count == 0){
            var d = create_destructible(&count, 334);
            data_usable = d.data == 334;
        }
        return count == 1 && data_usable;
    })
    test("test that var init struct without values get destructed", () => {
        var count = 0;
        var data_usable = false;
        if(count == 0){
            var d : Destructible;
            d.data = 426
            d.lamb = [&count]() => {
                *count = *count + 1;
            }
            data_usable = d.data == 426;
        }
        return count == 1 && data_usable;
    })
}