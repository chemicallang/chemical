THIS IS A CONCEPT, IT MAY CONTAIN BUGS, IT MAY LITERALLY BE IMPOSSIBLE BECAUSE OF AN
IMPLEMENTATION BARRIER, TREAT IT LIKE AN UNFINISHED PROPOSAL

Here's the syntax for non-capturing lambda, it cannot be assigned a capturing lambda

```
struct Lambda {
    var lamb : () => void
}
```

And here's the syntax for capturing lambda, which supports both capturing and non-capturing lambda assignment

```
struct Lambda {
    var lamb : ||() => void
}
```

Let's now check assignment of capturing lambdas

```
struct Lambda {
    var lamb : ||() => int
}
func test() {
    var x = 32;
    var capture_move_copy = Lambda {
        // you are moving x into the lamb, however its copy, so you can
        // call the lambda twice
        lamb : |x|() => return x;
    }
    var vec = std::vector<int>()
    var capture_move_non_copy = Lambda {
        // you are moving vec into the lamb, so you can only call the lambda once
        lamb : |vec|() => return x;
    }
    var capture_read = Lambda {
        // you are taking a non mutable reference, this lambda
        // can be called twice
        lamb : |&x| () => return x;
    }
    var capture_mutate = Lambda {
        // you are taking a mutable reference, this lambda
        // can be called twice, but no mutable reference to x should exist at call site
        lamb : |&mut x| () => return x;
    }
    var no_capture = Lambda {
        // you are assigning a lambda without capturing any variables
        // you can call this lambda as many times as you want
        lamb : () => return 32;
    }
    // both are called the same way
    capture.lamb()
    no_capture.lamb()
}
```