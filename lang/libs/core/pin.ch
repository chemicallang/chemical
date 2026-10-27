// Unpin / Pin (S4.1).
//
// `Unpin` is a safe auto trait derived from the fields; `PhantomPinned` opts
// out with a negative impl. `Pin<T>` wraps a raw pointer whose pointee must not
// move while pinned. `get_mut` is only available for `T: Unpin`;
// `get_unchecked_mut` is unsafe for pinned types.
public namespace core {

    public interface Unpin {}

    public struct PhantomPinned { }

    impl !Unpin for PhantomPinned { }

    public func <T> pin(ptr : &mut T) : Pin<T> where T : Unpin {
        return Pin<T> { ptr : unsafe(ptr as *mut T) }
    }

    public unsafe func <T> pin_unchecked(ptr : *mut T) : Pin<T> {
        return Pin<T> { ptr : ptr }
    }

    @direct_init
    public struct Pin<T> {

        var ptr : *mut T

        public func from_raw(ptr : *mut T) : Pin<T> {
            return Pin<T> { ptr : ptr }
        }

        public func new(ptr : &mut T) : Pin<T> where T : Unpin {
            return Pin<T> { ptr : unsafe(ptr as *mut T) }
        }

        public func get_ref(&self) : &T {
            return unsafe(&*self.ptr)
        }

        public func get_mut(&mut self) : &mut T where T : Unpin {
            return unsafe(&mut *self.ptr)
        }

        public unsafe func get_unchecked_mut(&mut self) : &mut T {
            return unsafe(&mut *self.ptr)
        }

        public func into_inner(self) : *mut T {
            return self.ptr
        }
    }
}
