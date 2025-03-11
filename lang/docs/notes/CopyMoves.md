The copy annotation can be used to make any struct copyable, when it's put into another variable
or just passed into function calls, a shallow copy of the struct would be made

```chemical
@copy
struct Copyable {
    var a : int
    var b : int
}
```

RULE: structs marked with copy annotation cannot contain a field that's a struct / variant and doesn't have the copy annotation

In this case struct is copied in these scenarios

1 - When passing to a function call, that takes the struct directly

```chemical
    func takes_copyable(c : Copyable) {
    
    }
    func send_it() {
        var c = Copyable { i : 10, b : 20 };
        // a copy of 'c' will be sent to the function
        // the newly created shallow copied struct
        takes_copyable(c)
        takes_copyable(Copyable { i : 10, b : 20 }) // <-- no copy is created
        takes_copyable(create_the_struct()) // <-- no copy is created
    }
```

2 - When saving into a struct or array or a location that has already allocated memory

```chemical
    func send_it() {
        var c = Copyable { i : 10, b : 20 };
        var s : struct { c : Copyable } = struct { c : c }
        // here we save into array, similarly a copy is made
        var arr = { c }
        // here we save into variant call, similar things apply
        var opt = Option.Some(c)
        // first the memory for 'another' is allocated, then c is shallow copied into it
        var another : Copyable = c
        // when assigning to 'another' variable, we do not need to allocate any memory
        // we shallow copy 'c' into 'another'
        another = c;
    }
```

Now let's check how moves work in chemical


interface Clone {

    func clone(&self) : Self;

    func clone_from(&mut self, source: &Self) {
        self = source.clone();
    }

}

```chemical
struct Movable : Clone {

    var a : int
    var b : int
    
    // the clone annotation means, auto clone the members, when you use this annotation, you can't provide a body for the function
    // because compiler generates a default clone implementation
    @override
    @clone
    func clone(&self) : Self;
    
}
```

This above struct cannot be copied, once you pass it into another function call, it means you cannot use the
moved variable, for example

```chemical
    func take_movable(m : Movable) {
        // movable will be dropped / destructed after this function
    }
    func send_movable() {
        var m = Movable { a : 10, b : 20 }
        take_movable(m)
        // ERROR: cannot access m.a
        printf("%d", m.a) // <--- we will not allow accessing 'm' after it has been moved (compiler error)
    }
```

The solutions to these problems are

1 - You can call clone function and create a clone when desired
  - since you don't have copy annotation on struct, it won't be copied everywhere automatically

```chemical
    func take_movable(m : Movable) {
        // movable will be dropped / destructed after this function
    }
    func send_movable() {
        var m = Movable { a : 10, b : 20 }
        take_movable(m.clone())
        // you can access the 'm' here
        printf("%d", m.a)
    }
```

2 - You can take a mutable or immutable reference to struct

Similarly moves take place at these locations

1 - When passing to a function call, that takes the struct directly

```chemical
    func take_movable(m : Movable) {
        // movable will be dropped / destructed after this function
    }
    func send_movable() {
        var m = Movable { a : 10, b : 20 }
        take_movable(m)
        // 'm' has been moves and cannot be accessed after this
        // 'm' won't be destructed here, since take_movable takes ownership and responsibility of destruction
    }
```

2 - When saving into a struct or array or a location that has already allocated memory

```chemical
    func send_it() {
        var m = Movable { a : 10, b : 20 }
        // here 'm' would be moved into the struct and would be unusable
        var s : struct { m : Movable } = struct { m : m }
        // here we save into array, similarly a move is made and 'm' will unusable after this
        var arr = { m }
        // here we save into variant call and 'm' would become unusable after this
        var opt = Option.Some(m)
        // here 'm' is being moved into a variable initialization
        var another : Movable = m
        // when assigning to 'another' variable, here as well 'm' is being moved and would become unusable after this
        another = m;
    }
```