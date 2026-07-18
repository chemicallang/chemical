public namespace std {
public namespace alloc {

@direct_init
public struct Layout {
    var size : size_t
    var align_ : size_t

    @retained
    @constructor
    func make(s : size_t, a : size_t) {
        return Layout {
            size : s,
            align_ : a
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
    var tracker = malloc(sizeof(DebugTracker)) as *mut DebugTracker
    tracker.head = null
    tracker.count = 0
    tracker.total_bytes = 0
    return tracker
}

func debug_tracker_record_alloc(tracker : *mut DebugTracker, ptr : *mut void, size : size_t) {
    var record = malloc(sizeof(AllocRecord)) as *mut AllocRecord
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
            free(curr as *mut void)
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
        free(curr as *mut void)
        curr = next_node
    }
    free(tracker as *mut void)
}

func debug_tracker_print_leaks(tracker : *mut DebugTracker) {
    if(tracker.count > 0) {
        printf("[Allocator] %zu leaks detected (%zu bytes unfreed)\n", tracker.count, tracker.total_bytes)
    } else {
        printf("[Allocator] no leaks detected\n")
    }
}

// GlobalAllocator — wraps malloc/free with optional debug tracking

public struct GlobalAllocator {
    var _debug : *mut void

    @constructor
    func make() {
        var a = GlobalAllocator { _debug : null }
        comptime if(def.debug) {
            a._debug = debug_tracker_create() as *mut void
        }
        return a
    }

    func alloc(&self, layout : Layout) : Result<*mut void, AllocError> {
        var ptr = malloc(layout.size)
        if(ptr == null) {
            return Result.Err(AllocError.OutOfMemory())
        }
        comptime if(def.debug) {
            debug_tracker_record_alloc(self._debug as *mut DebugTracker, ptr, layout.size)
        }
        return Result.Ok(ptr)
    }

    func dealloc(&self, ptr : *mut void, layout : Layout) {
        comptime if(def.debug) {
            debug_tracker_record_dealloc(self._debug as *mut DebugTracker, ptr)
        }
        free(ptr)
    }

    func grow(&self, old : *mut void, old_layout : Layout, new_size : size_t) : Result<*mut void, AllocError> {
        var ptr = realloc(old, new_size)
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
        var ptr = realloc(old, new_size)
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
        memset(ptr, 0, layout.size)
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
    var ptr = malloc(size)
    if(ptr == null) {
        return Result.Err(AllocError.OutOfMemory())
    }
    return Result.Ok(ptr)
}

public func dealloc_bytes(ptr : *mut void) {
    free(ptr)
}

public func alloc_zeroed_bytes(size : size_t) : Result<*mut void, AllocError> {
    var result = alloc_bytes(size)
    if(result is Result.Err) {
        return result
    }
    var Ok(ptr) = result else unreachable
    memset(ptr, 0, size)
    return result
}

}   // <-- alloc end
}   // <-- std end
