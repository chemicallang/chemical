public namespace md {

public struct CleanupEntry {
    var instance_ptr : *mut void
    var cleanup_fn : (obj : *void) => void
}

public struct Arena {
    var blocks : std::vector<*mut void>
    var cleanup_fns : std::vector<CleanupEntry>

    @make
    func make() {
        blocks = std::vector<*mut void>()
        cleanup_fns = std::vector<CleanupEntry>()
    }

    func allocate_size(&mut self, size : size_t, align : size_t) : *mut void {
        var p = malloc(size)
        if(p != null) {
            self.blocks.push(p)
        }
        return p
    }

    public func store_cleanup(&mut self, obj : *void, cleanup_fn : (obj : *void) => void) {
        self.cleanup_fns.push(CleanupEntry { instance_ptr : obj as *mut void, cleanup_fn : cleanup_fn })
    }

    public func allocate_with_cleanup(&mut self, obj_size : size_t, alignment : size_t, cleanup_fn : (obj : *void) => void) : *mut void {
        var obj = allocate_size(obj_size, alignment);
        store_cleanup(obj, cleanup_fn);
        return obj;
    }

    func deinit(&mut self) {
        // Run cleanup functions first (so destructors can still access object memory)
        var ci = 0u;
        while(ci < self.cleanup_fns.size()) {
            var e = self.cleanup_fns.get(ci);
            e.cleanup_fn(e.instance_ptr);
            ci++;
        }
        self.cleanup_fns.clear()

        var i = 0u;
        while(i < self.blocks.size()) {
            free(self.blocks.get(i) as *mut any)
            i++;
        }
        self.blocks.clear()
    }

    @delete
    func delete(&mut self) {
        deinit();
    }

}

}

public comptime func <T> (builder : &mut md::Arena) allocate() : *mut T {
    var delete_fn = intrinsics::get_child_fn<T>("delete") as (obj : *void) => void;
    if(delete_fn != null) {
        return intrinsics::wrap(builder.allocate_with_cleanup(sizeof(T), alignof(T), delete_fn)) as *mut T
    } else {
        return intrinsics::wrap(builder.allocate_size(sizeof(T), alignof(T))) as *mut T
    }
}