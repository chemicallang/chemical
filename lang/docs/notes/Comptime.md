THIS IS A CONCEPT, IT MAY CONTAIN BUGS, IT MAY LITERALLY BE IMPOSSIBLE BECAUSE OF AN
IMPLEMENTATION BARRIER, TREAT IT LIKE AN UNFINISHED PROPOSAL

### Comptime

comptime in chemical has never received any attention, because I was very focused on developing
the language's other features, comptime is not that important if other features are broken, its a
feature that I could live without, But it is becoming increasingly more important as the projects
are becoming larger and complex

Lets address the shortcomings of comptime in chemical.

### If Statements aren't comptime

So when you write a top level if statement, it is compile time, BUT it leaves so much to be desired
lets see, consider the following code

```chemical
if(def.windows) {
    @extern
    public func my_func(); 
} else {
    @test
    func perform_test(env : &mut TestEnv) {
        // ---
        // -----
    }
}
```

This code has many problems, First the if statement takes the `def.windows` as condition, which is evaluated
during symbol resolution, suppose it is true, The first bug would be

1 - The `perform_test` function would still be included in testing because annotation test is processed during parsing

This would crash the compiler, because `perform_test` didn't go through any other phases of the compiler (except parsing)
but `test` annotation still considers it a testing candidate, because testing annotation is processed regardless
of whether `def.windows` is true or not, because during parsing, we do not handle if statements like this.

But we do handle annotations. But lets say we changed the syntax to something parser could understand easily

```chemical
$if(option windows) {
    @extern
    public func my_func();
} else {
    @test
    func perform_test(env : &mut TestEnv) {
        // ---
        // ------
    }
}
```

This code contains a `$` before the if, signaling the Parser that user about to write a if statement that is
compile time only, We could even omit the '$' and Parser would still consider it compile time, because its
a top level if statement, if statements cannot exist at top level unless they are compile time only.

The `option windows` is written like this, so Parser can go, okay an option with name windows must be true
for this branch to be executed, previously we used `def.windows` which requires us to resolve `def`
and then child `windows` inside it, we could use that syntax as long as our parser doesn't hand the
if statement to symbol resolution phase, because it must take one of the branches and discard others
otherwise the annotations would be processed, remember we are doing this at parser level so we can
skip over annotations that aren't required.

Another solution would be to process annotations during symbol resolution, However that is very complicated
and it causes performance to drop, also consumes unnecessary memory. So that solution would not be considered.

Now during parsing when we encounter, we are able to find the exact branch user required and keep the previous syntax

```chemical
// parser automatically considers this $if
// parser automatically resolves `def` and `windows` and computes which branch to take
// the annotations in discarded branches won't be processed

// previous synatx is usable, because def.windows (we know where it is, its a global symbol)
// even function calls to globally accessible functions could be allowed
if(def.windows) {
    @extern
    public func my_func(); 
} else {
    @test
    func perform_test(env : &mut TestEnv) {
        // ---
        // -----
    }
}
```

Now comes the problem that comptime variables declared can't be accessed, Consider this code

```chemical
comptime const is_my_windows = true;
if(is_my_windows) {
    @extern
    public func my_func(); 
} else {
    @test
    func perform_test(env : &mut TestEnv) {
        // ---
        // -----
    }
}
```

This brings us to problem #2

2 - This code would fail, if you insert `$` before if, it would fail even more. During parsing we don't
store symbols and resolve them, The `def` symbol is available everywhere because its a compiler
intrinsic, however `is_my_windows` is only available during symbol resolution phase

We'll let the compiler fail here, So yeah, we are not going to support this feature. Because it requires
that we either shift processing of compile time if statements to symbol resolution phase (store annotations).
Or we store and resolve symbols during parsing, kind of like a preprocessor in C.

So what is the alternative ?, users can always access the compiler global symbols like intrinsics and definitions
on top of that user can provide their own global symbols, functions and definitions using the `build.lab` or `chemical.mod`
We'll support this as time goes on...

The syntax different we must create to understand this

```chemical

// comptime symbol
// comptime means, no runtime footprint
comptime is_my_windows = true

// $ means not comptime, even stronger than comptime
// cannot access comptime stuff
// does not create a scope (where symbols are dropped on end)
$if(def.windows) {
    @extern
    var external_variable : int
} else {
    var y : int = 973
}

// no $, (its a comptime if)
// doesn't support annotations
// supports comptime stuff
// top level `if` doesn't drop new symbols, however local ifs require comptime/inline keyword before them
if(is_my_windows) {
    var internal_variable : int = 33
} else {
    var y : int = 234
}

```