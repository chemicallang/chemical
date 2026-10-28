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

}
