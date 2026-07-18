using namespace std;

func test_allocator() {

    test("Layout.of<int> has correct size", () => {
        var layout = std::alloc::Layout.of<int>()
        return layout.size == sizeof(int)
    })

    test("Layout.of<double> has correct size", () => {
        var layout = std::alloc::Layout.of<double>()
        return layout.size == sizeof(double)
    })

    test("Layout has correct alignment", () => {
        var layout = std::alloc::Layout.of<int>()
        return layout.align_ == alignof(int)
    })

    test("GlobalAllocator can alloc and dealloc", () => {
        var a = std::alloc::GlobalAllocator.make()
        var layout = std::alloc::Layout.of<int>()
        var result = a.alloc(layout)
        if(result is Result.Err) {
            return false
        }
        var Ok(ptr) = result else unreachable
        a.dealloc(ptr, layout)
        return true
    })

    test("GlobalAllocator alloc returns non-null pointer", () => {
        var a = std::alloc::GlobalAllocator.make()
        var layout = std::alloc::Layout.of<int>()
        var result = a.alloc(layout)
        if(result is Result.Err) {
            return false
        }
        var Ok(ptr) = result else unreachable
        a.dealloc(ptr, layout)
        return ptr != null
    })

    test("GlobalAllocator alloc_zeroed returns zeroed memory", () => {
        var a = std::alloc::GlobalAllocator.make()
        var layout = std::alloc::Layout.of<long>()
        var result = a.alloc_zeroed(layout)
        if(result is Result.Err) {
            return false
        }
        var Ok(ptr) = result else unreachable
        var typed = ptr as *mut long
        var val = typed[0]
        a.dealloc(ptr, layout)
        return val == 0
    })

    test("GlobalAllocator grow preserves old data", () => {
        var a = std::alloc::GlobalAllocator.make()
        var small_layout = std::alloc::Layout.of<int>()
        var result = a.alloc(small_layout)
        if(result is Result.Err) {
            return false
        }
        var Ok(old_ptr) = result else unreachable
        var typed = old_ptr as *mut int
        typed[0] = 42

        var big_layout = std::alloc::Layout { size : sizeof(int) * 2, align_ : alignof(int) }
        var grow_result = a.grow(old_ptr, small_layout, big_layout.size)
        if(grow_result is Result.Err) {
            a.dealloc(old_ptr, small_layout)
            return false
        }
        var Ok(new_ptr) = grow_result else unreachable
        var new_typed = new_ptr as *mut int
        var old_val = new_typed[0]
        a.dealloc(new_ptr, big_layout)
        return old_val == 42
    })

    test("GlobalAllocator shrink works", () => {
        var a = std::alloc::GlobalAllocator.make()
        var big_layout = std::alloc::Layout { size : sizeof(int) * 4, align_ : alignof(int) }
        var result = a.alloc(big_layout)
        if(result is Result.Err) {
            return false
        }
        var Ok(old_ptr) = result else unreachable
        var typed = old_ptr as *mut int
        typed[0] = 99

        var small_layout = std::alloc::Layout { size : sizeof(int) * 2, align_ : alignof(int) }
        var shrink_result = a.shrink(old_ptr, big_layout, small_layout.size)
        if(shrink_result is Result.Err) {
            a.dealloc(old_ptr, big_layout)
            return false
        }
        var Ok(new_ptr) = shrink_result else unreachable
        var new_typed = new_ptr as *mut int
        var val = new_typed[0]
        a.dealloc(new_ptr, small_layout)
        return val == 99
    })

    test("GlobalAllocator check_leaks reports no leaks after clean dealloc", () => {
        var a = std::alloc::GlobalAllocator.make()
        var layout = std::alloc::Layout.of<int>()
        var result = a.alloc(layout)
        var Ok(ptr) = result else unreachable
        a.dealloc(ptr, layout)
        a.check_leaks()
        return true
    })

    test("alloc_bytes convenience function works", () => {
        var result = std::alloc::alloc_bytes(sizeof(int))
        if(result is Result.Err) {
            return false
        }
        var Ok(ptr) = result else unreachable
        std::alloc::dealloc_bytes(ptr)
        return ptr != null
    })

    test("alloc_zeroed_bytes convenience function returns zeroed memory", () => {
        var result = std::alloc::alloc_zeroed_bytes(sizeof(long))
        if(result is Result.Err) {
            return false
        }
        var Ok(ptr) = result else unreachable
        var typed = ptr as *mut long
        var val = typed[0]
        std::alloc::dealloc_bytes(ptr)
        return val == 0
    })

}
