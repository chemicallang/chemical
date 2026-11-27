// P1.2 generational node arena and per-frame bump arena. Internal machinery;
// the view tree/reconciler start in P1.3/P1.4.
using std::Option;
using std::Result;
using std::Unit;
using std::vector;

const IDX_NONE : u32 = 0xFFFFFFFFu32
const GEN_MAX : u32 = 0xFFFFFFFFu32

@direct_init
public struct NodeId {
    private var index_value : u32
    private var generation_value : u32

    func make_internal(index : u32, generation : u32) : NodeId {
        return NodeId { index_value : index, generation_value : generation }
    }
    public func index(&self) : u32 { return self.index_value }
    public func generation(&self) : u32 { return self.generation_value }
    public func is_valid(&self) : bool {
        return self.index_value != 0u32 && self.generation_value != 0u32
    }
}

public namespace ui_internal {

@direct_init
public struct ArenaSlot {
    var generation : u32
    var occupied : bool
}

public struct NodeArena {
    var slots : vector<ArenaSlot>
    var free : vector<u32>
    var max_nodes : u32
    var live : u32
    var free_len : u32

    public func new(max_nodes : u32) : NodeArena {
        var a = NodeArena {
            slots : vector<ArenaSlot>(),
            free : vector<u32>(),
            max_nodes : max_nodes,
            live : 0u32,
            free_len : 0u32
        }
        var cap = max_nodes as size_t
        a.slots.reserve(cap + 1u)
        a.free.reserve(cap)
        // Index 0 is the invalid sentinel and is never issued.
        a.slots.push(ArenaSlot { generation : 0u32, occupied : false })
        return a
    }

    public func reserve(&mut self, additional : u32) : Result<Unit, UiError> {
        if(additional > self.max_nodes) {
            return Result.Err<Unit, UiError>(UiError.make(ErrorCode.ResourceExhausted, "node arena reserve exceeds capacity"))
        }
        var cap = self.max_nodes as size_t
        self.slots.reserve(cap + 1u)
        self.free.reserve(cap)
        return Result.Ok<Unit, UiError>(Unit{})
    }

    public func capacity(&self) : u32 { return self.max_nodes }
    public func len(&self) : u32 { return self.live }
    public func free_count(&self) : u32 { return self.free_len }

    func slot_mut(&mut self, index : u32) : *mut ArenaSlot {
        if(index as size_t >= self.slots.size()) { return null }
        return self.slots.get_ptr(index as size_t)
    }

    @inline
    @inline
    public func insert_fast(&mut self) : NodeId {
        if(self.free_len > 0u32) {
            var reused = self.free.pop()
            if(reused is Option.Some) {
                self.free_len = self.free_len - 1u32
                var Some(idx) = reused else return zeroed<NodeId>()
                var slot = self.slot_mut(idx)
                if(slot != null) {
                    unsafe {
                        if(slot.generation == GEN_MAX) { return zeroed<NodeId>() }
                        slot.generation = slot.generation + 1u32
                        slot.occupied = true
                        self.live = self.live + 1u32
                        return NodeId.make_internal(idx, slot.generation)
                    }
                }
            }
            return zeroed<NodeId>()
        }
        if((self.slots.size() as u32) - 1u32 >= self.max_nodes) { return zeroed<NodeId>() }
        var idx = self.slots.size() as u32
        self.slots.push(ArenaSlot { generation : 1u32, occupied : true })
        self.live = self.live + 1u32
        return NodeId.make_internal(idx, 1u32)
    }

    public func insert(&mut self) : Result<NodeId, UiError> {
        var reused = Option.None<u32>()
        if(self.free_len > 0u32) {
            reused = self.free.pop()
            if(reused is Option.Some) { self.free_len = self.free_len - 1u32 }
        }
        if(reused is Option.Some) {
            var Some(idx) = reused else return Result.Err<NodeId, UiError>(UiError.make(ErrorCode.Internal, "arena free-list corrupt"))
            var slot = self.slot_mut(idx)
            if(slot != null) {
                unsafe {
                    if(slot.generation == GEN_MAX) {
                        return Result.Err<NodeId, UiError>(UiError.make(ErrorCode.ResourceExhausted, "node generation exhausted"))
                    }
                    slot.generation = slot.generation + 1u32
                    slot.occupied = true
                    self.live = self.live + 1u32
                    return Result.Ok<NodeId, UiError>(NodeId.make_internal(idx, slot.generation))
                }
            }
            return Result.Err<NodeId, UiError>(UiError.make(ErrorCode.Internal, "arena free-list stale"))
        }
        if((self.slots.size() as u32) - 1u32 >= self.max_nodes) {
            return Result.Err<NodeId, UiError>(UiError.make(ErrorCode.ResourceExhausted, "node arena is full"))
        }
        var idx = self.slots.size() as u32
        self.slots.push(ArenaSlot { generation : 1u32, occupied : true })
        self.live = self.live + 1u32
        return Result.Ok<NodeId, UiError>(NodeId.make_internal(idx, 1u32))
    }

    public func get(&self, id : NodeId) : Option<u32> {
        if(!id.is_valid()) { return Option.None<u32>() }
        var idx = id.index()
        if(idx as size_t >= self.slots.size()) { return Option.None<u32>() }
        var got = self.slots.get(idx as size_t)
        if(got is Option.Some) {
            var Some(slot) = got else return Option.None<u32>()
            if(slot.occupied && slot.generation == id.generation()) { return Option.Some<u32>(idx) }
        }
        return Option.None<u32>()
    }

    public func contains(&self, id : NodeId) : bool { return self.get(id) is Option.Some }

    public func remove(&mut self, id : NodeId) : Result<Unit, UiError> {
        if(!self.contains(id)) {
            return Result.Err<Unit, UiError>(UiError.make(ErrorCode.StaleNode, "stale or forged node id"))
        }
        var idx = id.index()
        var slot = self.slot_mut(idx)
        if(slot != null) {
            unsafe {
                slot.occupied = false
                self.live = self.live - 1u32
                if(slot.generation == GEN_MAX) {
                    // Retire: generation 0 is never issued and the slot is not
                    // freed, so the old handle can never alias.
                    slot.generation = 0u32
                    return Result.Err<Unit, UiError>(UiError.make(ErrorCode.ResourceExhausted, "node generation wrapped; slot retired"))
                }
                slot.generation = slot.generation + 1u32
                self.free.push(idx)
                self.free_len = self.free_len + 1u32
                return Result.Ok<Unit, UiError>(Unit{})
            }
        }
        return Result.Err<Unit, UiError>(UiError.make(ErrorCode.Internal, "arena slot missing"))
    }

    public func clear(&mut self) {
        self.free.clear()
        self.free_len = 0u32
        var i : size_t = 1u
        while(i < self.slots.size()) {
            var slot = self.slots.get_ptr(i)
            unsafe {
                slot.occupied = false
                if(slot.generation != GEN_MAX) { self.free.push(i as u32); self.free_len = self.free_len + 1u32 }
            }
            i = i + 1u
        }
        self.live = 0u32
    }

    // Internal test hook: put a free slot one step from generation wrap.
    public func debug_set_generation(&mut self, index : u32, generation : u32) : Result<Unit, UiError> {
        var slot = self.slot_mut(index)
        if(slot != null) {
            unsafe { slot.generation = generation }
            return Result.Ok<Unit, UiError>(Unit{})
        }
        return Result.Err<Unit, UiError>(UiError.make(ErrorCode.InvalidArgument, "arena index out of range"))
    }
}

public struct FrameArena {
    var storage : vector<u8>
    var used_bytes : size_t
    var high_water_bytes : size_t
    var alloc_count : u32

    public func new(capacity : size_t) : FrameArena {
        var a = FrameArena {
            storage : vector<u8>(),
            used_bytes : 0u,
            high_water_bytes : 0u,
            alloc_count : 0u32
        }
        a.storage.resize(capacity)
        return a
    }

    public func capacity(&self) : size_t { return self.storage.size() }
    public func used(&self) : size_t { return self.used_bytes }
    public func high_water(&self) : size_t { return self.high_water_bytes }
    public func allocations(&self) : u32 { return self.alloc_count }

    public func reset(&mut self) {
        self.used_bytes = 0u
        self.alloc_count = 0u32
    }

    public func alloc_offset(&mut self, size : size_t, alignment : size_t) : Result<size_t, UiError> {
        var align = alignment
        if(align == 0u) { align = 1u }
        const max_size = (0 as size_t) - 1u
        if(align > max_size - self.used_bytes) {
            return Result.Err<size_t, UiError>(UiError.make(ErrorCode.ResourceExhausted, "frame arena alignment overflow"))
        }
        var aligned = ((self.used_bytes + align - 1u) / align) * align
        if(size > self.storage.size() || aligned > self.storage.size() - size) {
            return Result.Err<size_t, UiError>(UiError.make(ErrorCode.ResourceExhausted, "frame arena capacity exceeded"))
        }
        self.used_bytes = aligned + size
        if(self.used_bytes > self.high_water_bytes) { self.high_water_bytes = self.used_bytes }
        self.alloc_count = self.alloc_count + 1u32
        return Result.Ok<size_t, UiError>(aligned)
    }

    public func alloc(&mut self, size : size_t, alignment : size_t) : Result<*mut u8, UiError> {
        var off = self.alloc_offset(size, alignment)
        if(off is Result.Err) {
            var Err(e) = off else return Result.Err<*mut u8, UiError>(UiError.make(ErrorCode.Internal, "frame arena result corrupt"))
            return Result.Err<*mut u8, UiError>(e)
        }
        var Ok(offset) = off else return Result.Err<*mut u8, UiError>(UiError.make(ErrorCode.Internal, "frame arena result corrupt"))
        return Result.Ok<*mut u8, UiError>(unsafe(self.storage.mutable_data() + offset))
    }

    public func bytes(&self) : &[u8] {
        return self.storage.as_span().subview(0u, self.used_bytes)
    }

    public func slice(&self, offset : size_t, len : size_t) : Option<std::span<u8>> {
        if(offset > self.used_bytes || len > self.used_bytes - offset) { return Option.None<std::span<u8>>() }
        return Option.Some<std::span<u8>>(self.storage.as_span().subview(offset, offset + len))
    }
}

}
