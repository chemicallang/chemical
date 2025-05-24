struct Destructible {

    var data : int

    var count : *mut int

    var lamb : (count : *mut int) => void;

    func copy(&self) : Destructible {
        return Destructible {
            data : data + 1,
            count : count,
            lamb : lamb
        }
    }

    @delete
    func delete(&self) {
        self.lamb(self.count);
    }

}


type DestructibleAlias = Destructible

func take_destructible_i(d : Destructible) {

}

func take_destructible_alias_i(d : DestructibleAlias) {

}

func take_destructible_ref(ref : &Destructible) {

}

struct GenDestruct<T> {

    var data : T

    var count : *mut int

    var lamb : (count : *mut int) => void;

    @delete
    func delete(&self) {
        self.lamb(self.count);
    }

}

func take_gen_destruct_short(d : GenDestruct<short>) {

}

func take_gen_destruct_ref(g : &GenDestruct<short>) {

}

struct GenDestructOwner {
    var d : GenDestruct<long>
}

func create_long_gen_dest(data : long, count : *mut int, lamb : (count : *mut int) => void) : GenDestruct<long> {
    return GenDestruct<long> {
        data : data,
        count : count,
        lamb : lamb
    }
}

func create_short_gen_dest(data : short, count : *mut int, lamb : (count : *mut int) => void) : GenDestruct<short> {
    return GenDestruct<short> {
        data : data,
        count : count,
        lamb : lamb
    }
}

struct Holder1 {
    var thing : Destructible
}

struct Holder2 {
    var thing : DestructibleAlias
}

func create_destructible(count : *mut int, data : int) : Destructible {
    return Destructible {
       data : data,
       count : count,
       lamb : (count : *mut int) => {
           *count = *count + 1;
       }
    }
}

func create_destructible_alias(count : *mut int, data : int) : DestructibleAlias {
    return DestructibleAlias {
       data : data,
       count : count,
       lamb : (count : *mut int) => {
           *count = *count + 1;
       }
    }
}

func destruct_inc_count(count : *mut int) {
    *count = *count + 1;
}

func destructible_but_last_if(count : *mut int, data : int) {
    var d = Destructible {
       data : data,
       count : count,
       lamb : (count : *mut int) => {
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

func destructible_but_last_if_returns(count : *mut int, data : int) {
    var d = Destructible {
       data : data,
       count : count,
       lamb : (count : *mut int) => {
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

func test_destruction_at_early_return(count : *mut int, early_return : bool) {
    var d = Destructible {
       count : count,
       lamb : (count : *mut int) => {
           *count = *count + 1;
       },
       data : 9990
    }
    if(early_return) {
        return;
    }
    var z = Destructible {
       count : count,
       lamb : (count : *mut int) => {
           *count = *count + 1;
       },
       data : 9990
    }
}

func test_conditional_destruction(count : *mut int, condition : bool) {
    var d = Destructible {
       count : count,
       lamb : (count : *mut int) => {
           *count = *count + 1;
       },
       data : 9990
    }
    if(condition) {
        var z = Destructible {
           count : count,
           lamb : (count : *mut int) => {
               *count = *count + 1;
           },
           data : 9990
        }
        return;
    }
}

func test_struct_param_destructor(d : Destructible) {

}

func test_return_struct_param(d : Destructible) : Destructible {
    return d;
}

func test_ret_ptr_to_d(d : *mut Destructible) : *mut Destructible {
    return d;
}

func send_lambda_struct(data : int, count : *mut int, lamb : (d : Destructible) => void) {
    lamb(Destructible {
        data : data,
        count : count,
        lamb : destruct_inc_count
    })
}

var my_string_destr_count : int = 0

struct my_string {

    @comptime
    @constructor
    func make(value : literal<string>) {
        return compiler::wrap(constructor(value, compiler::size(value)))
    }

    @constructor
    func constructor(value : *char, length : ubigint) {

    }

    @delete
    func delete(&self) {
        my_string_destr_count = my_string_destr_count + 1;
    }

}

func relative_path(path : my_string) : my_string {
    const m = my_string("wow");
    return m;
}

variant OptDestructible {
    Some(d : Destructible)
    None()
}

func test_variant_destruction_simple(count : *mut int) {
    var d = OptDestructible.Some(Destructible {
        count : count,
        lamb : (count : *mut int) => {
            *count = *count + 1;
        },
        data : 3422
     })
}

func test_variant_param_destructor(d : OptDestructible) {

}

func test_return_variant_param(d : OptDestructible) : OptDestructible {
    return d;
}

func test_variant_destruction_at_early_return(count : *mut int, early_return : bool) {
    var d = OptDestructible.Some(Destructible {
        count : count,
        lamb : (count : *mut int) => {
            *count = *count + 1;
        },
        data : 9990
     })
    if(early_return) {
        return;
    }
    var z = OptDestructible.Some(Destructible {
        count : count,
        lamb : (count : *mut int) => {
            *count = *count + 1;
        },
        data : 9990
     })
}

func test_variant_conditional_destruction(count : *mut int, condition : bool) {
    var d = OptDestructible.Some(Destructible {
        count : count,
        lamb : (count : *mut int) => {
            *count = *count + 1;
        },
        data : 9990
     })
    if(condition) {
        var z = OptDestructible.Some(Destructible {
            count : count,
            lamb : (count : *mut int) => {
                *count = *count + 1;
            },
            data : 9990
         })
        return;
    }
}

func test_destructors() {
    test("var init struct value destructs", () => {
        var count = 0;
        var data_usable = false;
        if(count == 0){
            var d = Destructible {
                data : 892,
                count : &count,
                lamb : (count : *mut int) => {
                    *count = *count + 1;
                }
            }
            data_usable = d.data == 892;
        }
        return count == 1 && data_usable;
    })
    test("var init struct value with typealias destructs", () => {
        var count = 0;
        var data_usable = false;
        if(count == 0){
            var d = DestructibleAlias {
                data : 892,
                count : &count,
                lamb : (count : *mut int) => {
                    *count = *count + 1;
                }
            }
            data_usable = d.data == 892;
        }
        return count == 1 && data_usable;
    })
    test("functions returning struct don't destruct the struct", () => {
        var count = 0;
        var data_usable = false;
        if(count == 0){
            var d = create_destructible(&count, 334);
            data_usable = d.data == 334 && count == 0;
        }
        return count == 1 && data_usable;
    })
    test("var init struct without values get destructed", () => {
        var count = 0;
        var data_usable = false;
        if(count == 0){
            var d : Destructible;
            d.data = 426
            d.count = &count;
            d.lamb = (count : *mut int) => {
                *count = *count + 1;
            }
            data_usable = d.data == 426;
        }
        return count == 1 && data_usable;
    })
    test("var init struct typealias without values get destructed", () => {
        var count = 0;
        var data_usable = false;
        if(count == 0){
            var d : DestructibleAlias;
            d.data = 426
            d.count = &count;
            d.lamb = (count : *mut int) => {
                *count = *count + 1;
            }
            data_usable = d.data == 426;
        }
        return count == 1 && data_usable;
    })
    test("destruction at early return : true", () => {
         var count = 0;
         test_destruction_at_early_return(&count, true);
         return count == 1;
    })
    test("destruction at early return : false", () => {
         var count = 0;
         test_destruction_at_early_return(&count, false);
         return count == 2;
    })
    test("conditional destruction : true", () => {
         var count = 0;
         test_conditional_destruction(&count, true);
         return count == 2;
    })
    test("conditional destruction : false", () => {
         var count = 0;
         test_conditional_destruction(&count, false);
         return count == 1;
    })
    test("destruct struct created via function call", () => {
        var count = 0;
        if(count == 0) {
            var d = create_destructible(&count, 852);
        }
        return count == 1;
    })
    test("destruct struct typealias created via function call", () => {
        var count = 0;
        if(count == 0) {
            var d = create_destructible_alias(&count, 654);
        }
        return count == 1;
    })
    test("destruct struct accessed via function call", () => {
        var count = 0;
        var data = create_destructible(&count, 858).data;
        return count == 1 && data == 858;
    })
    test("destruct struct typealias accessed via function call", () => {
        var count = 0;
        var data = create_destructible_alias(&count, 544).data;
        return count == 1 && data == 544;
    })
    test("destructor is called when access chain is inside a function", () => {
        var count = 0;
        var get_int = (thing : int) => thing;
        var data = get_int(create_destructible(&count, 363).data);
        return count == 1 && data == 363;
    })
    test("destructor is called when access chain typealias is inside a function", () => {
        var count = 0;
        var get_int = (thing : int) => thing;
        var data = get_int(create_destructible_alias(&count, 765).data);
        return count == 1 && data == 765;
    })
    test("destructor is not called on values moved to other var init", () => {
        var count = 0
        if(count == 0) {
            var d = create_destructible(&count, 874)
            var c = d;
        }
        return count == 1
    })
    test("destructor is not called on typealias values moved to other var init", () => {
        var count = 0
        if(count == 0) {
            var d = create_destructible_alias(&count, 874)
            var c = d;
        }
        return count == 1
    })
    test("destructor is not called on values moved to other assignment", () => {
        var count = 0
        if(count == 0) {
            var a = create_destructible(&count, 874)
            var d = create_destructible(&count, 874)
            // here a is destructed, then d is moved into a
            a = d;
            // then here a is destructed
        }
        return count == 2
    })
    test("destructor is not called on typealias values moved to other assignment", () => {
        var count = 0
        if(count == 0) {
            var a = create_destructible_alias(&count, 874)
            var d = create_destructible_alias(&count, 874)
            // here a is destructed, then d is moved into a
            a = d;
            // then here a is destructed
        }
        return count == 2
    })
    test("destructor is not called on values moved to other functions", () => {
        var count = 0
        if(count == 0) {
            var d = create_destructible(&count, 874)
            take_destructible_i(d)
        }
        return count == 1
    })
    test("destructor is not called on typealias values moved to other functions", () => {
        var count = 0
        if(count == 0) {
            var d = create_destructible_alias(&count, 874)
            take_destructible_alias_i(d)
        }
        return count == 1
    })
    test("destructor is not called on values moved to other structs", () => {
        var count = 0
        if(count == 0) {
            var d = create_destructible(&count, 874)
            var h = Holder1 { thing : d }
        }
        return count == 1
    })
    test("destructor is not called on typealias values moved to other structs", () => {
        var count = 0
        if(count == 0) {
            var d = create_destructible_alias(&count, 874)
            var h = Holder2 { thing : d }
        }
        return count == 1
    })
    test("destructor is not called on values moved to other arrays", () => {
        var count = 0
        if(count == 0) {
            var d = create_destructible(&count, 874)
            var h = [ d ]
        }
        return count == 1
    })
    test("destructor is not called on typealias values moved to other arrays", () => {
        var count = 0
        if(count == 0) {
            var d = create_destructible_alias(&count, 874)
            var h = [ d ]
        }
        return count == 1
    })
    test("destructor is not called on values moved to other variants", () => {
        var count = 0
        if(count == 0) {
            var d = create_destructible(&count, 874)
            var h = OptDestructible.Some(d)
        }
        return count == 1
    })
    test("destructor is not called on typealias values moved to other variants", () => {
        var count = 0
        if(count == 0) {
            var d = create_destructible_alias(&count, 874)
            var h = OptDestructible.Some(d)
        }
        return count == 1
    })
    test("destructor is not called on pointer types - 1", () => {
        var count = 0;
        if(count == 0) {
            var d : Destructible
            d.count = &count;
            d.lamb = (count : *mut int) => {
                *count = *count + 1;
            }
            var x : *Destructible
            x = &d;
            var y = &d
        }
        return count == 1;
    })
    test("destructor is not called on pointer types - 2", () => {
        var count = 0;
        if(count == 0) {
            var d : Destructible
            d.count = &count;
            d.lamb = (count : *mut int) => {
                *count = *count + 1;
            }
            const ptr_to_d = test_ret_ptr_to_d(&d)
        }
        return count == 1;
    })
    test("destructor is called on values that are reinitialized using assignment", () => {
        var count = 0
        if(count == 0) {
            var a = create_destructible(&count, 874)
            var d = create_destructible(&count, 874)
            // a is destructed, d is moved into it
            a = d;
            // since d is not initialized, it's not destructed, however now it's initialized
            d = create_destructible(&count, 323)
            // a is destructed and d is destructed
        }
        return count == 3
    })
    test("destructor is called on typealias values that are reinitialized using assignment", () => {
        var count = 0
        if(count == 0) {
            var a = create_destructible_alias(&count, 874)
            var d = create_destructible_alias(&count, 874)
            // a is destructed, d is moved into it
            a = d;
            // since d is not initialized, it's not destructed, however now it's initialized
            d = create_destructible_alias(&count, 323)
            // a is destructed and d is destructed
        }
        return count == 3
    })
    test("array values are destructed", () => {
        var count = 0;
        if(count == 0) {
            var arr : Destructible[10] = [];
            var i = 0;
            var ptr : *mut Destructible;
            while(i < 10) {
                ptr = &arr[i];
                ptr.count = &count;
                ptr.lamb = (count : *mut int) => {
                    *count = *count + 1;
                }
                i++;
            }
        }
        return count == 10;
    })
    test("array typealias values are destructed", () => {
        var count = 0;
        if(count == 0) {
            var arr : DestructibleAlias[10] = [];
            var i = 0;
            var ptr : *mut DestructibleAlias;
            while(i < 10) {
                ptr = &arr[i];
                ptr.count = &count;
                ptr.lamb = (count : *mut int) => {
                    *count = *count + 1;
                }
                i++;
            }
        }
        return count == 10;
    })
    test("array types are destructed", () => {
        var count = 0;
        if(count == 0) {
            var arr : Destructible[10];
            var i = 0;
            var ptr : *mut Destructible;
            while(i < 10) {
                ptr = &arr[i];
                ptr.count = &count;
                ptr.lamb = (count : *mut int) => {
                    *count = *count + 1;
                }
                i++;
            }
        }
        return count == 10;
    })
    test("array typealias types are destructed", () => {
        var count = 0;
        if(count == 0) {
            var arr : DestructibleAlias[10];
            var i = 0;
            var ptr : *mut DestructibleAlias;
            while(i < 10) {
                ptr = &arr[i];
                ptr.count = &count;
                ptr.lamb = (count : *mut int) => {
                    *count = *count + 1;
                }
                i++;
            }
        }
        return count == 10;
    })
    test("destructible struct present inside struct values is destructed", () => {
        var count = 0
        if(count == 0) {
            var holder = Holder1 {
                thing : create_destructible(&count, 332)
            }
        }
        return count == 1
    })
    test("destructible struct present inside struct values typealias is destructed", () => {
        var count = 0
        if(count == 0) {
            var holder = Holder2 {
                thing : create_destructible_alias(&count, 332)
            }
        }
        return count == 1
    })
    test("destructor works, when last if don't return", () => {
        var count = 0
        destructible_but_last_if(&count, 454);
        return count == 1;
    })
    test("destructor works, when last if returns completely", () => {
        var count = 0
        destructible_but_last_if_returns(&count, 655);
        return count == 1;
    })
    test("structs passed to functions as parameters are automatically destructed - 1", () => {
        var count = 0;
        if(count == 0) {
            test_struct_param_destructor(
                Destructible {
                    count : &count,
                    lamb : (count : *mut int) => {
                        *count = *count + 1;
                    },
                    data : 9990
                }
           )
        }
        return count == 1;
    })
    test("structs passed to functions as arguments are automatically destructed - 2", () => {
        var count = 0;
        if(count == 0) {
            test_struct_param_destructor(create_destructible(&count, 223))
        }
        return count == 1;
    })
    test("structs passed to functions as typealias arguments are automatically destructed - 3", () => {
        var count = 0;
        if(count == 0) {
            test_struct_param_destructor(create_destructible_alias(&count, 223))
        }
        return count == 1;
    })
    test("referenced structs are moved into functions as arguments are not destructed twice", () => {
        var count = 0;
        if(count == 0) {
            var d = create_destructible(&count, 223);
            test_struct_param_destructor(d)
        }
        return count == 1;
    })
    test("referenced structs are moved into functions as typealias arguments are not destructed twice", () => {
        var count = 0;
        if(count == 0) {
            var d = create_destructible_alias(&count, 223);
            test_struct_param_destructor(d)
        }
        return count == 1;
    })
    test("struct created in a access chain node, not assigned to a variable is destructed", () => {
        var count = 0;
        if(count == 0) {
            create_destructible(&count, 676)
        }
        return count == 1;
    })
    test("typealias struct created in a access chain node, not assigned to a variable is destructed", () => {
        var count = 0;
        if(count == 0) {
            create_destructible_alias(&count, 676)
        }
        return count == 1;
    })
    test("generic structs destructor is called in var init struct value", () => {
        var count = 0;
        if(count == 0) {
            var d = GenDestruct<int> { data : 454, count : &count, lamb : destruct_inc_count  }
        }
        return count == 1
    })
    test("generic structs destructor is called when created through function call", () => {
        var count = 0;
        if(count == 0) {
            var d = create_long_gen_dest(343, &count, destruct_inc_count);
        }
        return count == 1;
    })
    test("generic struct destructor is called when inside another struct", () => {
        var count = 0;
        if(count == 0) {
            var d = GenDestructOwner {
                d : GenDestruct<long> { data : 454, count : &count, lamb : destruct_inc_count  }
            }
        }
        return count == 1;
    })
    test("generic struct destructor is called when inside array value", () => {
        var count = 0;
        if(count == 0) {
            var d : GenDestruct<int>[1] = [
                GenDestruct<int> { data : 454, count : &count, lamb : destruct_inc_count  }
            ]
        }
        return count == 1;
    })
    test("generic struct destructor is called when passed to a function - 1", () => {
        var count = 0;
        if(count == 0) {
            take_gen_destruct_short(GenDestruct<short> {
                data : 889, count : &count, lamb : destruct_inc_count
            });
        }
        return count == 1;
    })
    test("generic struct destructor is called when passed to a function - 2", () => {
        var count = 0;
        if(count == 0) {
            take_gen_destruct_short(create_short_gen_dest(343, &count, destruct_inc_count));
        }
        return count == 1;
    })
    test("generic struct destructor is called in access chain", () => {
        var count = 0;
        const d = create_long_gen_dest(343, &count, destruct_inc_count).data
        return count == 1;
    })
    test("comptime constructor struct is destructed, when in var init", () => {
        my_string_destr_count = 0;
        if(true) {
            const input_file = my_string("ext/file.c")
        }
        return my_string_destr_count == 1;
    })
    test("comptime constructor struct is destructed, when passing to function call and returning from it", () => {
        my_string_destr_count = 0;
        if(true) {
            const input_file = relative_path(my_string("ext/file.c"))
        }
        return my_string_destr_count == 2;
    })
    test("lambda taking a struct, the struct is destructed", () => {
        var count = 0;
        send_lambda_struct(347, &count, (d) => {})
        return count == 1;
    })
    test("returning struct parameter doesn't destruct it", () => {
        var count = 0;
        const d = test_return_struct_param(Destructible { data : 777, count : &count, lamb : destruct_inc_count });
        return d.data == 777 && count == 0;
    })
    test("lambda doesn't destruct outside it's scope", () => {
        var count = 0;
        var d = Destructible { data : 677, count : &count, lamb : destruct_inc_count }
        var lamb : () => void = () => {
            return;
        }
        var lamb2 : () => void = () => {

        }
        lamb();
        lamb2();
        return count == 0 && d.data == 677;
    })
    test("var init variant call destructs", () => {
        var count = 0;
        var data_usable = false;
        if(count == 0){
            const x = OptDestructible.Some(Destructible {
                data : 892,
                count : &count,
                lamb : (count : *mut int) => {
                    *count = *count + 1;
                }
            })
            switch(x) {
                Some(d) => {
                    data_usable = d.data == 892;
                }
                None => {

                }
            }
        }
        return count == 1 && data_usable;
    })
    test("variants passed to functions as parameters are automatically destructed - 1", () => {
        var count = 0;
        if(count == 0) {
            test_variant_param_destructor(
                OptDestructible.Some(
                    Destructible {
                        count : &count,
                        lamb : (count : *mut int) => {
                            *count = *count + 1;
                        },
                        data : 9990
                    }
                )
           )
        }
        return count == 1;
    })
    test("variants passed to functions as parameters are automatically destructed - 2", () => {
        var count = 0;
        if(count == 0) {
            test_variant_param_destructor(OptDestructible.Some(create_destructible(&count, 223)))
        }
        return count == 1;
    })
    test("returning variant parameter doesn't destruct it", () => {
        var count = 0;
        const x = test_return_variant_param(OptDestructible.Some(Destructible {
            data : 777, count : &count, lamb : destruct_inc_count
        }));
        return count == 0;
    })
    test("variant destruction occurs when allocated using var init", () => {
        var count = 0
        test_variant_destruction_simple(&count);
        return count == 1
    })
    test("variant destruction at early return : true", () => {
         var count = 0;
         test_variant_destruction_at_early_return(&count, true);
         return count == 1;
    })
    test("variant destruction at early return : false", () => {
         var count = 0;
         test_variant_destruction_at_early_return(&count, false);
         return count == 2;
    })
    test("variant conditional destruction : true", () => {
         var count = 0;
         test_variant_conditional_destruction(&count, true);
         return count == 2;
    })
    test("variant conditional destruction : false", () => {
         var count = 0;
         test_variant_conditional_destruction(&count, false);
         return count == 1;
    })
    test("destructible structs passed as references to function calls are destructed - 1", () => {
        var count = 0
        if(count == 0) {
            take_destructible_ref(Destructible {
                data : 777, count : &count, lamb : destruct_inc_count
            })
        }
        return count == 1
    })
    test("destructible structs passed as references to function calls are destructed - 2", () => {
        var count = 0
        if(count == 0) {
            take_destructible_ref(create_destructible(&count, 334))
        }
        return count == 1
    })
    test("generic destructible structs passed as references to function calls are destructed - 1", () => {
        var count = 0
        if(count == 0) {
            take_gen_destruct_ref(GenDestruct<short> {
                data : 777, count : &count, lamb : destruct_inc_count
            })
        }
        return count == 1
    })
    test("generic destructible structs passed as references to function calls are destructed - 2", () => {
        var count = 0
        if(count == 0) {
            take_gen_destruct_ref(create_short_gen_dest(343, &count, destruct_inc_count))
        }
        return count == 1
    })
    test("destructible structs created in parent chain are destructed", () => {
        var count = 0
        if(count == 0) {
            var d = Destructible {
                data : 777, count : &count, lamb : destruct_inc_count
            }
            d.copy().copy().copy()
        }
        return count == 4
    })
    test("destructible structs created in parent chain are destructed - 2", () => {
        var count = 0
        if(count == 0) {
            var d = Destructible {
                data : 739, count : &count, lamb : destruct_inc_count
            }
            var data = d.copy().data
        }
        return count == 2
    })
}