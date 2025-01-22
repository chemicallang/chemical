struct OtherGen9<T> {
    var value : T
}

func get_other_gen(val : int) : OtherGen9<int> {
    return OtherGen9<int> { value : val }
}

func get_other_gen_value(t : OtherGen9<int>) : int {
    return t.value;
}

variant OtherVar1<T> {
    Some(value : T)
    None()
}

func get_other_var1(value : int, some : bool) : OtherVar1<int> {
    if(some) {
        return OtherVar1.Some(value)
    } else {
        return OtherVar1.None();
    }
}

func get_other_var1_value(other : OtherVar1<int>) : int {
    switch(other) {
        Some(value) => {
            return value;
        }
        None => {
            return -1;
        }
    }
}