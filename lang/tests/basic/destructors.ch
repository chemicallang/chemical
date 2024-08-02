import "../test.ch"

struct Destructible {

    var data : int

    var count : int*

    var lamb : (count : int*) => void;

    @destructor
    func delete(&self) {
        self.lamb(self.count);
    }

}

struct Holder1 {
    var thing : Destructible
}

func create_destructible(count : int*, data : int) : Destructible {
    return Destructible {
       data : data,
       count : count,
       lamb : (count : int*) => {
           *count = *count + 1;
       }
    }
}

func destructible_but_last_if(count : int*, data : int) {
    var d = Destructible {
       data : data,
       count : count,
       lamb : (count : int*) => {
           *count = *count + 1;
       }
    }
    var i = 55;
    if(i == 55) {
        var x = 33
    } else {
        var j = 12
    }
}

func destructible_but_last_if_returns(count : int*, data : int) {
    var d = Destructible {
       data : data,
       count : count,
       lamb : (count : int*) => {
           *count = *count + 1;
       }
    }
    var i = 55;
    if(i == 55) {
        var t = 99
        return;
    } else {
        var p = 99
        return;
    }
}

func test_destruction_at_early_return(count : int*, early_return : bool) {
    var d = Destructible {
       count : count,
       lamb : (count : int*) => {
           *count = *count + 1;
       }
    }
    if(early_return) {
        return;
    }
    var z = Destructible {
       count : count,
       lamb : (count : int*) => {
           *count = *count + 1;
       }
    }
}

func test_conditional_destruction(count : int*, condition : bool) {
    var d = Destructible {
       count : count,
       lamb : (count : int*) => {
           *count = *count + 1;
       }
    }
    if(condition) {
        var z = Destructible {
           count : count,
           lamb : (count : int*) => {
               *count = *count + 1;
           }
        }
        return;
    }
}

func test_struct_param_destrutor(d : Destructible) {

}

func test_destructors() {
    test("test that var init struct value destructs", () => {
        var count = 0;
        var data_usable = false;
        if(count == 0){
            var d = Destructible {
                data : 892,
                count : &count,
                lamb : (count : int*) => {
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
            data_usable = d.data == 334 && count == 0;
        }
        return count == 1 && data_usable;
    })
    test("test that var init struct without values get destructed", () => {
        var count = 0;
        var data_usable = false;
        if(count == 0){
            var d : Destructible;
            d.data = 426
            d.count = &count;
            d.lamb = (count : int*) => {
                *count = *count + 1;
            }
            data_usable = d.data == 426;
        }
        return count == 1 && data_usable;
    })
    test("test destruction at early return : true", () => {
         var count = 0;
         test_destruction_at_early_return(&count, true);
         return count == 1;
    })
    test("test destruction at early return : false", () => {
         var count = 0;
         test_destruction_at_early_return(&count, false);
         return count == 2;
    })
    test("test conditional destruction : true", () => {
         var count = 0;
         test_conditional_destruction(&count, true);
         return count == 2;
    })
    test("test conditional destruction : false", () => {
         var count = 0;
         test_conditional_destruction(&count, false);
         return count == 1;
    })
    test("test destruct struct accessed via function call", () => {
        var count = 0;
        var data = create_destructible(&count, 858).data;
        return count == 1 && data == 858;
    })
    test("test destructor is called when access chain is inside a function", () => {
        var count = 0;
        var get_int = (thing : int) => thing;
        var data = get_int(create_destructible(&count, 363).data);
        return count == 1 && data == 363;
    })
    test("test destructor is not called on pointer types", () => {
        var count = 0;
        if(count == 0) {
            var d : Destructible
            d.count = &count;
            d.lamb = (count : int*) => {
                *count = *count + 1;
            }
            var x : Destructible*
            x = &d;
            var y = &d
        }
        return count == 1;
    })
    test("test array values are destructed", () => {
        var count = 0;
        if(count == 0) {
            var arr = {}Destructible(10);
            var i = 0;
            var ptr : Destructible*;
            while(i < 10) {
                ptr = &arr[i];
                ptr.count = &count;
                ptr.lamb = (count : int*) => {
                    *count = *count + 1;
                }
                i++;
            }
        }
        return count == 10;
    })
    test("test array types are destructed", () => {
        var count = 0;
        if(count == 0) {
            var arr : Destructible[10];
            var i = 0;
            var ptr : Destructible*;
            while(i < 10) {
                ptr = &arr[i];
                ptr.count = &count;
                ptr.lamb = (count : int*) => {
                    *count = *count + 1;
                }
                i++;
            }
        }
        return count == 10;
    })
    test("test that destructible struct present inside struct values is destructed", () => {
        var count = 0
        if(count == 0) {
            var holder = Holder1 {
                thing : create_destructible(&count, 332)
            }
        }
        return count == 1
    })
    test("test that destructor works, when last if don't return", () => {
        var count = 0
        destructible_but_last_if(&count, 454);
        return count == 1;
    })
    test("test that destructor works, when last if returns completely", () => {
        var count = 0
        destructible_but_last_if_returns(&count, 655);
        return count == 1;
    })
    test("test structs passed to functions as parameters are automatically destructed - 1", () => {
        var count = 0;
        if(count == 0) {
            test_struct_param_destrutor(
                Destructible {
                    count : &count,
                    lamb : (count : int*) => {
                        *count = *count + 1;
                    }
                }
           )
        }
        return count == 1;
    })
    test("test structs passed to functions as parameters are automatically destructed - 2", () => {
        var count = 0;
        if(count == 0) {
            test_struct_param_destrutor(create_destructible(&count, 223))
        }
        return count == 1;
    })
}