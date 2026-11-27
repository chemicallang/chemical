// P0.3b test-only core. This is deliberately not the production `UiHarness`;
// it models just enough of the frozen surface to exercise the harness and
// golden contracts before the renderer exists. P6.1 replaces it.

using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;
using std::Unit;

public namespace ui_test_core {

@direct_init
public struct NodeId {
    var index : u32
    var generation : u32
}

@direct_init
public struct Key { var hash : u64 }

@direct_init
public struct Bounds { var x : float; var y : float; var w : float; var h : float }

@direct_init
public struct Semantics { var role : u8; var label : string }

@direct_init
public struct Query {
    var role : i32 = -1
    var text : string_view = ""
    var test_id : string_view = ""

    @make func make() : Query { return Query { role : -1, text : "", test_id : "" } }
}

@direct_init
public struct CoreStats { var frame : u64; var nodes : u32 }

@direct_init
public struct GoldenOptions {
    var max_diff_pixels : u32 = 0
    @make func make() : GoldenOptions { return GoldenOptions { max_diff_pixels : 0 } }
}

public var last_golden_diff : u32 = 0u32

@direct_init
public struct GoldenResult {
    var passed : bool
    var updated : bool
    var diff_pixels : u32
    var diff_path : string
}

public variant CoreError {
    Timeout()
    NotFound()
    InvalidArgument(msg : string)
    Io(msg : string)
    Image(msg : string)
    GoldenMismatch(diff_pixels : u32)
}

func key_hash(label : string_view) : u64 {
    return label.hash() as u64
}

public func key(label : string_view) : Key { return Key { hash : key_hash(label) } }

@direct_init
public struct TestNode {
    var id : NodeId
    var key : Key
    var parent : Option<NodeId>
    var children : vector<NodeId>
    var bounds : Bounds
    var text : string
    var semantics : Semantics
    var test_id : string
    var visible : bool
}

@direct_init
public struct KeyState { var key : Key; var value : i64 }

public struct Harness {
    var nodes : vector<TestNode>
    var states : vector<KeyState>
    var next_index : u32
    var frame_no : u64
    var now_ms : f64
    var dirty : bool
    var never_settle : bool
    var focused_id : Option<NodeId>
    var scroll_keys : vector<Key>
    var scroll_x : vector<float>
    var scroll_y : vector<float>

    @make func make() : Harness {
        unsafe {
        return Harness {
            nodes : vector<TestNode>(),
            states : vector<KeyState>(),
            next_index : 1u32,
            frame_no : 0u64,
            now_ms : 0.0,
            dirty : true,
            never_settle : false,
            focused_id : Option.None<NodeId>(),
            scroll_keys : vector<Key>(),
            scroll_x : vector<float>(),
            scroll_y : vector<float>()
        }
        }
    }

    public func add_node(&mut self, parent : Option<NodeId>, key : Key, bounds : Bounds,
                         text : string_view, role : u8, test_id : string_view) : NodeId {
        unsafe {
        var id = NodeId { index : self.next_index, generation : 1u32 }
        self.next_index = self.next_index + 1u32
        var node = TestNode {
            id : id,
            key : key,
            parent : parent,
            children : vector<NodeId>(),
            bounds : bounds,
            text : string(text),
            semantics : Semantics { role : role, label : string(text) },
            test_id : string(test_id),
            visible : true
        }
        self.nodes.push(node)
        if(parent is Option.Some) {
            var Some(p) = parent else return id
            var pi = self.index_of(p)
            if(pi is Option.Some) {
                var Some(ix) = pi else return id
                self.nodes.get_ptr(ix as size_t).children.push(id)
            }
        }
        self.dirty = true
        return id
        }
    }

    func index_of(&self, id : NodeId) : Option<size_t> {
        unsafe {
        var i : size_t = 0
        while(i < self.nodes.size()) {
            var n = self.nodes.get_ptr(i)
            if(n.id.index == id.index) {
                if(n.visible && n.id.generation == id.generation) {
                    return Option.Some<size_t>(i)
                }
                return Option.None<size_t>()
            }
            i = i + 1
        }
        return Option.None<size_t>()
        }
    }

    public func by_key(&self, key : Key) : Option<NodeId> {
        unsafe {
        var i : size_t = 0
        while(i < self.nodes.size()) {
            var n = self.nodes.get_ptr(i)
            if(n.visible && n.key.hash == key.hash) {
                return Option.Some<NodeId>(n.id)
            }
            i = i + 1
        }
        return Option.None<NodeId>()
        }
    }

    public func bounds(&self, id : NodeId) : Option<Bounds> {
        unsafe {
        var i = self.index_of(id)
        if(i is Option.Some) {
            var Some(ix) = i else return Option.None<Bounds>()
            return Option.Some<Bounds>(self.nodes.get_ptr(ix as size_t).bounds)
        }
        return Option.None<Bounds>()
        }
    }

    public func text_of(&self, id : NodeId) : Option<string> {
        unsafe {
        var i = self.index_of(id)
        if(i is Option.Some) {
            var Some(ix) = i else return Option.None<string>()
            return Option.Some<string>(self.nodes.get_ptr(ix as size_t).text.copy())
        }
        return Option.None<string>()
        }
    }

    public func semantics(&self, id : NodeId) : Option<Semantics> {
        unsafe {
        var i = self.index_of(id)
        if(i is Option.Some) {
            var Some(ix) = i else return Option.None<Semantics>()
            return Option.Some<Semantics>(self.nodes.get_ptr(ix as size_t).semantics)
        }
        return Option.None<Semantics>()
        }
    }

    public func child_at(&self, id : NodeId, index : u32) : Option<NodeId> {
        unsafe {
        var i = self.index_of(id)
        if(i is Option.Some) {
            var Some(ix) = i else return Option.None<NodeId>()
            if((index as size_t) < self.nodes.get_ptr(ix as size_t).children.size()) {
                return Option.Some<NodeId>(self.nodes.get_ptr(ix as size_t).children.get_ptr(index as size_t)[0])
            }
        }
        return Option.None<NodeId>()
        }
    }

    public func find_all(&self, q : Query) : vector<NodeId> {
        unsafe {
        var out = vector<NodeId>()
        var i : size_t = 0
        while(i < self.nodes.size()) {
            var n = self.nodes.get_ptr(i)
            if(n.visible) {
                var ok = true
                if(q.role >= 0 && (n.semantics.role as i32) != q.role) { ok = false }
                if(q.text.size() > 0 && !n.text.to_view().equals(&q.text)) { ok = false }
                if(q.test_id.size() > 0 && !n.test_id.to_view().equals(&q.test_id)) { ok = false }
                if(ok) { out.push(n.id) }
            }
            i = i + 1
        }
        return out
        }
    }

    public func reorder(&mut self, parent : NodeId, kids : &[NodeId]) {
        unsafe {
        var i = self.index_of(parent)
        if(i is Option.Some) {
            var Some(ix) = i else return
            var node = self.nodes.get_ptr(ix as size_t)
            node.children.clear()
            var k : size_t = 0
            while(k < kids.size()) { node.children.push(kids[k]); k = k + 1 }
            self.dirty = true
        }
        }
    }

    public func remove_node(&mut self, id : NodeId) {
        unsafe {
        var i = self.index_of(id)
        if(i is Option.Some) {
            var Some(ix) = i else return
            var n = self.nodes.get_ptr(ix as size_t)
            n.visible = false
            n.id.generation = n.id.generation + 1u32
            self.dirty = true
        }
        }
    }

    public func set_state(&mut self, key : Key, value : i64) {
        unsafe {
        var i : size_t = 0
        while(i < self.states.size()) {
            var s = self.states.get_ptr(i)
            if(s.key.hash == key.hash) { s.value = value; return }
            i = i + 1
        }
        self.states.push(KeyState { key : key, value : value })
        }
    }

    public func get_state(&self, key : Key) : i64 {
        unsafe {
        var i : size_t = 0
        while(i < self.states.size()) {
            var s = self.states.get_ptr(i)
            if(s.key.hash == key.hash) { return s.value }
            i = i + 1
        }
        return -1
        }
    }

    public func set_focus(&mut self, id : NodeId) { self.focused_id = Option.Some<NodeId>(id) }
    public func focused(&self) : Option<NodeId> { return self.focused_id }
    public func clear_focus(&mut self) { self.focused_id = Option.None<NodeId>() }

    public func set_scroll(&mut self, key : Key, x : float, y : float) {
        unsafe {
        var i : size_t = 0
        while(i < self.scroll_keys.size()) {
            var k = self.scroll_keys.get_ptr(i)
            if(k.hash == key.hash) { self.scroll_x.get_ptr(i)[0] = x; self.scroll_y.get_ptr(i)[0] = y; return }
            i = i + 1
        }
        self.scroll_keys.push(key)
        self.scroll_x.push(x)
        self.scroll_y.push(y)
        }
    }

    public func scroll_offset(&self, key : Key) : Bounds {
        unsafe {
        var i : size_t = 0
        while(i < self.scroll_keys.size()) {
            if(self.scroll_keys.get_ptr(i).hash == key.hash) {
                return Bounds { x : self.scroll_x.get_ptr(i)[0], y : self.scroll_y.get_ptr(i)[0], w : 0.0f, h : 0.0f }
            }
            i = i + 1
        }
        return Bounds { x : 0.0f, y : 0.0f, w : 0.0f, h : 0.0f }
        }
    }

    public func frame(&mut self) : CoreStats {
        unsafe {
        self.frame_no = self.frame_no + 1u64
        self.dirty = false
        return CoreStats { frame : self.frame_no, nodes : self.nodes.size() as u32 }
        }
    }

    public func advance(&mut self, ms : f64) : CoreStats {
        unsafe {
        self.now_ms = self.now_ms + ms
        return self.frame()
        }
    }

    public func settle(&mut self, max_frames : u32) : Result<u32, CoreError> {
        unsafe {
        var frames : u32 = 0
        while(self.dirty || self.never_settle) {
            if(frames >= max_frames) {
                return Result.Err<u32, CoreError>(CoreError.Timeout())
            }
            var s = self.frame()
            frames = frames + 1u32
        }
        return Result.Ok<u32, CoreError>(frames)
        }
    }

    public func set_never_settle(&mut self, value : bool) { self.never_settle = value }

    public func geometry_snapshot(&self) : string {
        unsafe {
        var out = string("[")
        var i : size_t = 0
        while(i < self.nodes.size()) {
            var n = self.nodes.get_ptr(i)
            if(n.visible) {
                if(i > 0u) { out.append_view(", ") }
                var buf : [160]char
                unsafe {
                    sprintf(&raw mut buf[0],
                            "{\"id\":%u,\"key\":%lu,\"x\":%.2f,\"y\":%.2f,\"w\":%.2f,\"h\":%.2f}",
                            n.id.index, n.key.hash, n.bounds.x, n.bounds.y, n.bounds.w, n.bounds.h)
                }
                var sv = unsafe(std::string_view.make_no_len(unsafe(&raw mut buf[0])))
                out.append_view(&sv)
            }
            i = i + 1
        }
        out.append_view("]")
        return out
        }
    }

    public func semantics_snapshot(&self) : string {
        unsafe {
        var out = string("[")
        var first = true
        var i : size_t = 0
        while(i < self.nodes.size()) {
            var n = self.nodes.get_ptr(i)
            if(n.visible) {
                if(!first) { out.append_view(", ") }
                first = false
                var buf : [256]char
                unsafe {
                    sprintf(&raw mut buf[0], "{\"id\":%u,\"role\":%u,\"label\":\"%s\"}",
                            n.id.index, n.semantics.role as u32, n.semantics.label.data())
                }
                var sv = unsafe(std::string_view.make_no_len(unsafe(&raw mut buf[0])))
                out.append_view(&sv)
            }
            i = i + 1
        }
        out.append_view("]")
        return out
        }
    }
}

func path_of(p_str : string_view) : path::PathBuf {
    return path::PathBuf.from(std::str.make_view(p_str))
}

func sha256_hex(data : &[u8]) : string {
    var digest = crypto::sha256(data)
    var out = string()
    var i : size_t = 0
    while(i < 32u) {
        var buf : [3]char
        unsafe { sprintf(&raw mut buf[0], "%02x", digest[i] as u32) }
        var sv = unsafe(std::string_view.make_no_len(unsafe(&raw mut buf[0])))
        out.append_view(&sv)
        i = i + 1u
    }
    return out
}

func write_png(img : &image::Image, path : string_view) : Result<Unit, CoreError> {
    var enc = img.encode_png()
    if(enc is Result.Err) {
        return Result.Err<Unit, CoreError>(CoreError.Image(string("encode failed")))
    }
    var Ok(bytes) = enc else return Result.Err<Unit, CoreError>(CoreError.Image(string("encode failed")))
    var p = path_of(path)
    var wr = fs::write(&p.as_path(), bytes.as_span())
    if(wr is Result.Err) {
        return Result.Err<Unit, CoreError>(CoreError.Io(string("write failed")))
    }
    return Result.Ok<Unit, CoreError>(Unit{})
}

func load_png(path : string_view) : Result<image::Image, CoreError> {
    var p = path_of(path)
    var read = fs::read(&p.as_path())
    if(read is Result.Err) {
        return Result.Err<image::Image, CoreError>(CoreError.Io(string("read failed")))
    }
    var Ok(data) = read else return Result.Err<image::Image, CoreError>(CoreError.Io(string("read failed")))
    var decoded = image::Image.from_png(data.as_span())
    if(decoded is Result.Err) {
        return Result.Err<image::Image, CoreError>(CoreError.Image(string("decode failed")))
    }
    var Ok(img) = decoded else return Result.Err<image::Image, CoreError>(CoreError.Image(string("decode failed")))
    return Result.Ok<image::Image, CoreError>(img)
}

public func update_requested() : bool {
    var env = environment::get("CHEMICAL_UI_UPDATE_GOLDENS")
    var value = env.unwrap_or(string(""))
    return value.to_view().equals_with_len("1", 1u)
}

public func assert_golden(actual : &image::Image, path : string_view,
                          opts : GoldenOptions) : Result<GoldenResult, CoreError> {
    var update = false
    var env = environment::get("CHEMICAL_UI_UPDATE_GOLDENS")
    var update_value = env.unwrap_or(string(""))
    if(update_value.to_view().equals_with_len("1", 1u)) { update = true }
    if(update) {
        var wr = write_png(actual, path)
        if(wr is Result.Err) { return Result.Err<GoldenResult, CoreError>(CoreError.Io(string("update write failed"))) }
        println("updated golden")
        return Result.Ok<GoldenResult, CoreError>(GoldenResult {
            passed : true, updated : true, diff_pixels : 0u32, diff_path : string()
        })
    }
    var expected = load_png(path)
    if(expected is Result.Err) { return Result.Err<GoldenResult, CoreError>(CoreError.Io(string("golden load failed"))) }
    var Ok(want) = expected else return Result.Err<GoldenResult, CoreError>(CoreError.Io(string("golden load failed")))
    if(want.width() != actual.width() || want.height() != actual.height()) {
        return Result.Err<GoldenResult, CoreError>(CoreError.Image(string("golden dimensions differ")))
    }
    var wp = want.pixels()
    var ap = actual.pixels()
    var diff : u32 = 0
    var w = want.width()
    var h = want.height()
    var y = 0
    while(y < h) {
        var x = 0
        while(x < w) {
            var o = ((y as size_t) * (w as size_t) + (x as size_t)) * 4u
            if(wp[o] != ap[o] || wp[o+1u] != ap[o+1u] || wp[o+2u] != ap[o+2u] || wp[o+3u] != ap[o+3u]) {
                diff = diff + 1u32
            }
            x = x + 1
        }
        y = y + 1
    }
    unsafe { last_golden_diff = diff }
    if(diff == 0u32 || diff <= opts.max_diff_pixels) {
        return Result.Ok<GoldenResult, CoreError>(GoldenResult {
            passed : true, updated : false, diff_pixels : diff, diff_path : string()
        })
    }
    var base = string(path)
    var exp_path = base.copy(); exp_path.append_view(".expected.png")
    var act_path = base.copy(); act_path.append_view(".actual.png")
    var dif_path = base.copy(); dif_path.append_view(".diff.png")
    write_png(&want, exp_path.to_view())
    write_png(actual, act_path.to_view())
    var diff_img = image::Image.new_rgba(w, h)
    if(diff_img is Result.Ok) {
        var Ok(di) = diff_img else return Result.Err<GoldenResult, CoreError>(CoreError.Image(string("diff alloc failed")))
        var dp = di.pixels_mut()
        var yy = 0
        while(yy < h) {
            var xx = 0
            while(xx < w) {
                var o = ((yy as size_t) * (w as size_t) + (xx as size_t)) * 4u
                var same = wp[o] == ap[o] && wp[o+1u] == ap[o+1u] && wp[o+2u] == ap[o+2u] && wp[o+3u] == ap[o+3u]
                if(same) { dp[o] = 0u8; dp[o+1u] = 0u8; dp[o+2u] = 0u8; dp[o+3u] = 255u8 }
                else { dp[o] = 255u8; dp[o+1u] = 0u8; dp[o+2u] = 0u8; dp[o+3u] = 255u8 }
                xx = xx + 1
            }
            yy = yy + 1
        }
        write_png(&di, dif_path.to_view())
    }
    return Result.Err<GoldenResult, CoreError>(CoreError.GoldenMismatch(diff))
}

} // namespace ui_test_core
