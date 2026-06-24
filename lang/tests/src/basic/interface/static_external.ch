func (thing : &mut ExternallyImplementedInterface) get_interface_num_in_curr_mod() : int {
    return thing.give_number();
}

struct RightExternalNumber {

}

impl ExternallyImplementedInterface for RightExternalNumber {
    func give_number(&self) : int {
        return 8765;
    }
}

func test_static_external() {
    test("externally implemented imported interface works", () => {
        var thing = CurrModPubIntImpl {}
        return thing.give_number() == 8787
    })

    test("externally implemented imported interface works through extension method", () => {
        var thing = CurrModPubIntImpl {}
        return (thing as ImplementedPublicInterface).inc_imp_pub_int() == 8788
    })

    test("external interfaces implemented in current module work", () => {
        var thing = RightExternalNumber {  }
        return thing.give_number() == 8765
    })

    test("external interfaces implemented in current module work", () => {
        var thing = RightExternalNumber {  }
        return thing.give_number() == 8765
    })

    test("external interfaces implemented in current module work through extension method in current module", () => {
        var thing = RightExternalNumber {  }
        return (thing as ExternallyImplementedInterface).get_interface_num_in_curr_mod() == 8765
    })

    test("external interfaces implemented in current module work through extension method in external module", () => {
        var thing = RightExternalNumber {  }
        return (thing as ExternallyImplementedInterface).pls_give_ext_num() == 8765
    })
}