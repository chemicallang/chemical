// The async/await runtime protocol (design Section 4.2/4.3).
//
// These types are compiler-facing: generated async ramps return a
// `FutureHandle<T>`, and `await` materializes its operand into one. Users never
// name these directly except through the `std::async` re-exports.
public namespace core {
public namespace async {

    // The output of polling a future.
    public variant Poll<T> {
        Ready(value : T)
        Pending()
    }

    // A type-erased wake callback table. `data` is the task handle.
    public struct WakerVTable {
        var wake  : (data : *mut void) => void
        var clone : (data : *mut void) => Waker
        var drop  : (data : *mut void) => void
    }

    // A cloneable, type-erased wake callback. `data` points at the task.
    public struct Waker {
        var data : *mut void
        var vtbl : *WakerVTable

        public func wake(&self) {
            vtbl.wake(data)
        }

        public func clone(&self) : Waker {
            return vtbl.clone(data)
        }

        @delete
        func delete(&mut self) {
            if(vtbl != null) {
                vtbl.drop(data)
            }
        }
    }

    // Passed to every poll; carries the waker for the current task.
    public struct Context {
        var waker : Waker
    }

    // The contract a hand-authored future implements. An `async func` does NOT
    // return this type; its static return type is `FutureHandle<T>` (D14).
    public interface Future<T> {
        func poll(&mut self, cx : *mut Context) : Poll<T>
    }

    // The vtable behind every runtime future handle.
    public struct FutureTable<T> {
        var poll : (frame : *mut void, cx : *mut Context) => Poll<T>
        var drop : (frame : *mut void) => void
    }

    // The concrete, move-only representation of every future. `@delete` is what
    // makes cancellation work: dropping a suspended handle runs the frame's
    // live-local destructors exactly once (Sections 8.5, 13.3).
    @direct_init
    public struct FutureHandle<T> {
        var frame : *mut void
        var vtbl  : *mut FutureTable<T>

        @delete
        func delete(&mut self) {
            if(frame != null) {
                vtbl.drop(frame)
                frame = null
            }
        }
    }

    // A zero-sized "no value" result.
    public struct Unit { }

    // Frame allocation hooks, provided by the runtime library (`lang/libs/async`).
    @extern public func chemical_async_frame_alloc(size : u64, align : u64) : *mut void
    @extern public func chemical_async_frame_free(ptr : *mut void, size : u64, align : u64)

}
}
