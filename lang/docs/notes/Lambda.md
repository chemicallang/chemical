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
    // this is basically a fat pointer
    var lamb : ||() => void
}
```

Let's now check assignment of capturing lambdas

```
struct Lambda {
    var lamb : () => int
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

Above examples capturing lambda with its allocated data lived on the stack, since the lambda
is not going to be accessible after the function ends, so there's no point in allocating lambda
on the heap.

To be able to store and manage allocation and de-allocation of capturing lambdas, we need to write
a struct, called lambda provider, this struct is allocated, constructed and dropped in a 
capturing lambda lifecycle when user requires. 

```
@lambda.instance
struct function_instance {
    
    // every capturing lambda is a fat pointer
    var fn_pointer : *void

    // we store data pointer, so storage can be done on stack or heap    
    var fn_data_ptr : *void
    
    // 32 bytes of buffer, 4 pointers
    @maxalign
    var buffer : char[32]
    
    // a destructor pointer is stored inside the function so we can
    // handle destruction of any variables required
    var dtor : (obj : *void) => void;
   
    // is the captured struct allocated on heap 
    var is_heap : bool
    
    @make
    func make(lambda : () => void) {
        // copy the fat pointer
        fn_pointer = compiler::get_lambda_fn(lambda);
        // we get the captured struct from the lambda
        const captured = intrinsics::get_lambda_captured(lambda)
        // since lambda has no captured data, we don't need to do anything else
        if(captured == null) {
            fn_data_ptr = null;
            is_heap = false;
            dtor = null;
            return;
        }
        // set the destructor function for the given lambda
        dtor = intrinsics::get_lambda_captured_destructor(lambda)
        // now lets store the data into buffer
        const size_data = intrinsics::sizeof_lambda_captured(lambda);
        const align_data = intrinsics::alignof_lambda_captured(lambda)
        if(size_data >= 32) {
            // we are going to need to allocate the lambda on heap, its too big
            var allocated = malloc(size_data, align_data) as *mut char
            memcpy(allocated, captured, size_data)
            fn_data_ptr = allocated
            is_heap = true;
        } else {
            // we are going to allocate it in buffer
            var dest = &buffer[0]
            memcpy(dest, captured, size_data)
            fn_data_ptr = dest
            is_heap = false;
        }
    }
    
    @inline
    func get_fn_ptr(&self) : *void {
        return fn_pointer;
    }
    
    @inline
    func get_fn_data_ptr(&self) : *void {
        return fn_data_ptr;   
    }
    
    @delete
    func delete(&self) {
        if(dtor != null) {
            dtor(fn_data_ptr)
        }
        if(is_heap) {
            free(fn_data_ptr)
        }
    }
    
}

type function<T, M = function_instance> = #lambda<T, M>;

```