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

    public func <'a, T> pin(ptr : &'a mut T) : Pin<'a, T> where T : Unpin {
        return Pin<'a, T> {
            ptr : unsafe(ptr as *mut T),
            owner : unsafe(&*ptr)
        }
    }

    public unsafe func <T> pin_unchecked(ptr : *mut T) : Pin<'static, T> {
        var value = Pin<'static, T> {
            ptr : ptr,
            owner : unsafe(&*ptr)
        }
        return value
    }

    @direct_init
    public struct Pin<'a, T> {

        internal var ptr : *mut T
        internal var owner : &'a T

        // A raw pointer does not prove that the pointee is pinned, live or
        // aligned; callers must take an explicit unsafe boundary.
        public unsafe func from_raw(ptr : *mut T) : Pin<'static, T> {
            var value = Pin<'static, T> {
                ptr : ptr,
                owner : unsafe(&*ptr)
            }
            return value
        }

        public func <'b> new(ptr : &'b mut T) : Pin<'b, T> where T : Unpin {
            return Pin<'b, T> {
                ptr : unsafe(ptr as *mut T),
                owner : unsafe(&*ptr)
            }
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
