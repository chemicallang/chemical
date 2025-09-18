public namespace core {
public namespace ops {

// Arithmetic

// +
interface Add<Output, Rhs = Self> {
    func add(self, rhs : Rhs) : Output
}

// -
interface Sub<Output, Rhs = Self> {
    func sub(self, rhs : Rhs) : Output
}

// *
interface Mul<Output, Rhs = Self> {
    func mul(self, rhs : Rhs) : Output
}

// /
interface Div<Output, Rhs = Self> {
    func div(self, rhs : Rhs) : Output
}

// %
interface Rem<Output, Rhs = Self> {
    func rem(self, rhs : Rhs) : Output
}

// Unary Arithmetic

interface Neg<Output> {
    func neg(self) : Output
}

interface Increment {
    // ++x
    func inc_pre(&mut self) : &Self;
    // x++
    func inc_post(&mut self) : Self;
}

interface Decrement {
    // --x
    func dec_pre(&mut self) : &Self;
    // x--
    func dec_post(&mut self) : Self;
}

// Compound Assignment

interface AddAssign<Rhs = Self> {
    func add_assign(&mut self, rhs: Rhs);
}

interface SubAssign<Rhs = Self> {
    func sub_assign(&mut self, rhs: Rhs);
}

interface MulAssign<Rhs = Self> {
    func mul_assign(&mut self, rhs: Rhs);
}

interface DivAssign<Rhs = Self> {
    func div_assign(&mut self, rhs: Rhs);
}

interface RemAssign<Rhs = Self> {
    func rem_assign(&mut self, rhs: Rhs);
}

// Bitwise (integral only)

interface BitAnd<Output, Rhs = Self> {
    func bitand(self, rhs: Rhs) : Output;
}

interface BitOr<Output, Rhs = Self> {
    func bitor(self, rhs: Rhs) : Output;
}

interface BitXor<Output, Rhs = Self> {
    func bitxor(self, rhs: Rhs) : Output;
}

interface Not<Output> {
    func not(self) : Output;
}

// Shifts (Rhs typically unsigned/sized integer)

interface Shl<Output, Rhs> {
    func shl(self, rhs: Rhs) : Output;
}

interface Shr<Output, Rhs> {
    func shr(self, rhs: Rhs) : Output;
}

// Comparisons & Ordering

interface PartialEq<Rhs = Self> {
    func eq(&self, other : &Rhs) : bool;
    func ne(&self, other : &Rhs) : bool;
}

interface Eq : PartialEq {}   // marker for total equality

enum Ordering { Less, Equal, Greater }

// this option exists in this module
// is separate from the std::Option
public variant Option<T> {
    Some(value : T)
    None()
}

interface PartialOrd<OrderingP = Ordering, Rhs = Self> {
    func partial_cmp(&self, other : &Rhs) : Option<OrderingP>;
}

interface Ord : PartialOrd {

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

interface Index<Idx, Output> {
    func index(&self, idx : Idx) : &Output;
}

interface IndexMut<Idx, Output> : Index<Idx, Output> {
    func index(&mut self, idx : Idx) : &mut Output;
}

/** TODO
// Call (function call operator)

interface Fn<Args...> {
    func call(&self, args: Args...) -> Output;
}

interface FnMut<Args...> {
    func call_mut(&mut self, args: Args...) -> Output;
}

interface FnOnce<Args...> {
    func call_once(self, args: Args...) -> Output;
}
**/

}   // <-- ops end
}  //  <-- core end