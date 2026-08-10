/**
 * Runtime ASTAllocator — shared by all runtime parser packages (js, html, css,
 * universal).
 *
 * Implements the compiler `BatchAllocator` static interface so the shared
 * parser packages can allocate their AST nodes at runtime without the
 * compiler. This allocator is written from scratch for the runtime use case:
 * every allocation is tracked, and `deinit()` frees everything at once.
 *
 * Cleanup functions (destructors of allocated structs like HtmlElement) are
 * stored and invoked before the blocks are freed, mirroring the order the
 * compiler's own BatchAllocator uses.
 */
using namespace std;

public struct CleanupEntry {
    var instance_ptr : *mut void
    var cleanup_fn : (obj : *void) => void
}

public struct ASTAllocatorState {
    var blocks : *mut *mut void
    var block_count : size_t
    var block_capacity : size_t

    var cleanups : *mut CleanupEntry
    var cleanup_count : size_t
    var cleanup_capacity : size_t
}

public struct ASTAllocator {
    var state : *mut ASTAllocatorState

    @make
    func make() : ASTAllocator {
        var state = malloc(sizeof(ASTAllocatorState)) as *mut ASTAllocatorState
        new (state) ASTAllocatorState {
            blocks : null,
            block_count : 0,
            block_capacity : 0,
            cleanups : null,
            cleanup_count : 0,
            cleanup_capacity : 0
        }
        return ASTAllocator { state : state }
    }

    // Records a cleanup function to run over an allocated object during
    // deinit(), before the block memory is freed.
    public func store_cleanup(&mut self, obj : *void, cleanup_fn : (obj : *void) => void) {
        if(self.state.cleanup_capacity < self.state.cleanup_count + 1) {
            var new_cap = 8u
            while(new_cap < self.state.cleanup_count + 1) {
                new_cap *= 2
            }
            const new_cleanups = malloc(sizeof(CleanupEntry) * new_cap) as *mut CleanupEntry
            if(self.state.cleanup_count > 0) {
                memcpy(new_cleanups, self.state.cleanups, sizeof(CleanupEntry) * self.state.cleanup_count)
            }
            if(self.state.cleanups != null) {
                free(self.state.cleanups as *mut any)
            }
            self.state.cleanups = new_cleanups
            self.state.cleanup_capacity = new_cap
        }
        self.state.cleanups[self.state.cleanup_count] = CleanupEntry { instance_ptr : obj as *mut void, cleanup_fn : cleanup_fn }
        self.state.cleanup_count += 1
    }

    // Allocates and also records a cleanup fn, so the object's destructor runs
    // during deinit() before its memory is freed.
    public func allocate_with_cleanup(&mut self, obj_size : size_t, alignment : size_t, cleanup_fn : (obj : *void) => void) : *mut void {
        const obj = self.allocate_size(obj_size, alignment) as *mut void
        self.store_cleanup(obj, cleanup_fn)
        return obj
    }

    // Frees every tracked allocation. Cleanup functions run first so
    // destructors can still access the object memory.
    public func deinit(&mut self) {
        var ci = 0u
        while(ci < self.state.cleanup_count) {
            const e = self.state.cleanups[ci]
            e.cleanup_fn(e.instance_ptr)
            ci += 1
        }
        if(self.state.cleanups != null) {
            free(self.state.cleanups as *mut any)
        }
        self.state.cleanups = null
        self.state.cleanup_count = 0
        self.state.cleanup_capacity = 0

        var i = 0u
        while(i < self.state.block_count) {
            free(self.state.blocks[i] as *mut any)
            i += 1
        }
        if(self.state.blocks != null) {
            free(self.state.blocks as *mut any)
        }
        self.state.blocks = null
        self.state.block_count = 0
        self.state.block_capacity = 0
    }

    @delete
    func delete(&mut self) {
        deinit()
        free(self.state as *mut any)
        self.state = null
    }

}

impl BatchAllocator for ASTAllocator {

    func allocate_size(&self, obj_size : size_t, alignment : size_t) : *mut char {
        const ptr = malloc(obj_size) as *mut char
        // track the pointer so deinit() can free it later; the interface
        // method takes &self, but we mutate through the *mut state pointer
        var state = self.state
        if(state.block_capacity < state.block_count + 1) {
            var new_cap = 16u
            while(new_cap < state.block_count + 1) {
                new_cap *= 2
            }
            const new_blocks = malloc(sizeof(*mut void) * new_cap) as *mut *mut void
            if(state.block_count > 0) {
                memcpy(new_blocks, state.blocks, sizeof(*mut void) * state.block_count)
            }
            if(state.blocks != null) {
                free(state.blocks as *mut any)
            }
            state.blocks = new_blocks
            state.block_capacity = new_cap
        }
        state.blocks[state.block_count] = ptr as *mut void
        state.block_count += 1
        return ptr
    }

}
