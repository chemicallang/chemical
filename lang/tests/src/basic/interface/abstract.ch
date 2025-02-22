@static
interface AbstractRandomNumber {
    func give_number(&self) : int;
}

func (abs : &mut AbstractRandomNumber) ext_abs_rand_num() : int {
    return abs.give_number();
}

@abstract
struct AbstractAbstractRandomNumber : AbstractRandomNumber {

    func give_abstract_number_indirect(&self) : int {
        return give_number() + 2;
    }

}

func (a : &mut AbstractAbstractRandomNumber) abs_str_give_rand_num() : int {
    return a.give_number()
}

func (a : &mut AbstractAbstractRandomNumber) abs_str_give_rand_num_ind() : int {
    return a.give_abstract_number_indirect();
}

struct ImplAbstractRandomNumber : AbstractAbstractRandomNumber {

    @override
    func give_number(&self) : int {
        return 553;
    }

}

func test_abstract_structs() {

    test("method in abstract structs work through implementation struct", () => {
        var abs = ImplAbstractRandomNumber {}
        return abs.give_number() == 553;
    })
    test("method in abstract structs work through abstract struct", () => {
        var abs = ImplAbstractRandomNumber {}
        return abs.give_abstract_number_indirect() == 555;
    })
    test("method in abstract structs work through extension method on abstract struct - 1", () => {
        var abs = ImplAbstractRandomNumber {}
        return abs.abs_str_give_rand_num() == 553;
    })
    test("method in abstract structs work through extension method on abstract struct - 2", () => {
        var abs = ImplAbstractRandomNumber {}
        return abs.abs_str_give_rand_num_ind() == 555;
    })
    test("method in abstract structs work through extension method on interface", () => {
        var abs = ImplAbstractRandomNumber {}
        return abs.ext_abs_rand_num() == 553;
    })

}