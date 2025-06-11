THIS IS A CONCEPT, IT MAY CONTAIN BUGS, IT MAY LITERALLY BE IMPOSSIBLE BECAUSE OF AN
IMPLEMENTATION BARRIER, TREAT IT LIKE AN UNFINISHED PROPOSAL

### Interfaces, Structs & Implementations

Chemical has same kind of interfaces, structs and implementations syntax as Rust.

But there's a big difference in how chemical's interfaces and structs work. Chemicals interfaces are
static by default. Which means you should expect that no virtual calls will be made.

For now, remember that You can only inherit an interface once.

```
// this interface means someone will implement the sum function once !
// once in the whole code base, multiple implementations of sum cannot exist
interface Adder {
    // sum function doesn't take a pointer to self
    func sum(x : int, y : int) : int;
}

// when someone implements the interface Adder, he will gain access to print function
// which calls the sum function, this can also be written inside the interface
// if print is inside the interface, you could reimplement it
// to prevent from being reimplemented, writing it here is better
func (Adder ar) print(x : int, y : int){
    printf("sum %d = ", ar.sum(x, y));
}

// you can implement a selfless interface like this
impl Adder {
    func sum(x : int, y : int) {
        return x + y;
    };
}

func main() {
    // will sum and print the result 10
    // 1 - this is because it knows the implementation exists above
    //   when you make a call to a function inside a interface
    // 2 - all of its functions must be implemented somewhere that can be found
    Adder.print(5, 5);
}

// this interface means that there can only be one implementation
// of sum function, in the whole codebase, a single sum function implementation
// or store function, a single implementation
// but store function takes a pointer to self
// so this interface can't be 
interface Adder {
    // this cannot call store because that's self dependent function
    // store requires this / self pointer & this function doesn't
    func sum(x : int, y : int) : int;

    // this function will take a pointer to self
    func store(&self, sum : int);
}

@selfless // doesn't take a pointer to self
// can only call functions that are marked selfless
// in release mode, automatically optimize selfless
// if only calls to selfless functions were made
func (Adder ar) print(x : int, y : int){
    printf("sum %d = ", ar.sum(x, y));
}

// function calls store which takes a reference to self
// so this function must also take a reference to self
// to be able to call store
func (Adder ar) sumAndStore(&self, x : int, y : int){
    ar.store(ar.sum(x, y));
}

struct Something {
    var sum : int
}

// will take a pointer to self, since a function on a struct
func (Something s) print() {
    printf("sum %d = ", s.sum);
}

// since Adder is being implemented for a struct
// it will secretly take a pointer to the struct
impl Adder for Something {
    func sum(x : int, y : int) : int {
        return x + y;
    }
    func store(&self, sum : int) {
        slef.sum = sum;
    }
}

func main() {
    // selfless function can be called like this as well
    // if it can find the implementation !, otherwise a compile time error
    var sum = Adder.sum(5, 5);  // 10
    var adder = Something { sum : 0 };
    adder.store(5, 5);
    adder.print(); // prints 10
}
```