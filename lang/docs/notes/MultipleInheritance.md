THIS IS A CONCEPT, IT MAY CONTAIN BUGS, IT MAY LITERALLY BE IMPOSSIBLE BECAUSE OF AN
IMPLEMENTATION BARRIER, TREAT IT LIKE AN UNFINISHED PROPOSAL

CURRENTLY THIS DOCUMENT IS EXTREMELY UN PROFESSIONAL

We have a lot of stuff in chemical, that aids the implementation of inheritance and avoiding
virtuality, allowing more and more multiple inheritance without consequences !

This document just discusses one of the problems and explores a potential solution to avoiding
getters in interfaces by redirecting symbols at compile time, It's an unfinished proposal

one of the problems is sometimes we need multiple inheritance, however it's not allowed
however we're not even introducing data which may cause multiple copies of the base class

for example

```cpp
class BaseClass { 
public:
    
    int i;

}

// this class does not introduce any members
// it wants to however introduce functions
interface AnotherClass {
public:
    
    void do_something() {
        
    }
    
}

// however something introduces it's members
// causing the flat structure to be Something { BaseClass { int i }, AnotherClass { }, int x }
class Something : BaseClass, AnotherClass {

    int x;
    
}
```

the problem with the above structure is that AnotherClass doesn't have any members, it just has a 
static like function, which could be anything, Now since the self pointer is being passed to AnotherClass

What we could do is pass the self pointer of Something, which would mean AnotherClass can have access to data
of BaseClass and Something, if it requires

The solution is ofcourse interfaces

```cpp
class BaseClass { 
public:
    
    int i;

}

// this class does not introduce any members
// it wants to however introduce functions, by reusing the data of the below class
// suppose this is an interface
interface AnotherClass {
public:
    
    // this is not a virtual method, suppose it's a forward prototype declaration
    // but this means that the method can only be implemented by a single a struct
    // we have static interfaces in chemical for this purpose, a single implementation
    int please_give_x();
    
    void do_something() {
        int x = please_give_x();
        // I got the x
        // now I can perform the calculation
        // I'm accessing members of Something by making it implement the abstract methods
    }
    
}

// however something introduces it's members
// causing the flat structure to be Something { BaseClass { int i }, AnotherClass { }, int x }
class Something : BaseClass, AnotherClass {

    int x;
    
    int please_give_x() {
       return x; 
    }
    
}
```

Now some people however may say it could be solved like this

```cpp
class BaseClass { 
public:
    
    int i;

}

// this is a normal c++ class now
class AnotherClass : private BaseClass {
public:
    
    int x;
    
    void do_something() {
        // I got the x
        // now I can perform the calculation
        // I'm accessing members of Something by making it implement the abstract methods
    }
    
}

// flat structure is now Something { AnotherClass { BaseClass { int i }, int x } }
class Something : AnotherClass {
    

}
```

This has the disadvantage of AnotherClass depending completely on BaseClass, where changes in BaseClass
impacts the AnotherClass

This has the benefit of directly accessing the int variable AND not having to implement methods, which is
what the purpose of this document is, we don't want to implement the method when it can be avoided

We already have static interfaces, so we aren't worried by avoiding virtuality here, with rust like interfaces
user can also decide between dynamic dispatch and generics

in our case AnotherClass is just an interface, that wants to access the members directly by finalizing the
flat structure of the struct that inherits it

This could be solved by multiple inheritance in c++ like this

```cpp
class BaseClass { 
public:
    
    int i;

}

// normal c++ class
class AnotherClass {
public:
    
    int x;
    
    void do_something() {
        // I got the x
        // I can use it here
    }
    
}

// causing the flat structure to be Something { BaseClass { int i }, AnotherClass { int x } }
class Something : BaseClass, AnotherClass {
    
    

}
```

The problem with this approach is that the pointer passed to do_something in AnotherClass is of
AnotherClass, and it's not a base class, in other words we can't do this NOW

```cpp
class BaseClass { 
public:
    
    int i;

}

// normal c++ class
class AnotherClass {
public:
    
    int x;
    
    int give_i_now();
    
    void do_something() {
        // I got the x
        // let's get i
        int i = give_i_now()
    }
    
}

// causing the flat structure to be Something { BaseClass { int i }, AnotherClass { int x } }
class Something : BaseClass, AnotherClass {
    
    int give_i_now() {
        return i;
    }
    

}
```

this does NOT work because when do_something is called, it get's the pointer to AnotherClass because
it expects the int x to be the first member in the flat structure, if we passed it self (something pointer), which contains base class
in which int i is the first member, it would not be accessing x but i, and now that we use that pointer
to call give_i_now, then in the implementation this passed pointer is being used to access i, however initially we passed
it pointer of AnotherClass and not Something

are you getting this ->

1 - we are passing pointers (self implicitly in c++), when we invoke a non static method on a class
2 - if we see the structure of Something { BaseClass { int i }, AnotherClass { int x } }
  - a pointer to Something contains a 32bit integer at first location named 'i' and then a 32 bit int named 'x'
  - which means a pointer to Something can become a pointer to AnotherClass if we move it 32 bits ahead
  - a pointer to AnotherClass can become a pointer to Something if we move it 32 bits behind
  - however this movement is also not a solution to this problem because AnotherClass 
  - is unaware of what the pointer can contains before it, it is a pointer to Something because that's what implements it
  - it could be a pointer to another implementation as well, which could contain say 64 bit integer before it

which brings us to the problem, NOW I have the solution to this problem, atleast a solution that can work with our chemical system of interfaces
it's non virtual, it allows for multiple implementations, AND it's a work in progress, it can fail at a lot of things

so interfaces already generate declarations and methods for each implementation, what we can do is when generating code for an interface
we can basically declare functions for all implementations, however we implement those methods knowing the implementation, what does this mean

so user access x

```cpp
class BaseClass { 
public:
    
    int i;

}

// normal c++ class
interface AnotherClass {
public:
    
    int x;
    int i;
    
    void do_something() {
        // I can access x here
        // I can access i here too
        // They would access the right variables, even though they are not declared in order
        // interface is incapable of creating implementation variables, these variables must exist
        // in structs that implement the interface
    }
    
}

// causing the flat structure to be Something { BaseClass { int i }, AnotherClass { }, int x }
class Something : BaseClass, AnotherClass {
    
    int x;
    
    // we could have syntax like this, if we wanted the user to realize that AnotherClass requires two variables
    // so it doesn't just implicitly start accessing the variables in base class, because this may cause 
    // alias AnotherClass::x = x; <-- only identifiers are allowed as values
    // alias AnotherClass::i = i; <--- only identifiers are allowed as values
    
    // or you could just write
    // @override
    // int x;

}
```

This means that we ofcourse don't have to provide the implementation of redundant methods that are getters for variables, You may say this introduces some work
of writing override keyword (which we could make implicit by allowing syntax like implicit keyword before inheriting AnotherClass) and has no performance impact since
the final getter methods may still get nuked by the optimizer, however with this solution we won't even force optimizer to do this job, since this is just symbol indirection
we're just saying redirect symbol x of interface to the symbol x in this class before implementing it's methods (this would be really fast)

```cpp
// how fun is this
class Something : BaseClass, implicit AnotherClass {

}
```

and pointer to AnotherClass can dispatch methods using either generics or dynamic dispatch since it's not static, now if we make it static, we can have the same benefits

and more like extension methods on interface, no more generic or dynamic dispatch, direct dispatch as long as we have it's object

so implicitly implementing interfaces like this should require that the variables names are same, otherwise a override specifier would be required
to redirect symbol to another one, which would provide some security to implicitness at compile time from accessing wrong variables

implicitly implementing interfaces is just a convenience or syntactic sugar, which can be replaced as soon as you are ready to write more code
this still can be buggy, explicitness done once saves you from inventing bugs later
so maybe we would require you to be explicit, I don't about how we may implement this

it's like we're providing direct memory to the interface so it can provide us with the services

kotlin abstract classes allow this behavior, where you override a variable in base abstract class, it's similar to that
of-course this is native and performant

We could however also use the impl to implement it better for another struct, this allows us to access
that classes variables, provide a service, without ever touching the struct