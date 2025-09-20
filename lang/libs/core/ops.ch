public namespace core {
public namespace ops {

// Arithmetic

// +
public interface Add<Output, Rhs = Self> {
    func add(&self, rhs : Rhs) : Output
}

// -
public interface Sub<Output, Rhs = Self> {
    func sub(&self, rhs : Rhs) : Output
}

// *
public interface Mul<Output, Rhs = Self> {
    func mul(&self, rhs : Rhs) : Output
}

// /
public interface Div<Output, Rhs = Self> {
    func div(&self, rhs : Rhs) : Output
}

// %
public interface Rem<Output, Rhs = Self> {
    func rem(&self, rhs : Rhs) : Output
}

// Unary Arithmetic

public interface Neg<Output> {
    func neg(&self) : Output
}

public interface Increment {
    // ++x
    func inc_pre(&mut self) : &Self;
    // x++
    func inc_post(&mut self) : Self;
}

public interface Decrement {
    // --x
    func dec_pre(&mut self) : &Self;
    // x--
    func dec_post(&mut self) : Self;
}

// Compound Assignment

// arithmetic assign
public interface AddAssign<Rhs = Self> {
    func add_assign(&mut self, rhs: Rhs);
}

public interface SubAssign<Rhs = Self> {
    func sub_assign(&mut self, rhs: Rhs);
}

public interface MulAssign<Rhs = Self> {
    func mul_assign(&mut self, rhs: Rhs);
}

public interface DivAssign<Rhs = Self> {
    func div_assign(&mut self, rhs: Rhs);
}

public interface RemAssign<Rhs = Self> {
    func rem_assign(&mut self, rhs: Rhs);
}

// bitwise assign
public interface BitAndAssign<Rhs = Self> {
    func bitand_assign(&mut self, rhs: Rhs);
}

public interface BitOrAssign<Rhs = Self> {
    func bitor_assign(&mut self, rhs: Rhs);
}

public interface BitXorAssign<Rhs = Self> {
    func bitxor_assign(&mut self, rhs: Rhs);
}

// shift assign
public interface ShlAssign<Rhs = Self> {
    func shl_assign(&mut self, rhs: Rhs);
}

public interface ShrAssign<Rhs = Self> {
    func shr_assign(&mut self, rhs: Rhs);
}

// Bitwise (integral only)

public interface BitAnd<Output, Rhs = Self> {
    func bitand(self, rhs: Rhs) : Output;
}

public interface BitOr<Output, Rhs = Self> {
    func bitor(self, rhs: Rhs) : Output;
}

public interface BitXor<Output, Rhs = Self> {
    func bitxor(self, rhs: Rhs) : Output;
}

public interface Not<Output> {
    func not(self) : Output;
}

// Shifts (Rhs typically unsigned/sized integer)

public interface Shl<Output, Rhs> {
    func shl(self, rhs: Rhs) : Output;
}

public interface Shr<Output, Rhs> {
    func shr(self, rhs: Rhs) : Output;
}

// Comparisons & Ordering

public interface PartialEq<Rhs = Self> {
    func eq(&self, other : &Rhs) : bool;
    func ne(&self, other : &Rhs) : bool;
}

public interface Eq : PartialEq {}   // marker for total equality

enum Ordering { Less, Equal, Greater }

// this option exists in this module
// is separate from the std::Option
public variant Option<T> {
    Some(value : T)
    None()
}

public interface PartialOrd<OrderingP = Ordering, Rhs = Self> {
    func partial_cmp(&self, other : &Rhs) : Option<OrderingP>;
}

public interface Ord : PartialOrd {

    func cmp(&self, other : &Self) : Ordering;

    func lt(&self, other : &Self) : bool {
        return cmp(other) == Ordering.Less
    }

    func lte(&self, other : &Self) : bool {
        return cmp(other) in Ordering.Less, Ordering.Equal
    }

    func gt(&self, other : &Self) : bool {
        return cmp(other) == Ordering.Greater
    }

    func gte(&self, other : &Self) : bool {
        return cmp(other) in Ordering.Greater, Ordering.Equal
    }

}

// Indexing

public interface Index<Idx, Output> {
    func index(&self, idx : Idx) : &Output;
}

public interface IndexMut<Idx, Output> : Index<Idx, Output> {
    func index(&mut self, idx : Idx) : &mut Output;
}

/** TODO
// Call (function call operator)

public interface Fn<Args...> {
    func call(&self, args: Args...) -> Output;
}

public interface FnMut<Args...> {
    func call_mut(&mut self, args: Args...) -> Output;
}

public interface FnOnce<Args...> {
    func call_once(self, args: Args...) -> Output;
}
**/

}   // <-- ops end
}  //  <-- core end