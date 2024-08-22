struct OtherGen9<T> {
    var value : T
}

func get_other_gen(val : int) : OtherGen9<int> {
    return OtherGen9<int> { value : val }
}

func get_other_gen_value(t : OtherGen9<int>) : int {
    return t.value;
}