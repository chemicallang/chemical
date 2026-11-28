// P1.4 retained tree and keyed reconciler. Internal only; no public API.
using std::Option;
using std::Result;
using std::Unit;
using std::string;
using std::string_view;
using std::unordered_map;
using std::vector;

const RNONE : u32 = 0xFFFFFFFFu32

public namespace ui_internal {

@direct_init
public struct ReconcileStats {
    var visited : u32
    var created : u32
    var updated : u32
    var moved : u32
    var removed : u32
    var state_moved : u32
}

@direct_init
public struct RetainedState {
    var used : bool
    var hover : bool
    var focus : bool
    var pressed : bool
    var scroll_x : float
    var scroll_y : float
    var text : string
    var animation : i64
    var custom : i64
}

@direct_init
public struct StateStore {
    var buckets : unordered_map<u64, u32>
    var keys : vector<Key>
    var slots : vector<RetainedState>
    var nexts : vector<u32>
    var live_count : u32

    @make func make() : StateStore {
        return StateStore { buckets : unordered_map<u64, u32>(), keys : vector<Key>(), slots : vector<RetainedState>(), nexts : vector<u32>(), live_count : 0u32 }
    }
    func empty_state() : RetainedState {
        return RetainedState { used : false, hover : false, focus : false, pressed : false,
            scroll_x : 0.0f, scroll_y : 0.0f, text : string(), animation : 0i64, custom : 0i64 }
    }
    func lookup(&self, k : Key) : *mut RetainedState {
        unsafe {
        var h = k.hash_internal()
        h = (h ^ k.id()) * 0x100000001b3u64
        h = (h ^ k.tag()) * 0x100000001b3u64
        var head = self.buckets.get_ptr(&h)
        if(head == null) { return null }
        var cur = unsafe(*head)
        while(cur != RNONE) {
            if(unsafe(self.keys.get_ptr(cur as size_t).equals_key(k))) {
                var s = self.slots.get_ptr(cur as size_t)
                if(unsafe(s.used)) { return s }
                return null
            }
            cur = unsafe(self.nexts.get_ptr(cur as size_t)[0])
        }
        return null
    
        }}
    public func get_or_create(&mut self, k : Key) : *mut RetainedState {
        unsafe {
        var found = self.lookup(k)
        if(found != null) { return found }
        var idx = self.slots.size() as u32
        self.keys.push(k)
        self.slots.push(self.empty_state())
        self.nexts.push(RNONE)
        unsafe { self.slots.get_ptr(idx as size_t).used = true }
        var h = k.hash_internal()
        h = (h ^ k.id()) * 0x100000001b3u64
        h = (h ^ k.tag()) * 0x100000001b3u64
        var head = self.buckets.get_ptr(&h)
        if(head == null) { self.buckets.insert(h, idx) }
        else {
            var cur = unsafe(*head)
            while(unsafe(self.nexts.get_ptr(cur as size_t)[0]) != RNONE) { cur = unsafe(self.nexts.get_ptr(cur as size_t)[0]) }
            unsafe { self.nexts.get_ptr(cur as size_t)[0] = idx }
        }
        self.live_count = self.live_count + 1u32
        return self.slots.get_ptr(idx as size_t)
    
        }}
    public func remove(&mut self, k : Key) {
        var s = self.lookup(k)
        if(s == null) { return }
        unsafe { s.used = false; s.text.clear(); s.hover = false; s.focus = false; s.pressed = false; s.scroll_x = 0.0f; s.scroll_y = 0.0f; s.animation = 0i64; s.custom = 0i64 }
        if(self.live_count > 0u32) { self.live_count = self.live_count - 1u32 }
    }
    public func has(&self, k : Key) : bool { return self.lookup(k) != null }
    public func count(&self) : u32 { return self.live_count }
}

@direct_init
public struct KeyIndex {
    var keys : vector<Key>
    var values : vector<u32>
    var occupied : vector<bool>
    var touched : vector<size_t>
    var mask : size_t
    var count : u32

    public func new(expected : u32) : KeyIndex {
        var cap : size_t = 1u
        var want = (expected as size_t) * 2u + 1u
        while(cap < want) { cap = cap * 2u }
        var idx = KeyIndex { keys : vector<Key>(), values : vector<u32>(), occupied : vector<bool>(), touched : vector<size_t>(), mask : cap - 1u, count : 0u32 }
        idx.keys.resize(cap); idx.values.resize(cap); idx.occupied.resize(cap)
        idx.touched.reserve(cap)
        return idx
    }
    public func clear(&mut self) {
        unsafe {
        var i : size_t = 0
        while(i < self.touched.size()) {
            var s = self.touched.get_ptr(i)[0]
            unsafe { self.occupied.get_ptr(s)[0] = false }
            i = i + 1u
        }
        self.touched.clear()
        self.count = 0u32
    
        }}
    func hash(k : Key) : size_t {
        var h = k.hash_internal()
        h = (h ^ k.id()) * 0x100000001b3u64
        h = (h ^ k.tag()) * 0x100000001b3u64
        return h as size_t
    }
    public func put(&mut self, k : Key, value : u32) : bool {
        unsafe {
        if(self.count >= self.keys.size() as u32) { return false }
        var slot = self.hash(k) & self.mask
        var probes : size_t = 0
        while(probes < self.keys.size()) {
            var occ = self.occupied.get_ptr(slot)
            if(occ == null) { return false }
            unsafe {
                if(!*occ) {
                    *occ = true
                    self.keys.get_ptr(slot)[0] = k
                    self.values.get_ptr(slot)[0] = value
                    self.touched.push(slot)
                    self.count = self.count + 1u32
                    return true
                }
                if(self.keys.get_ptr(slot)[0].equals_key(k)) { return false }
            }
            slot = (slot + 1u) & self.mask
            probes = probes + 1u
        }
        return false
    
        }}
    public func contains(&self, k : Key) : bool {
        unsafe {
        var slot = self.hash(k) & self.mask
        var probes : size_t = 0
        while(probes < self.keys.size()) {
            var occ = self.occupied.get_ptr(slot)
            if(occ == null) { return false }
            if(!*occ) { return false }
            if(self.keys.get_ptr(slot)[0].equals_key(k)) { return true }
            slot = (slot + 1u) & self.mask
            probes = probes + 1u
        }
        return false

        }}
    public func take(&mut self, k : Key) : Option<u32> {
        unsafe {
        var slot = self.hash(k) & self.mask
        var probes : size_t = 0
        while(probes < self.keys.size()) {
            var occ = self.occupied.get_ptr(slot)
            if(occ == null) { return Option.None<u32>() }
            unsafe {
                if(!*occ) { return Option.None<u32>() }
                if(self.keys.get_ptr(slot)[0].equals_key(k)) {
                    var v = self.values.get_ptr(slot)[0]
                    if(v == RNONE) { return Option.None<u32>() }
                    self.values.get_ptr(slot)[0] = RNONE
                    return Option.Some<u32>(v)
                }
            }
            slot = (slot + 1u) & self.mask
            probes = probes + 1u
        }
        return Option.None<u32>()
    
        }}
}

@direct_init
public struct RetainedNode {
    var id : NodeId
    var key : Key
    var kind : ViewKind
    var parent_index : u32
    var first_child : u32
    var last_child : u32
    var next_sibling : u32
    var child_count : u32
    var style_index : u32
    var state_style_index : u32
    var path : u64
    var text : string_view
    var test_id : string_view
    var semantics_label : string_view
}

@direct_init
public struct RetainedTree {
    var arena : NodeArena
    var nodes : vector<RetainedNode>
    var state : StateStore
    var text_arena : FrameArena
    var root : NodeId
    var capacity_value : u32
    var removed_total : u32
    var retained_keys : KeyIndex
    var last_stats : ReconcileStats
    var retained_scratch : vector<NodeId>
    var unkeyed_scratch : vector<u32>
    var matched_scratch : vector<u32>
    var mark_gen : u32
    var work_rids : vector<NodeId>
    var work_fresh : vector<NodeId>

    public func new(capacity : u32) : RetainedTree {
        var t = RetainedTree {
            arena : NodeArena.new(capacity), nodes : vector<RetainedNode>(),
            state : StateStore.make(), text_arena : FrameArena.new(4u * 1024u * 1024u),
            root : zeroed<NodeId>(), capacity_value : capacity, removed_total : 0u32,
            retained_keys : KeyIndex.new(capacity + 1u32),
            last_stats : ReconcileStats { visited : 0u32, created : 0u32, updated : 0u32, moved : 0u32, removed : 0u32, state_moved : 0u32 },
            retained_scratch : vector<NodeId>(), unkeyed_scratch : vector<u32>(),
            matched_scratch : vector<u32>(), mark_gen : 0u32,
            work_rids : vector<NodeId>(), work_fresh : vector<NodeId>()
        }
        t.nodes.reserve(capacity as size_t)
        t.state.keys.reserve(capacity as size_t)
        t.state.slots.reserve(capacity as size_t)
        t.state.nexts.reserve(capacity as size_t)
        t.retained_scratch.reserve(capacity as size_t)
        t.unkeyed_scratch.reserve(capacity as size_t)
        t.matched_scratch.reserve(capacity as size_t)
        t.work_rids.reserve(capacity as size_t)
        t.work_fresh.reserve(capacity as size_t)
        return t
    }
    func empty_node(id : NodeId, k : Key, kind : ViewKind) : RetainedNode {
        return RetainedNode { id : id, key : k, kind : kind, parent_index : RNONE,
            first_child : RNONE, last_child : RNONE, next_sibling : RNONE, child_count : 0u32,
            style_index : 0u32, state_style_index : RNONE, path : 0u64,
            text : "", test_id : "", semantics_label : "" }
    }
    func node_ptr(&mut self, id : NodeId) : *mut RetainedNode {
        unsafe {
        if(!self.arena.contains(id)) { return null }
        if(id.index() > self.nodes.size() as u32) { return null }
        return self.nodes.get_ptr((id.index() - 1u32) as size_t)
    
        }}
    func node_ptr_at(&mut self, index : u32) : *mut RetainedNode {
        unsafe {
        if(index == 0u32 || index == RNONE || index > self.nodes.size() as u32) { return null }
        return self.nodes.get_ptr((index - 1u32) as size_t)
    
        }}
    func create_node(&mut self, k : Key, kind : ViewKind) : NodeId {
        var id = self.arena.insert_fast()
        if(!id.is_valid()) { return zeroed<NodeId>() }
        var ix = id.index()
        if(ix > self.nodes.size() as u32) { self.nodes.push(self.empty_node(id, k, kind)) }
        else { unsafe { self.nodes.get_ptr((ix - 1u32) as size_t)[0] = self.empty_node(id, k, kind) } }
        return id
    }
    func copy_text(&mut self, value : string_view) : string_view {
        if(value.size() == 0u) { return "" }
        var p = self.text_arena.alloc(value.size(), 1u)
        if(p is Result.Err) { return "" }
        var Ok(dst) = p else return ""
        unsafe { memcpy(dst as *mut void, value.data() as *mut void, value.size()) }
        return unsafe(std::string_view(dst as *char, value.size()))
    }
}

func key_is_zero(k : Key) : bool {
    return k.hash_internal() == 0u64 && k.id() == 0u64 && k.tag() == 0u64
}

func fresh_node(b : &ViewBuilder, id : NodeId) : *mut ViewNode {
        unsafe {
    if(!b.arena.contains(id) || id.index() > b.node_count_value) { return null }
    return b.node_base + (id.index() - 1u32)

        }}

func fresh_children(b : &ViewBuilder, id : NodeId) : vector<NodeId> {
        unsafe {
    var out = vector<NodeId>()
    var n = fresh_node(b, id)
    if(n == null) { return out }
    unsafe {
        var cur = n.first_child
        while(cur != RNONE) {
            var cp = b.node_base + (cur - 1u32)
            out.push(cp.id)
            cur = cp.next_sibling
        }
    }
    return out

        }}

func fresh_test_id(b : &ViewBuilder, n : *mut ViewNode) : string_view {
        unsafe {
    unsafe {
        if(n.meta_index == RNONE) { return "" }
        return b.metas.get_ptr(n.meta_index as size_t).test_id
    }

        }}

func fresh_semantics_label(b : &ViewBuilder, n : *mut ViewNode) : string_view {
        unsafe {
    unsafe {
        if(n.meta_index == RNONE) { return "" }
        return b.metas.get_ptr(n.meta_index as size_t).semantics.label
    }

        }}

func sync_node(tree : &mut RetainedTree, id : NodeId, b : &ViewBuilder, fid : NodeId, path : u64) : bool {
    var p = tree.node_ptr(id)
    var f = fresh_node(b, fid)
    if(p == null || f == null) { return false }
    var changed = false
    unsafe {
        if(p.kind != f.kind) { p.kind = f.kind; changed = true }
        if(p.style_index != f.style_index) { p.style_index = f.style_index; changed = true }
        if(p.state_style_index != f.state_style_index) { p.state_style_index = f.state_style_index; changed = true }
        if(!p.text.equals(&f.text)) { p.text = tree.copy_text(f.text); changed = true }
        var ft = fresh_test_id(b, f)
        if(!p.test_id.equals(&ft)) { p.test_id = tree.copy_text(ft); changed = true }
        var sl = fresh_semantics_label(b, f)
        if(!p.semantics_label.equals(&sl)) { p.semantics_label = tree.copy_text(sl); changed = true }
        p.path = path
    }
    return changed
}

func remove_subtree(tree : &mut RetainedTree, id : NodeId, stats : &mut ReconcileStats) {
    unsafe {
        var p = tree.node_ptr(id)
        if(p == null) { return }
        var cur = p.first_child
        while(cur != RNONE) {
            var cp = tree.node_ptr_at(cur)
            if(cp == null) { break }
            var next = cp.next_sibling
            remove_subtree(tree, cp.id, stats)
            cur = next
        }
        var keyv = p.key
        tree.state.remove(keyv)
        var rr = tree.arena.remove(id)
        if(rr is Result.Ok) { stats.removed = stats.removed + 1u32; tree.removed_total = tree.removed_total + 1u32 }
    }
}

func reconcile_children(tree : &mut RetainedTree, parent : NodeId, fresh_parent : NodeId, b : &ViewBuilder, stats : &mut ReconcileStats) : Result<Unit, UiError> {
    unsafe {
        var pp = tree.node_ptr(parent)
        if(pp == null) { return Result.Err<Unit, UiError>(UiError.make(ErrorCode.StaleNode, "retained parent missing")) }
        var fn = fresh_node(b, fresh_parent)
        if(fn == null) { return Result.Err<Unit, UiError>(UiError.make(ErrorCode.StaleNode, "fresh parent missing")) }

        tree.retained_keys.clear()
        tree.retained_scratch.clear()
        tree.unkeyed_scratch.clear()
        var cur = pp.first_child
        while(cur != RNONE) {
            var cp = tree.node_ptr_at(cur)
            if(cp == null) { break }
            var pos = tree.retained_scratch.size() as u32
            tree.retained_scratch.push(cp.id)
            if(key_is_zero(cp.key)) { tree.unkeyed_scratch.push(pos) }
            else { var _ = tree.retained_keys.put(cp.key, pos) }
            cur = cp.next_sibling
        }
        var retained_count = tree.retained_scratch.size() as u32
        if(retained_count > tree.matched_scratch.size() as u32) { tree.matched_scratch.resize(retained_count as size_t) }
        tree.mark_gen = tree.mark_gen + 1u32
        if(tree.mark_gen == 0u32) {
            var z : size_t = 0
            while(z < tree.matched_scratch.size()) { tree.matched_scratch.get_ptr(z)[0] = 0u32; z = z + 1u }
            tree.mark_gen = 1u32
        }
        var gen = tree.mark_gen
        var matched_count = 0u32

        pp.first_child = RNONE
        pp.last_child = RNONE
        pp.child_count = 0u32
        var ppath = pp.path
        var duplicate = false
        var unkeyed_i : size_t = 0
        var new_index = 0u32
        var curf = fn.first_child
        while(curf != RNONE) {
            var fp = b.node_base + (curf - 1u32)
            var nextf = fp.next_sibling
            stats.visited = stats.visited + 1u32
            var fid = fp.id
            var keyv = fp.key
            var found_pos : Option<u32> = Option.None<u32>()
            if(key_is_zero(keyv)) {
                if(unkeyed_i < tree.unkeyed_scratch.size()) { found_pos = Option.Some<u32>(tree.unkeyed_scratch.get_ptr(unkeyed_i)[0]) }
                unkeyed_i = unkeyed_i + 1u
            } else {
                found_pos = tree.retained_keys.take(keyv)
                if(found_pos is Option.None && tree.retained_keys.contains(keyv)) {
                    duplicate = true
                    curf = nextf
                    continue
                }
            }
            var rid = zeroed<NodeId>()
            var pos = found_pos.unwrap_or(RNONE)
            if(pos != RNONE) {
                tree.matched_scratch.get_ptr(pos as size_t)[0] = gen
                matched_count = matched_count + 1u32
                rid = tree.retained_scratch.get_ptr(pos as size_t)[0]
                if(pos != new_index) {
                    stats.moved = stats.moved + 1u32
                    if(tree.state.count() > 0u32 && tree.state.has(keyv)) { stats.state_moved = stats.state_moved + 1u32 }
                }
            } else {
                rid = tree.create_node(keyv, fp.kind)
                if(!rid.is_valid()) { return Result.Err<Unit, UiError>(UiError.make(ErrorCode.ResourceExhausted, "retained tree full")) }
                stats.created = stats.created + 1u32
                var _ = tree.retained_keys.put(keyv, RNONE)
            }
            var path = ppath + (new_index as u64) + 1u64
            if(!key_is_zero(keyv)) { path = (ppath ^ keyv.hash_internal()) * 0x100000001b3u64 }
            var changed = sync_node(tree, rid, b, fid, path)
            if(changed) { stats.updated = stats.updated + 1u32 }
            var rp = tree.node_ptr(rid)
            if(rp == null) { return Result.Err<Unit, UiError>(UiError.make(ErrorCode.StaleNode, "retained child missing")) }
            rp.parent_index = parent.index()
            rp.next_sibling = RNONE
            if(pp.last_child == RNONE) { pp.first_child = rid.index(); pp.last_child = rid.index() }
            else {
                var lp = tree.node_ptr_at(pp.last_child)
                if(lp != null) { lp.next_sibling = rid.index() }
                pp.last_child = rid.index()
            }
            pp.child_count = pp.child_count + 1u32
            if(fp.first_child != RNONE || rp.child_count != 0u32) {
                tree.work_rids.push(rid)
                tree.work_fresh.push(fid)
            }
            new_index = new_index + 1u32
            curf = nextf
        }
        if(matched_count < retained_count) {
            var r : size_t = 0
            while(r < tree.retained_scratch.size()) {
                if(tree.matched_scratch.get_ptr(r)[0] != gen) { remove_subtree(tree, tree.retained_scratch.get_ptr(r)[0], stats) }
                r = r + 1u
            }
        }
        if(duplicate) { return Result.Err<Unit, UiError>(UiError.make(ErrorCode.DuplicateKey, "duplicate sibling key; first wins")) }
        return Result.Ok<Unit, UiError>(Unit{})
    }
}

public func new_retained_tree(capacity : u32) : RetainedTree {
    return RetainedTree.new(capacity)
}

public func reconcile(tree : &mut RetainedTree, b : &ViewBuilder) : Result<ReconcileStats, UiError> {
    var stats = ReconcileStats { visited : 0u32, created : 0u32, updated : 0u32, moved : 0u32, removed : 0u32, state_moved : 0u32 }
    var fn = fresh_node(b, b.root)
    if(fn == null) { return Result.Err<ReconcileStats, UiError>(UiError.make(ErrorCode.StaleNode, "fresh root missing")) }
    var fkey = unsafe(fn.key)
    var fkind = unsafe(fn.kind)
    if(!tree.root.is_valid() || !tree.arena.contains(tree.root)) {
        tree.root = tree.create_node(fkey, fkind)
        if(!tree.root.is_valid()) { return Result.Err<ReconcileStats, UiError>(UiError.make(ErrorCode.ResourceExhausted, "retained root allocation failed")) }
        stats.created = stats.created + 1u32
    } else {
        var rp = tree.node_ptr(tree.root)
        var rk = fkey
        if(rp != null) { rk = unsafe(rp.key) }
        if(rp == null || !rk.equals_key(fkey)) {
            var old = tree.root
            remove_subtree(tree, old, &mut stats)
            tree.root = tree.create_node(fkey, fkind)
            if(!tree.root.is_valid()) { return Result.Err<ReconcileStats, UiError>(UiError.make(ErrorCode.ResourceExhausted, "retained root allocation failed")) }
            stats.created = stats.created + 1u32
        }
    }
    var changed = sync_node(tree, tree.root, b, b.root, 0u64)
    if(changed) { stats.updated = stats.updated + 1u32 }
    tree.work_rids.clear()
    tree.work_fresh.clear()
    var duplicate = false
    var r = reconcile_children(tree, tree.root, b.root, b, &mut stats)
    if(r is Result.Err) {
        var Err(e) = r else return Result.Err<ReconcileStats, UiError>(UiError.make(ErrorCode.Internal, "reconcile result corrupt"))
        if(e.code() != ErrorCode.DuplicateKey) { tree.last_stats = stats; return Result.Err<ReconcileStats, UiError>(e) }
        duplicate = true
    }
    var wi : size_t = 0
    while(wi < tree.work_rids.size()) {
        var rid = unsafe(tree.work_rids.get_ptr(wi)[0])
        var fid = unsafe(tree.work_fresh.get_ptr(wi)[0])
        wi = wi + 1u
        var wr = reconcile_children(tree, rid, fid, b, &mut stats)
        if(wr is Result.Err) {
            var Err(e) = wr else continue
            if(e.code() == ErrorCode.DuplicateKey) { duplicate = true }
            else { tree.last_stats = stats; return Result.Err<ReconcileStats, UiError>(e) }
        }
    }
    tree.last_stats = stats
    if(duplicate) { return Result.Err<ReconcileStats, UiError>(UiError.make(ErrorCode.DuplicateKey, "duplicate sibling key; first wins")) }
    return Result.Ok<ReconcileStats, UiError>(stats)
}

public func retained_root(t : &RetainedTree) : NodeId { return t.root }
public func retained_node_count(t : &RetainedTree) : u32 { return t.nodes.size() as u32 }
public func retained_live(t : &RetainedTree) : u32 { return t.arena.len() }
public func retained_removed_total(t : &RetainedTree) : u32 { return t.removed_total }
public func retained_stats(t : &RetainedTree) : ReconcileStats { return t.last_stats }
public func retained_contains(t : &RetainedTree, id : NodeId) : bool { return t.arena.contains(id) }
public func retained_key(t : &RetainedTree, id : NodeId) : Option<Key> {
        unsafe {
    if(!t.arena.contains(id)) { return Option.None<Key>() }
    var ix = (id.index() - 1u32) as size_t
    if(ix >= t.nodes.size()) { return Option.None<Key>() }
    return Option.Some<Key>(t.nodes.get_ptr(ix)[0].key)

        }}
public func retained_kind(t : &RetainedTree, id : NodeId) : Option<ViewKind> {
        unsafe {
    if(!t.arena.contains(id)) { return Option.None<ViewKind>() }
    return Option.Some<ViewKind>(t.nodes.get_ptr((id.index() - 1u32) as size_t)[0].kind)

        }}
public func retained_parent(t : &RetainedTree, id : NodeId) : Option<NodeId> {
        unsafe {
    if(!t.arena.contains(id)) { return Option.None<NodeId>() }
    var n = t.nodes.get_ptr((id.index() - 1u32) as size_t)
    if(n[0].parent_index == RNONE) { return Option.None<NodeId>() }
    return Option.Some<NodeId>(t.nodes.get_ptr((n[0].parent_index - 1u32) as size_t)[0].id)

        }}
public func retained_child_count(t : &RetainedTree, id : NodeId) : u32 {
        unsafe {
    if(!t.arena.contains(id)) { return 0u32 }
    return t.nodes.get_ptr((id.index() - 1u32) as size_t)[0].child_count

        }}
public func retained_child_at(t : &RetainedTree, id : NodeId, index : u32) : Option<NodeId> {
        unsafe {
    if(!t.arena.contains(id)) { return Option.None<NodeId>() }
    var n = t.nodes.get_ptr((id.index() - 1u32) as size_t)
    if(index >= n[0].child_count) { return Option.None<NodeId>() }
    var cur = n[0].first_child
    var i = 0u32
    while(i < index && cur != RNONE) {
        var c = t.nodes.get_ptr((cur - 1u32) as size_t)
        cur = c[0].next_sibling
        i = i + 1u32
    }
    if(cur == RNONE) { return Option.None<NodeId>() }
    return Option.Some<NodeId>(t.nodes.get_ptr((cur - 1u32) as size_t)[0].id)

        }}
public func retained_text(t : &RetainedTree, id : NodeId) : Option<string_view> {
        unsafe {
    if(!t.arena.contains(id)) { return Option.None<string_view>() }
    return Option.Some<string_view>(t.nodes.get_ptr((id.index() - 1u32) as size_t)[0].text)

        }}
public func retained_test_id(t : &RetainedTree, id : NodeId) : Option<string_view> {
        unsafe {
    if(!t.arena.contains(id)) { return Option.None<string_view>() }
    return Option.Some<string_view>(t.nodes.get_ptr((id.index() - 1u32) as size_t)[0].test_id)

        }}
public func retained_semantics_label(t : &RetainedTree, id : NodeId) : Option<string_view> {
        unsafe {
    if(!t.arena.contains(id)) { return Option.None<string_view>() }
    return Option.Some<string_view>(t.nodes.get_ptr((id.index() - 1u32) as size_t)[0].semantics_label)

        }}
public func retained_style_index(t : &RetainedTree, id : NodeId) : Option<u32> {
        unsafe {
    if(!t.arena.contains(id)) { return Option.None<u32>() }
    return Option.Some<u32>(t.nodes.get_ptr((id.index() - 1u32) as size_t)[0].style_index)

        }}
public func retained_path(t : &RetainedTree, id : NodeId) : u64 {
        unsafe {
    if(!t.arena.contains(id)) { return 0u64 }
    return t.nodes.get_ptr((id.index() - 1u32) as size_t)[0].path

        }}
public func retained_state_count(t : &RetainedTree) : u32 { return t.state.count() }
public func retained_state_has(t : &RetainedTree, k : Key) : bool { return t.state.has(k) }
public func retained_state_set_hover(t : &mut RetainedTree, k : Key, on : bool) {
    var s = t.state.get_or_create(k); unsafe { s.hover = on }
}
public func retained_state_hover(t : &RetainedTree, k : Key) : Option<bool> {
    var s = t.state.lookup(k); if(s == null) { return Option.None<bool>() }; return Option.Some<bool>(unsafe(s.hover))
}
public func retained_state_set_focus(t : &mut RetainedTree, k : Key, on : bool) {
    var s = t.state.get_or_create(k); unsafe { s.focus = on }
}
public func retained_state_focus(t : &RetainedTree, k : Key) : Option<bool> {
    var s = t.state.lookup(k); if(s == null) { return Option.None<bool>() }; return Option.Some<bool>(unsafe(s.focus))
}
public func retained_state_set_pressed(t : &mut RetainedTree, k : Key, on : bool) {
    var s = t.state.get_or_create(k); unsafe { s.pressed = on }
}
public func retained_state_pressed(t : &RetainedTree, k : Key) : Option<bool> {
    var s = t.state.lookup(k); if(s == null) { return Option.None<bool>() }; return Option.Some<bool>(unsafe(s.pressed))
}
public func retained_state_set_scroll(t : &mut RetainedTree, k : Key, x : float, y : float) {
    var s = t.state.get_or_create(k); unsafe { s.scroll_x = x; s.scroll_y = y }
}
public func retained_state_scroll(t : &RetainedTree, k : Key) : Option<Point> {
    var s = t.state.lookup(k); if(s == null) { return Option.None<Point>() }; return Option.Some<Point>(Point { x : unsafe(s.scroll_x), y : unsafe(s.scroll_y) })
}
public func retained_state_set_animation(t : &mut RetainedTree, k : Key, value : i64) {
    var s = t.state.get_or_create(k); unsafe { s.animation = value }
}
public func retained_state_animation(t : &RetainedTree, k : Key) : Option<i64> {
    var s = t.state.lookup(k); if(s == null) { return Option.None<i64>() }; return Option.Some<i64>(unsafe(s.animation))
}
public func retained_state_set_custom(t : &mut RetainedTree, k : Key, value : i64) {
    var s = t.state.get_or_create(k); unsafe { s.custom = value }
}
public func retained_state_custom(t : &RetainedTree, k : Key) : Option<i64> {
    var s = t.state.lookup(k); if(s == null) { return Option.None<i64>() }; return Option.Some<i64>(unsafe(s.custom))
}
public func retained_state_set_text(t : &mut RetainedTree, k : Key, value : string_view) {
    var s = t.state.get_or_create(k)
    unsafe { s.text.clear(); s.text.append_view(&value) }
}
public func retained_state_text(t : &RetainedTree, k : Key) : Option<string_view> {
    var s = t.state.lookup(k)
    if(s == null) { return Option.None<string_view>() }
    unsafe { return Option.Some<string_view>(s.text.to_view()) }
}

} // namespace ui_internal
