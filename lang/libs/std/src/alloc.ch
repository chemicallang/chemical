public namespace std {
public namespace alloc {

public struct Layout {
    // The layout invariant is established by `make`/`of` and validated again
    // before the allocator uses it: a power-of-two non-zero alignment and a
    // size no larger than isize::MAX. Safe code cannot mutate these fields.
    private var _size : size_t
    private var _align : size_t

    @retained
    @constructor
    public func make(s : size_t, a : size_t) {
        comptime if(intrinsics::is_runtime()) {
            if(a == 0) {
                unsafe { __chc_panic("Layout alignment must not be zero", null, 0u, 0u) }
            }
            if((a & (a - 1)) != 0) {
                unsafe { __chc_panic("Layout alignment must be a power of two", null, 0u, 0u) }
            }
            const max_size = ((1 as size_t) << (sizeof(size_t) * 8 - 1)) - 1
            if(s > max_size) {
                unsafe { __chc_panic("Layout size exceeds isize::MAX", null, 0u, 0u) }
            }
        }
        var layout = zeroed:unsafe<Layout>()
        layout._size = s
        layout._align = a
        return layout
    }

    public func size(&self) : size_t {
        return _size
    }

    public func align(&self) : size_t {
        return _align
    }

    public func validate(&self) {
        if(_align == 0 || (_align & (_align - 1)) != 0) {
            panic("invalid Layout alignment")
        }
        const max_size = ((1 as size_t) << (sizeof(size_t) * 8 - 1)) - 1
        if(_size > max_size) {
            panic("invalid Layout size")
        }
    }

    comptime func <T> of() : Layout {
        return Layout.make(sizeof(T), alignof(T))
    }
}

public variant AllocError {
    OutOfMemory()
}

public interface Allocator {

    func alloc(&self, layout : Layout) : Result<*mut void, AllocError>

    func dealloc(&self, ptr : *mut void, layout : Layout)

    func grow(&self, old : *mut void, old_layout : Layout, new_size : size_t) : Result<*mut void, AllocError>

    func shrink(&self, old : *mut void, old_layout : Layout, new_size : size_t) : Result<*mut void, AllocError>

}

// Debug tracking types

struct AllocRecord {
    var ptr : *mut void
    var size : size_t
    var next : *mut AllocRecord
}

struct DebugTracker {
    var head : *mut AllocRecord
    var count : size_t
    var total_bytes : size_t
}

func debug_tracker_create() : *mut DebugTracker {
    var tracker = unsafe(malloc(sizeof(DebugTracker))) as *mut DebugTracker
    tracker.head = null
    tracker.count = 0
    tracker.total_bytes = 0
    return tracker
}

func debug_tracker_record_alloc(tracker : *mut DebugTracker, ptr : *mut void, size : size_t) {
    var record = unsafe(malloc(sizeof(AllocRecord))) as *mut AllocRecord
    record.ptr = ptr
    record.size = size
    record.next = tracker.head
    tracker.head = record
    tracker.count++
    tracker.total_bytes += size
}

func debug_tracker_record_dealloc(tracker : *mut DebugTracker, ptr : *mut void) {
    var prev : *mut AllocRecord = null
    var curr = tracker.head
    while(curr != null) {
        if(curr.ptr == ptr) {
            if(prev != null) {
                prev.next = curr.next
            } else {
                tracker.head = curr.next
            }
            tracker.total_bytes -= curr.size
            tracker.count--
            unsafe { free(curr as *mut void) }
            return
        }
        prev = curr
        curr = curr.next
    }
}

func debug_tracker_destroy(tracker : *mut DebugTracker) {
    var curr = tracker.head
    while(curr != null) {
        var next_node = curr.next
        unsafe { free(curr as *mut void) }
        curr = next_node
    }
    unsafe { free(tracker as *mut void) }
}

func debug_tracker_print_leaks(tracker : *mut DebugTracker) {
    if(tracker.count > 0) {
        unsafe { printf("[Allocator] %zu leaks detected (%zu bytes unfreed)\n", tracker.count, tracker.total_bytes) }
    } else {
        unsafe { printf("[Allocator] no leaks detected\n") }
    }
}

// GlobalAllocator — wraps malloc/free with optional debug tracking

public struct GlobalAllocator {
    private var _debug : *mut void

    @constructor
    func make() {
        var a = GlobalAllocator { _debug : null }
        comptime if(def.debug) {
            a._debug = debug_tracker_create() as *mut void
        }
        return a
    }

    func alloc(&self, layout : Layout) : Result<*mut void, AllocError> {
        layout.validate()
        var ptr = unsafe(malloc(layout.size()))
        if(ptr == null) {
            return Result.Err(AllocError.OutOfMemory())
        }
        comptime if(def.debug) {
            debug_tracker_record_alloc(self._debug as *mut DebugTracker, ptr, layout.size())
        }
        return Result.Ok(ptr)
    }

    func dealloc(&self, ptr : *mut void, layout : Layout) {
        layout.validate()
        comptime if(def.debug) {
            debug_tracker_record_dealloc(self._debug as *mut DebugTracker, ptr)
        }
        unsafe { free(ptr) }
    }

    func grow(&self, old : *mut void, old_layout : Layout, new_size : size_t) : Result<*mut void, AllocError> {
        old_layout.validate()
        var ptr = unsafe(realloc(old, new_size))
        if(ptr == null) {
            return Result.Err(AllocError.OutOfMemory())
        }
        comptime if(def.debug) {
            var tracker = self._debug as *mut DebugTracker
            debug_tracker_record_dealloc(tracker, old)
            debug_tracker_record_alloc(tracker, ptr, new_size)
        }
        return Result.Ok(ptr)
    }

    func shrink(&self, old : *mut void, old_layout : Layout, new_size : size_t) : Result<*mut void, AllocError> {
        old_layout.validate()
        var ptr = unsafe(realloc(old, new_size))
        if(ptr == null) {
            return Result.Err(AllocError.OutOfMemory())
        }
        comptime if(def.debug) {
            var tracker = self._debug as *mut DebugTracker
            debug_tracker_record_dealloc(tracker, old)
            debug_tracker_record_alloc(tracker, ptr, new_size)
        }
        return Result.Ok(ptr)
    }

    func alloc_zeroed(&self, layout : Layout) : Result<*mut void, AllocError> {
        var result = alloc(layout)
        if(result is Result.Err) {
            return result
        }
        var Ok(ptr) = result else unreachable
        unsafe { memset(ptr, 0, layout.size()) }
        return result
    }

    func check_leaks(&self) {
        comptime if(def.debug) {
            debug_tracker_print_leaks(self._debug as *mut DebugTracker)
        }
    }

    @delete
    func destroy(&self) {
        comptime if(def.debug) {
            debug_tracker_destroy(self._debug as *mut DebugTracker)
        }
    }
}

// Convenience free functions (bypass GlobalAllocator)

public func alloc_bytes(size : size_t) : Result<*mut void, AllocError> {
    var ptr = unsafe(malloc(size))
    if(ptr == null) {
        return Result.Err(AllocError.OutOfMemory())
    }
    return Result.Ok(ptr)
}

public func dealloc_bytes(ptr : *mut void) {
    unsafe { free(ptr) }
}

public func alloc_zeroed_bytes(size : size_t) : Result<*mut void, AllocError> {
    var result = alloc_bytes(size)
    if(result is Result.Err) {
        return result
    }
    var Ok(ptr) = result else unreachable
    unsafe { memset(ptr, 0, size) }
    return result
}

}   // <-- alloc end
}   // <-- std end
