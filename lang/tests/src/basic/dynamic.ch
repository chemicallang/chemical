interface Phone {

    func call(&self) : int;

}

struct SmartPhone {
    var number1 : int = 0
    var number2 : int = 0
}

struct CellPhone {
    var number1 : int = 0
    var number2 : int = 0
}

struct PhoneContainer {
    var p : dyn Phone
}

func call_actual(phone : dyn Phone) : int {
    return phone.call();
}

func ret_dyn_obj1(phone : SmartPhone) : dyn Phone {
    return phone;
}

func ret_dyn_obj2(phone : CellPhone) : dyn Phone {
    return phone;
}

impl Phone for SmartPhone {
    func call(&self) : int {
        return number1;
    }
}

impl Phone for CellPhone {
    func call(&self) : int {
        return number2;
    }
}

func test_dynamic_dispatch() {
    test("passing struct as dynamic object in function argument works - 1", () => {
        var p = SmartPhone { number1 : 99 }
        return call_actual(p) == 99;
    })
    test("passing struct as dynamic object in function argument works - 2", () => {
        var p = CellPhone { number2 : 22 }
        return call_actual(p) == 22;
    })
    test("getting dynamic object from function return works - 1", () => {
        var p = SmartPhone { number1 : 88 }
        var o = ret_dyn_obj1(p);
        return o.call() == 88;
    })
    test("getting dynamic object from function return works - 2", () => {
        var p = CellPhone { number2 : 66 }
        var o = ret_dyn_obj2(p);
        return o.call() == 66;
    })
    test("storing struct as dynamic object in var init works - 1", () => {
        var p : dyn Phone = SmartPhone { number1 : 45 }
        return p.call() == 45;
    })
    test("storing struct as dynamic object in var init works - 2", () => {
        var p : dyn Phone = CellPhone { number2 : 35 }
        return p.call() == 35;
    })
    test("storing struct ref as dynamic object in var init works - 1", () => {
        var s = SmartPhone { number1 : 73 };
        var p : dyn Phone = s
        return p.call() == 73;
    })
    test("storing struct ref as dynamic object in var init works - 2", () => {
        var c = CellPhone { number2 : 26 };
        var p : dyn Phone = c
        return p.call() == 26;
    })
    test("assignment to dynamic object using struct works - 1", () => {
        var p : dyn Phone = SmartPhone { number1 : 57 }
        p = CellPhone { number2 : 73 }
        return p.call() == 73;
    })
    test("assignment to dynamic object using struct works - 2", () => {
        var p : dyn Phone = CellPhone { number2 : 35 }
        p = SmartPhone { number1 : 26 }
        return p.call() == 26;
    })
    test("assignment to dynamic object using struct ref works - 3", () => {
        var p : dyn Phone = SmartPhone { number1 : 57 }
        var o = CellPhone { number2 : 73 };
        p = o
        return p.call() == 73;
    })
    test("assignment to dynamic object using struct ref works - 3", () => {
        var p : dyn Phone = CellPhone { number2 : 35 }
        var o = SmartPhone { number1 : 26 };
        p = o
        return p.call() == 26;
    })
    test("storing dynamic object as a struct member works - 2", () => {
        var c = PhoneContainer { p : SmartPhone { number1 : 33 } }
        return c.p.call() == 33;
    })
    test("storing dynamic object as a struct member works - 1", () => {
        var c = PhoneContainer { p : CellPhone { number2 : 55 } }
        return c.p.call() == 55;
    })
    test("storing dynamic object in a array value works - 1", () => {
        var p : dyn Phone[] = [ SmartPhone { number1 : 11 } ]
        return p[0].call() == 11;
    })
    test("storing dynamic object in a array value works - 2", () => {
        var p : dyn Phone[] = [ CellPhone { number2 : 88 } ]
        return p[0].call() == 88;
    })
}