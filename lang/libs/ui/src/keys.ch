// P1.2 cross-frame keys. Keys store only (hash, tag, id); the bounded debug
// label table is best-effort and never participates in identity.
using std::Option;
using std::Result;
using std::Unit;
using std::string_view;
using std::unordered_map;
using std::vector;

const KEY_DEBUG_CAP : size_t = 64u
const KEY_DEBUG_LEN : size_t = 32u

@direct_init
struct KeyDebugLabel {
    var hash : u64
    var len : u32
    var bytes : [32]char
}

@direct_init
struct KeyDebugTable {
    var count : u32
    var entries : [64]KeyDebugLabel
}

@thread_local
var KEY_DEBUG : KeyDebugTable = zeroed<KeyDebugTable>()

func key_debug_remember(hash : u64, label : string_view) {
    if(KEY_DEBUG.count >= KEY_DEBUG_CAP as u32) { return }
    var entry = &raw mut KEY_DEBUG.entries[KEY_DEBUG.count as size_t]
    unsafe { entry.hash = hash }
    var n = label.size()
    if(n > KEY_DEBUG_LEN) { n = KEY_DEBUG_LEN }
    unsafe { entry.len = n as u32 }
    var i : size_t = 0
    while(i < n) {
        unsafe { entry.bytes[i] = label.data()[i] }
        i = i + 1u
    }
    KEY_DEBUG.count = KEY_DEBUG.count + 1u32
}

const FNV_OFFSET : u64 = 0xcbf29ce484222325u64
const FNV_PRIME : u64 = 0x100000001b3u64

func key_hash_view(label : string_view) : u64 {
    var h = FNV_OFFSET
    var i : size_t = 0
    while(i < label.size()) {
        var b = unsafe(label.data()[i]) as u8
        h = (h ^ (b as u64)) * FNV_PRIME
        i = i + 1u
    }
    return h
}

func key_hash_u64(h : u64, value : u64) : u64 {
    var v = value
    var i = 0
    while(i < 8) {
        h = (h ^ (v & 0xFFu64)) * FNV_PRIME
        v = v >> 8u64
        i = i + 1
    }
    return h
}

@direct_init
public struct Key {
    private var hash_value : u64
    private var tag_value : u64
    private var id_value : u64

    func make_internal(hash : u64, tag : u64, id : u64) : Key {
        return Key { hash_value : hash, tag_value : tag, id_value : id }
    }

    func hash_internal(&self) : u64 { return self.hash_value }
    public func id(&self) : u64 { return self.id_value }
    public func tag(&self) : u64 { return self.tag_value }
    public func equals(&self, label : string_view) : bool {
        return self.hash_value == key_hash_view(label)
    }
    public func equals_key(&self, other : Key) : bool {
        return self.hash_value == other.hash_value && self.tag_value == other.tag_value && self.id_value == other.id_value
    }
}

public func key(label : string_view) : Key {
    var h = key_hash_view(label)
    key_debug_remember(h, label)
    return Key.make_internal(h, 0u64, 0u64)
}

public func key_of(label : string_view, id : u64) : Key {
    var h = key_hash_view(label)
    key_debug_remember(h, label)
    return Key.make_internal(h, 0u64, id)
}

// Order-sensitive: label, 0xFF, little-endian a, 0xFF, little-endian b.
public func key_pair(label : string_view, a : u64, b : u64) : Key {
    var h = key_hash_view(label)
    h = (h ^ 0xFFu64) * FNV_PRIME
    h = key_hash_u64(h, a)
    h = (h ^ 0xFFu64) * FNV_PRIME
    h = key_hash_u64(h, b)
    key_debug_remember(h, label)
    return Key.make_internal(h, b, a)
}

public namespace ui_internal {

@direct_init
public struct KeyScope {
    var buckets : unordered_map<u64, u32>
    var keys : vector<Key>
    var nexts : vector<u32>

    @make func make() : KeyScope {
        return KeyScope { buckets : unordered_map<u64, u32>(), keys : vector<Key>(), nexts : vector<u32>() }
    }

    public func insert(&mut self, k : Key) : Result<Unit, UiError> {
        var h = k.hash_internal()
        var head = self.buckets.get_ptr(&h)
        if(head == null) {
            var idx = self.keys.size() as u32
            self.buckets.insert(h, idx)
            self.keys.push(k)
            self.nexts.push(0xFFFFFFFFu32)
            return Result.Ok<Unit, UiError>(Unit{})
        }
        var cur = unsafe(*head)
        var last = 0xFFFFFFFFu32
        while(cur != 0xFFFFFFFFu32) {
            var existing = self.keys.get_ptr(cur as size_t)
            if(unsafe(existing.equals_key(k))) {
                return Result.Err<Unit, UiError>(UiError.make(ErrorCode.DuplicateKey, "duplicate key in one scope"))
            }
            last = cur
            cur = unsafe(self.nexts.get_ptr(cur as size_t)[0])
        }
        var idx = self.keys.size() as u32
        self.keys.push(k)
        self.nexts.push(0xFFFFFFFFu32)
        if(last != 0xFFFFFFFFu32) {
            unsafe { self.nexts.get_ptr(last as size_t)[0] = idx }
        }
        return Result.Ok<Unit, UiError>(Unit{})
    }

    public func contains(&self, k : Key) : bool {
        var kh = k.hash_internal()
        var head = self.buckets.get_ptr(&kh)
        if(head == null) { return false }
        var cur = unsafe(*head)
        while(cur != 0xFFFFFFFFu32) {
            if(unsafe(self.keys.get_ptr(cur as size_t).equals_key(k))) { return true }
            cur = unsafe(self.nexts.get_ptr(cur as size_t)[0])
        }
        return false
    }
    public func size(&self) : size_t { return self.keys.size() }
}

@direct_init
public struct KeyedSlots {
    var buckets : unordered_map<u64, u32>
    var keys : vector<Key>
    var values : vector<i64>
    var nexts : vector<u32>

    @make func make() : KeyedSlots {
        return KeyedSlots { buckets : unordered_map<u64, u32>(), keys : vector<Key>(), values : vector<i64>(), nexts : vector<u32>() }
    }

    public func set(&mut self, k : Key, value : i64) {
        var h = k.hash_internal()
        var head = self.buckets.get_ptr(&h)
        if(head == null) {
            var idx = self.keys.size() as u32
            self.buckets.insert(h, idx)
            self.keys.push(k)
            self.values.push(value)
            self.nexts.push(0xFFFFFFFFu32)
            return
        }
        var cur = unsafe(*head)
        var last = 0xFFFFFFFFu32
        while(cur != 0xFFFFFFFFu32) {
            if(unsafe(self.keys.get_ptr(cur as size_t).equals_key(k))) {
                unsafe { self.values.get_ptr(cur as size_t)[0] = value }
                return
            }
            last = cur
            cur = unsafe(self.nexts.get_ptr(cur as size_t)[0])
        }
        var idx = self.keys.size() as u32
        self.keys.push(k)
        self.values.push(value)
        self.nexts.push(0xFFFFFFFFu32)
        if(last != 0xFFFFFFFFu32) {
            unsafe { self.nexts.get_ptr(last as size_t)[0] = idx }
        }
    }

    public func get(&self, k : Key) : Option<i64> {
        var kh = k.hash_internal()
        var head = self.buckets.get_ptr(&kh)
        if(head == null) { return Option.None<i64>() }
        var cur = unsafe(*head)
        while(cur != 0xFFFFFFFFu32) {
            if(unsafe(self.keys.get_ptr(cur as size_t).equals_key(k))) {
                return Option.Some<i64>(unsafe(self.values.get_ptr(cur as size_t)[0]))
            }
            cur = unsafe(self.nexts.get_ptr(cur as size_t)[0])
        }
        return Option.None<i64>()
    }

    public func size(&self) : size_t { return self.keys.size() }
}

public func key_debug_count() : u32 { return KEY_DEBUG.count }

// Test-only same-hash construction for collision handling. Not public API.
public func key_with_hash(hash : u64, tag : u64, id : u64) : Key {
    return Key.make_internal(hash, tag, id)
}

}
