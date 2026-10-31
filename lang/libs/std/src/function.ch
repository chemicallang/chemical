public namespace std {

public type destructor_type = (obj : *mut void) => void

@retained
public struct default_function_instance {

    // The erased function/data pair is an unchecked runtime contract; safe
    // code must not forge one. Construction goes through `make`/`new` below.
    private var fn_pointer : *mut any

    // we store data pointer, so storage can be done on stack or heap
    private var fn_data_ptr : *mut any

    // 32 bytes of buffer, 4 pointers. Kept for layout compatibility; captures
    // are always heap-indirect now (see make2), so this buffer is no longer a
    // source of an internal pointer.
    @maxalign
    private var buffer : [32]char

    // a destructor pointer is stored inside the function so we can
    // handle destruction of any variables required
    private var dtor : (obj : *mut void) => void;

    // is the captured struct allocated on heap
    private var is_heap : bool

    // The allocation returned by malloc. `fn_data_ptr` is aligned inside it,
    // so this may differ from `fn_data_ptr` for an over-aligned capture.
    private var alloc_base : *mut void

    // TODO: raw capturing function type should be developed
    //  which can only take literal capturing functions
    //  and here we'd use a type that takes literal capturing + non capturing lambdas
    //  this should reject any runtime lambdas
    @make
    @expand
    comptime func make(lambda : () => void) {
        const ptr = intrinsics::get_lambda_fn_ptr(lambda)
        const cap = intrinsics::get_lambda_cap_ptr(lambda)
        const destr = intrinsics::get_lambda_cap_destructor(lambda)
        const size_data = intrinsics::sizeof_lambda_captured(lambda);
        const align_data = intrinsics::alignof_lambda_captured(lambda)
        return unsafe(make2(ptr, cap, destr as destructor_type, size_data, align_data))
    }

    // Builds a function object from raw pieces: an arbitrary code pointer, a
    // possibly-unowned capture pointer, and a destructor that will be invoked
    // on it. Every part of the invariant is unchecked, so this is unsafe and
    // only the audited `make` path above may call it.
    @make
    unsafe func make2(ptr : *mut void, cap : *mut void, destr : destructor_type, size_data : size_t, align_data : size_t) {
        // We get the captured struct from the lambda
        const captured = cap
        // A non-capturing lambda needs no allocation.
        if(captured == null) {
            return default_function_instance {
                fn_pointer : ptr,
                fn_data_ptr : null,
                is_heap : false,
                dtor : null,
                buffer : [],
                alloc_base : null
            }
        }

        // Captures are always heap-indirect. Storing the capture in the inline
        // buffer makes fn_data_ptr point into this value; returning/moving the
        // struct then leaves that pointer dangling. The heap copy also lets us
        // honour align_data, which malloc alone does not.
        var align = align_data
        if(align == 0) {
            align = 1
        }
        if((align & (align - 1)) != 0) {
            panic("function capture alignment must be a power of two")
        }
        var total : size_t = size_data
        if(total == 0) {
            total = 1
        } else {
            total = total + align - 1
        }
        var base = unsafe(malloc(total)) as *mut char
        if(base == null) {
            panic("function capture allocation failed")
        }
        const raw = base as usize
        const aligned = (raw + align - 1) & ~(align - 1)
        var allocated = aligned as *mut char
        if(size_data > 0) {
            unsafe { memcpy(allocated as *mut void, captured, size_data) }
        }
        return default_function_instance {
            fn_pointer : ptr,
            fn_data_ptr : allocated as *mut any,
            is_heap : true,
            dtor : destr,
            buffer : [],
            alloc_base : base as *mut void
        }
    }

    func get_fn_ptr(&self) : *mut any {
        return fn_pointer
    }

    func get_data_ptr(&self) : *mut any {
        return fn_data_ptr
    }

    @delete
    func delete(&self) {
        unsafe {
            if(dtor != null) {
                dtor(get_data_ptr())
            }
            if(alloc_base != null) {
                dealloc alloc_base
            }

        }
    }

}

@partial_instantiate
public type function<T> = %capture<T, default_function_instance>;

/**
 * A callback carrying an explicit capture-environment witness. The witness is
 * type-level only (the runtime layout is still the function instance); it lets
 * Send/Sync/'static bounds inspect the actual captured fields. Safe thread and
 * scoped APIs accept this type and infer `C` from the lambda argument.
 */
@partial_instantiate
public type typed_function<T, C> = %capture<T, default_function_instance, C>;

/**
 * Type-level marker for a callback whose capture environment was proven
 * `Send + 'static` when it was created. Unlike a bare `std::function<F>`
 * parameter, a `send_function<F>` parameter keeps that guarantee at every
 * later use, so a non-generic safe API can accept it and hand it to a thread
 * without naming the concrete capture type.
 *
 * The marker is empty and carries no runtime data; verification happens when
 * the lambda (or a typed callback) is assigned to the parameter.
 */
public struct SendStaticCapture { }

// The marker proves `Send`, not `Sync`: a closure environment may contain
// interior mutability that is safe to move but not to share. Keeping the
// marker `!Sync` keeps `send_function` !Sync unless a future marker proves
// both properties.
impl !Sync for SendStaticCapture { }

@partial_instantiate
public type send_function<T> = %capture<T, default_function_instance, SendStaticCapture>;

/**
 * Type-level marker for a callback whose capture environment was proven
 * `Send` but may borrow caller storage. It is deliberately not `Send` itself:
 * it may only be handed to a join-before-return scoped API, never to an
 * unscoped thread submission that could outlive the borrowed values.
 */
public struct ScopedSendCapture { }
impl !Send for ScopedSendCapture { }
impl !Sync for ScopedSendCapture { }

@partial_instantiate
public type scoped_send_function<T> = %capture<T, default_function_instance, ScopedSendCapture>;

}