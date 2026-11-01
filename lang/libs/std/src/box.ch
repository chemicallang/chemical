// Copyright (c) Chemical Language Foundation 2026.

public namespace std {

/**
 * Unique heap ownership of a `T`.
 *
 * The pointer is private and only created by `box`, so a `Box<T>` always owns
 * exactly one `T`. It is destroyed when the box goes out of scope; `get` and
 * `get_mut` hand out references whose lifetime is bounded by the box. `T` is
 * never copied: construction copies the bytes into fresh storage and then
 * clears the source's drop flag.
 */
@direct_init
public struct Box<T> {

    // The box owns exactly one `T`; the pointer is private so safe code cannot
    // retarget it at another box's storage and cause a double free.
    private var ptr : *mut T

    @delete
    func delete(&mut self) {
        if(self.ptr != null) {
            unsafe {
                delete self.ptr
            }
            self.ptr = null
        }
    }

    public func get(&self) : &T {
        if(self.ptr == null) {
            panic("Box is empty")
        }
        return unsafe(&*self.ptr)
    }

    public func get_mut(&mut self) : &mut T {
        if(self.ptr == null) {
            panic("Box is empty")
        }
        return unsafe(&mut *self.ptr)
    }

    public func as_ptr(&self) : *T {
        return self.ptr
    }

    public func as_mut_ptr(&mut self) : *mut T {
        return self.ptr
    }

    /** Move the value out of the box and free its storage. */
    public func into_inner(&mut self) : T {
        if(self.ptr == null) {
            panic("Box is empty")
        }
        var out = zeroed:unsafe<T>()
        unsafe {
            memcpy(&raw mut out, self.ptr as *mut void, sizeof(T))
            free(self.ptr as *mut void)
        }
        self.ptr = null
        return out
    }
}

public func <T> box(value : T) : Box<T> {
    var ptr = unsafe(malloc(sizeof(T))) as *mut T
    if(ptr == null) {
        panic("box allocation failed")
    }
    unsafe {
        memcpy(ptr as *mut void, &raw value as *void, sizeof(T))
        intrinsics::forget(value)
    }
    return unsafe(box_unchecked<T>(ptr))
}

/**
 * Unsafe constructor for an already-owned, non-null pointer. Safe code uses
 * `box`, which checks the allocation and establishes the ownership invariant.
 */
public unsafe func <T> box_unchecked(ptr : *mut T) : Box<T> {
    return Box<T> {
        ptr : ptr
    }
}

unsafe impl<T> Send for Box<T> where T: Send { }
unsafe impl<T> Sync for Box<T> where T: Sync { }

/**
 * Owning type-erased value built on a borrowed `dyn I` fat pointer.
 *
 * `DynBox<I>` is not the same as `Box<dyn I>`: `Box<dyn I>` owns only the
 * heap-allocated fat-pointer record and borrows the referent. `DynBox<I>`
 * owns the concrete value: the `owner` closure keeps the backing `Box<T>`
 * alive and drops it exactly once when the `DynBox` is destroyed.
 *
 * The `dyn I` view points at heap storage, so its address is stable while the
 * box moves. Construction is a safe audited wrapper around the raw pointer
 * arithmetic; callers never see the erased allocation.
 */
public struct DynBox<I> {
    // Internal by design: construction goes through `dyn_box`, which keeps the
    // fat pointer and its owning allocation in sync.
    var fat : dyn I
    var owner : function<() => void>

    public func get(&self) : &dyn I {
        return &self.fat
    }

    public func get_mut(&mut self) : &mut dyn I {
        return &mut self.fat
    }
}

/**
 * Move `value` into an owning type-erased `DynBox<I>`.
 *
 * The value is allocated once, the fat pointer is built against the heap
 * address, and the owning `Box<T>` is moved into an empty capturing closure.
 * Dropping the closure drops the `Box<T>`, which destroys and frees `value`.
 */
public func <T, I> dyn_box(value : T) : DynBox<I> where T: I {
    var backing = box(value)
    var ptr = backing.as_mut_ptr()
    var erased : dyn I
    unsafe {
        // Hide the local `Box` provenance from the borrow checker: the
        // `owner` closure below keeps the allocation alive for the lifetime of
        // the DynBox, so this view does not borrow the stack local.
        var addr = ptr as usize
        var heap_ptr = addr as *mut T
        erased = dyn<I>(*heap_ptr)
    }
    var owner : function<() => void> = |backing|() => {
    }
    return DynBox<I> { fat : erased, owner : owner }
}

}
