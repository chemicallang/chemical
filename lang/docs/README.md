## Learn Chemical

Chemical is a very easy programming language, It has syntax very similar to 
Typescript

Let's look at a hello world program that prints hello world to console

```chemical
import "@system/stdio.h"

func main() {
    printf("hello world");
}
```

### Functions

A function in chemical is defined by `func` keyword like this

Here's a function that takes two integers and returns the sum
```chemical
func add(a : int, b : int) : int {
    return a + b;
}
```
Very familiar isn't it...

### Variables
Variables are declared using var keyword, similar to typescript

```chemical
func main() {
    var x = 5; // initialize x (int32) with value 5
    x += 2; // add two to x
    printf("number : %d", x); // output -> number : 7
}
```
Another code snippet, that initializes x later

```chemical
func main() {
    var x : int; // declares x
    x = 2; // assigns 2 to x
    printf("number : %d", x); // output -> number : 2
}
```

### Loops in chemical
For loop is similar to C's for loop
```chemical
for(var i = 0; i < 5; i++) {
    printf("%d ", i);
}
// output -> 0 1 2 3 4
```
similarly While Loop
```chemical
var i = 0;
while(i < 5) {
    printf("%d ", i);
    i++;
}
// output -> 0 1 2 3 4
```
similarly Do While Loop
```chemical
var i = 0;
do {
    printf("%d ", i);
    i++;
}while(i < 5);
// output -> 0 1 2 3 4
```
### Switch statement
Chemical supports the C styled switch statement, with braced and non braced blocks
```chemical
switch(x) {
    case 3 :
    printf("x is three");
    break;
    case 4 -> {
        printf("x is 4");
    }
}
```

### Comments in Chemical
```chemical
// this is a single line comment
/**
This is a multiline comment
Yup.
**/
```

### Pointers and References
Similar to C, Chemical offers pointers and references, however these should be used with caution, we will introduce better ways of doing things.
```chemical
func main() {
    var x : int;
    x = 5;
    var y : int* = &x;
    printf("%d", *y); // output -> 5
}
```

### Lambda functions in Chemical
Chemical has support for both capturing and non-capturing lambdas by default
```chemical
var x = () => {
    return 5;
}
printf("%d", x()); // output -> 5
```

### Taking a lambda as a parameter
```chemical
func takes_lambda(lambda : () => int) {
    var x : int = lambda();
    printf("number : %d", x);
}
func main() {
    takes_lambda(() => 5); // output -> number : 5
}
```

However to take a capturing lambda as a parameter, You must use `[]` in the lambda type like
```chemical
func takes_lambda(lambda : []() => int) {
    printf("number : %d", lambda());
}
func main() {
    var x = 5;
    // [x] means capture x by value
    takes_lambda([x]() => x); // output -> number : 5
}
```

### Structs
A struct can hold data of different types
```chemical
struct Data {
    
    int x; // a integer number
    float pi; // a floating pointer number
    
    // &self means self : Data*, which is passed implicitly
    func print(&self) {
        printf("numbers : %d, %f", self.x, self.pi);    
    }
   

}
func main() {
    var d = Data {
        x : 5,
        pi : 3.14f
    }
    d.print();
}
```
### Interfaces
An interface allows you to declare methods
```chemical
interface Data {

    // the implementor doesn't have a self reference
    // Interface can act like a namespace
    func do();

    // must say &self, if the implementor needs a self reference
    func print(&self);
    
    // this function needs do implementation
    // it could be overridden by user, but provides a default implementation
    func needs_do() {
        do();
    }

}

struct MyData : Data {
    func do() {
        // cannot access self here
    }
    func print(&self) {
        // here
    }
}
```
### Implementation
```chemical
interface Dog {
    func bark();
}

impl Dog {
    func bark() {
        printf("barked !");    
    }
}
func main() {
    // You can call bark method directly
    Dog.bark();
}
```
An interface could be defined in a library source and implementation in the application source code.
However, this will not lead to `virtual` method calls