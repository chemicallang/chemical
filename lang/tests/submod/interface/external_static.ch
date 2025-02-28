@static
public interface ExternallyImplementedInterface {

    func give_number(&self) : int

}

public func (check : &mut ExternallyImplementedInterface) pls_give_ext_num() : int {
    return check.give_number();
}

@static
public interface ImplementedPublicInterface {

    func give_number(&self) : int

}

public func (c : &mut ImplementedPublicInterface) inc_imp_pub_int() : int {
    return c.give_number() + 1;
}

public struct CurrModPubIntImpl : ImplementedPublicInterface {

    @override
    func give_number(&self) : int {
        return 8787
    }

}