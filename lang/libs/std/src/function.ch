public namespace std {

public type destructor_type = (obj : *mut void) => void

public struct default_function_instance {

    // the pointer to function we are going to call
    var fn_pointer : *mut any

    // we store data pointer, so storage can be done on stack or heap
    var fn_data_ptr : *mut any

    // 32 bytes of buffer, 4 pointers
    @maxalign
    var buffer : [32]char

    // a destructor pointer is stored inside the function so we can
    // handle destruction of any variables required
    var dtor : (obj : *mut void) => void;

    // is the captured struct allocated on heap
    var is_heap : bool

    @make
    comptime func make(lambda : () => void) {
        const ptr = intrinsics::get_lambda_fn_ptr(lambda)
        const cap = intrinsics::get_lambda_cap_ptr(lambda)
        const destr = intrinsics::get_lambda_cap_destructor(lambda)
        const size_data = intrinsics::sizeof_lambda_captured(lambda);
        const align_data = intrinsics::alignof_lambda_captured(lambda)
        return intrinsics::wrap(make2(ptr, cap, destr as destructor_type, size_data, align_data))
    }

    @make
    func make2(ptr : *mut void, cap : *mut void, destr : destructor_type, size_data : size_t, align_data : size_t) {
        // we get the captured struct from the lambda
        const captured = cap
        // since lambda has no captured data, we don't need to do anything else
        if(captured == null) {
            return default_function_instance {
                fn_pointer : ptr,
                fn_data_ptr : null,
                is_heap : false,
                dtor : null,
                buffer : []
            }
        }
        // now lets store the data into buffer
        //  because on move the pointer gets invalidated, so we are taking a shortcut and putting
        //  everything on heap until we can get this done
        if(size_data >= 32) {
            // we are going to need to allocate the lambda on heap, its too big
            // TODO: malloc call should take alignment malloc(size_data, align_data)
            var allocated = malloc(size_data) as *mut char
            memcpy(allocated, captured, size_data)
            return default_function_instance {
                fn_pointer : ptr,
                fn_data_ptr : allocated,
                is_heap : true,
                dtor : destr,
                buffer : []
            }
        } else {
            // we are going to allocate it in buffer
            var i = default_function_instance {
                fn_pointer : ptr,
                fn_data_ptr : null,
                is_heap : false,
                dtor : destr,
                buffer : []
            }
            memcpy(&mut i.buffer[0], captured, size_data)
            i.fn_data_ptr = &mut i.buffer[0]
            return i;
        }
    }

    func get_fn_ptr(&self) : *mut any {
        return fn_pointer
    }

    func get_data_ptr(&self) : *mut any {
        if(is_heap) {
            return fn_data_ptr
        } else {
            return (&buffer[0]) as *mut any
        }
    }

    @delete
    func delete(&self) {
        if(dtor != null) {
            dtor(fn_data_ptr)
        }
        if(is_heap) {
            dealloc fn_data_ptr
        }
    }

}

public type function<T, M = default_function_instance> = %capture<T, M>;

}