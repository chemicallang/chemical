interface Phone {

    func call(&self) : int;

}

interface BasePhone {
    func base_call(&self) : int
}

interface DerivedPhone : BasePhone {
    func derived_call(&self) : int
}

struct DualPhone {
    var n : int
}

impl BasePhone for DualPhone {
    func base_call(&self) : int { return n }
}

impl DerivedPhone for DualPhone {
    func derived_call(&self) : int { return n + 1 }
}

interface SendPhone {
    func send_call(&self) : int
}

struct SendPhoneImpl {
    var n : int
}

impl SendPhone for SendPhoneImpl {
    func send_call(&self) : int { return n }
}

func <T> dyn_requires_send(value : T) : T where T: Send {
    return value
}

func <T> dyn_requires_sync(value : T) : T where T: Sync {
    return value
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

func call_actual_ref(phone : &dyn Phone) : int {
    return phone.call();
}

func ret_dyn_obj1(phone : &SmartPhone) : dyn Phone {
    return dyn<Phone>(*phone);
}

func ret_dyn_obj2(phone : &CellPhone) : dyn Phone {
    return dyn<Phone>(*phone);
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

// A returned `dyn<I>` must point at caller-owned storage. A by-value parameter
// is a copy inside the callee frame, so this helper takes a reference instead;
// the caller keeps the referent alive for as long as the returned view is used.
interface FrameProbe {

    func value(&self) : int;

}

struct FrameProbeImpl {
    var n : int = 0
}

impl FrameProbe for FrameProbeImpl {
    func value(&self) : int {
        var v : [16]long = zeroed<[16]long>()
        v[0] = 1
        return n + (v[0] as int) - 1;
    }
}

func ret_frame_probe(x : &FrameProbeImpl) : dyn FrameProbe {
    return dyn<FrameProbe>(*x);
}

func test_dynamic_dispatch() {
    test("passing struct as dynamic object in function argument works - 1", () => {
        var p = SmartPhone { number1 : 99 }
        return call_actual(dyn<Phone>(p)) == 99;
    })
    test("passing struct as dynamic object in function argument works - 2", () => {
        var p = CellPhone { number2 : 22 }
        return call_actual(dyn<Phone>(p)) == 22;
    })
    test("dynamic dispatch through a reference to a fat pointer works", () => {
        var p = SmartPhone { number1 : 64 }
        var d : dyn Phone = dyn<Phone>(p)
        return call_actual_ref(&d) == 64;
    })
    test("getting dynamic object from function return works - 1", () => {
        var p = SmartPhone { number1 : 88 }
        var o = ret_dyn_obj1(&p);
        return o.call() == 88;
    })
    test("getting dynamic object from function return works - 2", () => {
        var p = CellPhone { number2 : 66 }
        var o = ret_dyn_obj2(&p);
        return o.call() == 66;
    })
    test("storing struct as dynamic object in var init works - 1", () => {
        var obj = SmartPhone { number1 : 45 }
        var p : dyn Phone = dyn<Phone>(obj)
        return p.call() == 45;
    })
    test("storing struct as dynamic object in var init works - 2", () => {
        var obj = CellPhone { number2 : 35 }
        var p : dyn Phone = dyn<Phone>(obj)
        return p.call() == 35;
    })
    test("storing struct ref as dynamic object in var init works - 1", () => {
        var s = SmartPhone { number1 : 73 };
        var p : dyn Phone = dyn<Phone>(s)
        return p.call() == 73;
    })
    test("storing struct ref as dynamic object in var init works - 2", () => {
        var c = CellPhone { number2 : 26 };
        var p : dyn Phone = dyn<Phone>(c)
        return p.call() == 26;
    })
    test("assignment to dynamic object using struct works - 1", () => {
        var first = SmartPhone { number1 : 57 }
        var p : dyn Phone = dyn<Phone>(first)
        var next = CellPhone { number2 : 73 }
        p = dyn<Phone>(next)
        return p.call() == 73;
    })
    test("assignment to dynamic object using struct works - 2", () => {
        var first = CellPhone { number2 : 35 }
        var p : dyn Phone = dyn<Phone>(first)
        var next = SmartPhone { number1 : 26 }
        p = dyn<Phone>(next)
        return p.call() == 26;
    })
    test("assignment to dynamic object using struct ref works - 3", () => {
        var first = SmartPhone { number1 : 57 }
        var p : dyn Phone = dyn<Phone>(first)
        var o = CellPhone { number2 : 73 };
        p = dyn<Phone>(o)
        return p.call() == 73;
    })
    test("assignment to dynamic object using struct ref works - 3", () => {
        var first = CellPhone { number2 : 35 }
        var p : dyn Phone = dyn<Phone>(first)
        var o = SmartPhone { number1 : 26 };
        p = dyn<Phone>(o)
        return p.call() == 26;
    })
    test("storing dynamic object as a struct member works - 2", () => {
        var obj = SmartPhone { number1 : 33 }
        var c = PhoneContainer { p : dyn<Phone>(obj) }
        return c.p.call() == 33;
    })
    test("storing dynamic object as a struct member works - 1", () => {
        var obj = CellPhone { number2 : 55 }
        var c = PhoneContainer { p : dyn<Phone>(obj) }
        return c.p.call() == 55;
    })
    test("storing dynamic object in a array value works - 1", () => {
        var obj = SmartPhone { number1 : 11 }
        var p : []dyn Phone = [ dyn<Phone>(obj) ]
        return p[0].call() == 11;
    })
    test("storing dynamic object in a array value works - 2", () => {
        var obj = CellPhone { number2 : 88 }
        var p : []dyn Phone = [ dyn<Phone>(obj) ]
        return p[0].call() == 88;
    })
    test("dyn + Send and + Sync bounds construct and satisfy where clauses", () => {
        var p = SendPhoneImpl { n : 12 }
        var d : dyn SendPhone + Send + Sync = dyn<SendPhone + Sync + Send>(p)
        var sent = dyn_requires_send(d)
        var synced = dyn_requires_sync(sent)
        return synced.send_call() == 12
    })
    test("dyn Derived upcasts to dyn Base through a prefix vtable", () => {
        var p = DualPhone { n : 9 }
        var derived : dyn DerivedPhone = dyn<DerivedPhone>(p)
        var base : dyn BasePhone = derived
        return base.base_call() == 9 && derived.derived_call() == 10;
    })
    test("dynamic object built from a by-value parameter survives the callee frame", () => {
        var p = FrameProbeImpl { n : 88 }
        var o = ret_frame_probe(&p)
        return o.value() == 88;
    })
}