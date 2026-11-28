// P1.4 retained tree/keyed reconciler differential + release checks.
using std::Option;
using std::Result;
using std::string_view;
using std::vector;
using ui_internal::new_view_builder;
using ui_internal::view_root;
using ui_internal::reset_view_builder;
using ui_internal::new_retained_tree;
using ui_internal::RetainedTree;
using ui_internal::reconcile;
using ui_internal::retained_root;
using ui_internal::retained_live;
using ui_internal::retained_key;
using ui_internal::retained_child_count;
using ui_internal::retained_child_at;
using ui_internal::retained_text;
using ui_internal::retained_style_index;
using ui_internal::retained_semantics_label;
using ui_internal::retained_path;
using ui_internal::retained_removed_total;
using ui_internal::retained_state_count;
using ui_internal::retained_state_has;
using ui_internal::retained_state_set_hover;
using ui_internal::retained_state_set_focus;
using ui_internal::retained_state_set_pressed;
using ui_internal::retained_state_set_scroll;
using ui_internal::retained_state_scroll;
using ui_internal::retained_state_set_animation;
using ui_internal::retained_state_animation;
using ui_internal::retained_state_set_custom;
using ui_internal::retained_state_custom;
using ui_internal::retained_state_set_text;
using ui_internal::retained_state_text;
using ui_internal::retained_stats;

public namespace reconcile_tests {

func fail(name : *char, n : u64) : bool {
    unsafe { printf("reconcile FAIL %s case=%lu\n", name, n) }
    return false
}
func is_llvm() : bool {
    var env = environment::get("CHEMICAL_UI_TEST_BACKEND")
    if(env is Option.Some) { var Some(v)=env else return false; return v.to_view().equals_with_len("llvm",4u) }
    return false
}
func release_mode() : bool {
    var env = environment::get("CHEMICAL_UI_RELEASE")
    if(env is Option.Some) { var Some(v)=env else return false; return v.to_view().equals_with_len("1",1u) }
    return false
}
func key_at(tree : &RetainedTree, root : NodeId, i : u32) : Option<Key> {
    var r = retained_child_at(tree, root, i)
    if(r is Option.Some) { var Some(id)=r else return Option.None<Key>(); return retained_key(tree,id) }
    return Option.None<Key>()
}
func semantics_label(label : string_view) : Semantics {
    return Semantics {
        role : Role.Unknown, label : label, description : "", value : "", alt : "",
        lang : "", placeholder : "", error : "",
        states : zeroed<SemState>(), actions : zeroed<SemAction>(),
        level : 0u8, pos_in_set : 0u32, set_size : 0u32, row_index : 0u32,
        column_index : 0u32, selected_index : 0i32,
        labelled_by : Option.None<NodeId>(), described_by : Option.None<NodeId>(),
        controls : Option.None<NodeId>(), owns : Option.None<NodeId>(),
        active_descendant : Option.None<NodeId>(), has_popup : false, secure : false
    }
}
func build_prefix(b : &mut ViewBuilder, root : NodeId, model : &[Key], extra_dup : bool) {
    var i : size_t = 0
    while(i < model.size()) {
        var n = b.text(model[i], "x", Style.new())
        b.child(root, n)
        i = i + 1u
    }
    if(extra_dup && model.size() > 0u) { var n = b.text(model[0], "dup", Style.new()); b.child(root, n) }
}

func check_basic() : bool {
    var b = new_view_builder(64u32, 100.0f, 100.0f)
    var root = view_root(&b)
    var ka = key("a"); var kb = key("b"); var kc = key("c")
    var styled = Style.new().background(ColorToken.Primary)
    var a = b.text(ka, "A", styled); b.child(root,a)
    var sem = semantics_label("alpha"); b.semantics(a, sem)
    var c = b.text(kc, "C", Style.new()); b.child(root,c)
    var rb = b.text(kb, "B", Style.new()); b.child(root,rb)
    var t = new_retained_tree(64u32)
    var rr = reconcile(&mut t, &mut b)
    if(rr is Result.Err) { return fail("basic-reconcile",0u64) }
    var rrroot = retained_root(&t)
    if(retained_live(&t) != 4u32 || retained_child_count(&t, rrroot) != 3u32) { return fail("basic-count",0u64) }
    var k0 = key_at(&t, rrroot, 0u32)
    if(k0 is Option.Some) { var Some(v)=k0 else return false; if(!v.equals_key(ka)) { return fail("basic-order",0u64) } } else { return fail("basic-key",0u64) }
    var tx = retained_child_at(&t, rrroot, 0u32)
    if(tx is Option.Some) {
        var Some(id)=tx else return false
        var tv=retained_text(&t,id)
        if(tv is Option.Some) { var Some(s)=tv else return false; if(!s.equals_with_len("A",1u)) { return fail("basic-text",0u64) } } else { return fail("basic-text-missing",0u64) }
        var si=retained_style_index(&t,id)
        if(si is Option.Some) { var Some(v)=si else return false; if(v == 0u32) { return fail("basic-style",0u64) } } else { return fail("basic-style-missing",0u64) }
        var sl=retained_semantics_label(&t,id)
        if(sl is Option.Some) { var Some(v)=sl else return false; if(!v.equals_with_len("alpha",5u)) { return fail("basic-semantics",0u64) } } else { return fail("basic-semantics-missing",0u64) }
    } else { return fail("basic-child",0u64) }
    // State (including text edit/custom/animation/scroll/focus/pressed) survives move.
    retained_state_set_hover(&mut t, ka, true)
    retained_state_set_focus(&mut t, ka, true)
    retained_state_set_pressed(&mut t, ka, true)
    retained_state_set_scroll(&mut t, ka, 4.0f, 8.0f)
    retained_state_set_animation(&mut t, ka, 37i64)
    retained_state_set_custom(&mut t, ka, 99i64)
    retained_state_set_text(&mut t, ka, "draft")
    reset_view_builder(&mut b)
    var r2 = view_root(&b)
    var c2 = b.text(kc, "C", Style.new()); b.child(r2,c2)
    var b2 = b.text(kb, "B", Style.new()); b.child(r2,b2)
    var a2 = b.text(ka, "A2", Style.new()); b.child(r2,a2)
    var rr2 = reconcile(&mut t, &mut b)
    if(rr2 is Result.Err) { return fail("move-reconcile",0u64) }
    var hs = retained_state_scroll(&t, ka)
    if(hs is Option.Some) { var Some(p)=hs else return false; if(p.x != 4.0f || p.y != 8.0f) { return fail("state-scroll",0u64) } } else { return fail("state-scroll-missing",0u64) }
    var an = retained_state_animation(&t, ka)
    if(an is Option.Some) { var Some(v)=an else return false; if(v != 37i64) { return fail("state-animation",0u64) } } else { return fail("state-animation-missing",0u64) }
    var cu = retained_state_custom(&t, ka)
    if(cu is Option.Some) { var Some(v)=cu else return false; if(v != 99i64) { return fail("state-custom",0u64) } } else { return fail("state-custom-missing",0u64) }
    var tx2 = retained_state_text(&t, ka)
    if(tx2 is Option.Some) { var Some(v)=tx2 else return false; if(!v.equals_with_len("draft",5u)) { return fail("state-text",0u64) } } else { return fail("state-text-missing",0u64) }
    var sr = retained_stats(&t)
    if(sr.moved == 0u32 || sr.state_moved == 0u32 || sr.updated == 0u32) { return fail("state-moved-stat",0u64) }
    var moved_a = retained_child_at(&t, retained_root(&t), 2u32)
    if(moved_a is Option.Some) { var Some(id)=moved_a else return false; var tv=retained_text(&t,id); if(tv is Option.Some) { var Some(s)=tv else return false; if(!s.equals_with_len("A2",2u)) { return fail("update-text",0u64) } } else { return fail("update-text-missing",0u64) } } else { return fail("update-child",0u64) }
    return true
}

func check_duplicate_stale_remove() : bool {
    var b = new_view_builder(32u32, 100.0f, 100.0f)
    var root = view_root(&b)
    var k = key("dup")
    var a = b.text(k, "first", Style.new()); b.child(root,a)
    var d = b.text(k, "second", Style.new()); b.child(root,d)
    var t = new_retained_tree(32u32)
    var rr = reconcile(&mut t, &mut b)
    if(rr is Result.Ok) { return fail("duplicate-should-error",0u64) }
    if(rr is Result.Err) { var Err(e)=rr else return false; if(e.code() != ErrorCode.DuplicateKey) { return fail("duplicate-code",0u64) } }
    var tr = retained_root(&t)
    if(retained_child_count(&t,tr) != 1u32) { return fail("duplicate-first-wins",0u64) }
    var first = retained_child_at(&t,tr,0u32)
    if(first is Option.Some) { var Some(id)=first else return false; var tv=retained_text(&t,id); if(tv is Option.Some) { var Some(s)=tv else return false; if(!s.equals_with_len("first",5u)) { return fail("duplicate-no-steal",0u64) } } } else return fail("duplicate-child",0u64)
    retained_state_set_custom(&mut t,k,5i64)
    var forged = zeroed<NodeId>()
    if(retained_key(&t, forged) is Option.Some) { return fail("forged-key",0u64) }
    // Empty fresh tree removes all children and drops state.
    reset_view_builder(&mut b)
    var r2 = view_root(&b)
    var rr2 = reconcile(&mut t, &mut b)
    if(rr2 is Result.Err) { return fail("remove-reconcile",0u64) }
    if(retained_child_count(&t, retained_root(&t)) != 0u32) { return fail("remove-count",0u64) }
    if(retained_state_has(&t,k)) { return fail("remove-state",0u64) }
    var removed1 = retained_removed_total(&t)
    var rr3 = reconcile(&mut t, &mut b)
    if(rr3 is Result.Err) { return fail("remove-second",0u64) }
    if(retained_removed_total(&t) != removed1) { return fail("remove-once",0u64) }
    return true
}

func check_unkeyed_path() : bool {
    var b = new_view_builder(16u32, 100.0f, 100.0f)
    var root = view_root(&b)
    var u1 = b.text(zeroed<Key>(), "u1", Style.new()); b.child(root,u1)
    var u2 = b.text(zeroed<Key>(), "u2", Style.new()); b.child(root,u2)
    var t = new_retained_tree(16u32)
    var rr = reconcile(&mut t,&mut b)
    if(rr is Result.Err) { return fail("unkeyed-reconcile",0u64) }
    var tr=retained_root(&t)
    var c0=retained_child_at(&t,tr,0u32); var c1=retained_child_at(&t,tr,1u32)
    if(c0 is Option.Some && c1 is Option.Some) {
        var Some(id0)=c0 else return false; var Some(id1)=c1 else return false
        if(retained_path(&t,id0) == retained_path(&t,id1)) { return fail("path-distinct",0u64) }
    } else { return fail("unkeyed-child",0u64) }
    return true
}

func check_reorder_gate() : bool {
    var capacity = 10050u32
    var b = new_view_builder(capacity, 1280.0f, 800.0f)
    var root = view_root(&b)
    var i = 0u32
    while(i < 10000u32) { var n=b.text(key_of("r", i as u64), "x", Style.new()); b.child(root,n); i=i+1u32 }
    var t = new_retained_tree(capacity)
    var rr = reconcile(&mut t,&mut b)
    if(rr is Result.Err) { return fail("reorder-initial",0u64) }
    retained_state_set_custom(&mut t, key_of("r",0u64), 77i64)
    var times : [5]i64
    var run = 0
    while(run < 5) {
        var fb = new_view_builder(capacity, 1280.0f, 800.0f)
        var froot = view_root(&fb)
        var j = 0u32
        while(j < 10000u32) { var n=fb.text(key_of("r", (9999u32-j) as u64), "x", Style.new()); fb.child(froot,n); j=j+1u32 }
        var start = std::chrono::Instant.now()
        var r = reconcile(&mut t,&mut fb)
        times[run] = start.elapsed().as_micros()
        if(r is Result.Err) { return fail("reorder-run",run as u64) }
        var k0 = key_at(&t, retained_root(&t), 0u32)
        if(k0 is Option.Some) { var Some(v)=k0 else return false; if(!v.equals_key(key_of("r",9999u64))) { return fail("reorder-order",run as u64) } } else return fail("reorder-child",run as u64)
        run = run + 1
    }
    var sorted : [5]i64 = times
    var x = 0
    while(x < 5) { var y=x+1; while(y<5) { if(sorted[y] < sorted[x]) { var q=sorted[x]; sorted[x]=sorted[y]; sorted[y]=q }; y=y+1 }; x=x+1 }
    var p50=sorted[2]; var p95=sorted[4]
    var cs = retained_state_custom(&t, key_of("r",0u64))
    if(cs is Option.Some) { var Some(v)=cs else return false; if(v != 77i64) { return fail("reorder-state",0u64) } } else { return fail("reorder-state-missing",0u64) }
    unsafe { printf("reconcile reorder p50_us=%ld p95_us=%ld live=%u\n", p50, p95, retained_live(&t)) }
    if(release_mode() && is_llvm() && (p50 > 1000i64 || p95 > 1500i64)) {
        unsafe { printf("reconcile release gate BLOCKER p50=%ld limit=1000 p95=%ld limit=1500\n", p50, p95) }
        return false
    }
    return true
}

struct Rng { var state : u64
    func next(&mut self) : u64 { var x=self.state; x=x^(x<<13u64); x=x^(x>>7u64); x=x^(x<<17u64); self.state=x; return x }
    func u32(&mut self) : u32 { return (self.next() >> 32u64) as u32 }
}

func check_differential() : bool {
    var b = new_view_builder(64u32, 100.0f, 100.0f)
    var t = new_retained_tree(64u32)
    var rng = Rng { state : 0x9E3779B97F4A7C15u64 }
    var model = vector<Key>()
    model.push(key("a")); model.push(key("b")); model.push(key("c"))
    var i = 0u64
    while(i < 10u64) {
        var op = rng.u32() % 6u32
        var duplicate = false
        if(op == 0u32 && model.size() < 8u) { model.push(key_of("d", i)) }
        else if(op == 1u32 && model.size() > 1u) { var at=(rng.u32() as size_t)%model.size(); model.remove(at) }
        else if(op == 2u32 && model.size() > 1u) { var at=(rng.u32() as size_t)%model.size(); var jt=(rng.u32() as size_t)%model.size(); var tmp=unsafe(model.get_ptr(at)[0]); model.set(at, unsafe(model.get_ptr(jt)[0])); model.set(jt,tmp) }
        else if(op == 3u32 && model.size() > 1u) { var at=(rng.u32() as size_t)%model.size(); var tmp=unsafe(model.get_ptr(at)[0]); model.remove(at); model.push(tmp) }
        else if(op == 4u32) { duplicate = true }
        else { if(retained_key(&t, zeroed<NodeId>()) is Option.Some) { return fail("differential-stale",i) } }
        reset_view_builder(&mut b)
        var root = view_root(&b)
        build_prefix(&mut b, root, model.as_span(), duplicate)
        var custom = (i % 13u64) as i64
        var j = 0u32
        while(j < model.size() as u32) { retained_state_set_custom(&mut t, unsafe(model.get_ptr(j as size_t)[0]), custom); j=j+1u32 }
        var rr = reconcile(&mut t, &mut b)
        if(duplicate) { if(rr is Result.Ok) { return fail("differential-dup-ok",i) } }
        else if(rr is Result.Err) { return fail("differential-reconcile",i) }
        var tr=retained_root(&t)
        if(retained_child_count(&t,tr) != model.size() as u32) { return fail("differential-count",i) }
        var k=0u32
        while(k < model.size() as u32) {
            var got = retained_child_at(&t,tr,k)
            if(got is Option.Some) {
                var Some(id)=got else return fail("differential-pattern",i)
                var gk=retained_key(&t,id)
                if(gk is Option.Some) {
                    var Some(v)=gk else return fail("differential-pattern",i)
                    if(!v.equals_key(unsafe(model.get_ptr(k as size_t)[0]))) { return fail("differential-key",i) }
                } else {
                    unsafe { printf("differential key missing i=%lu k=%u id=%u\n", i, k, id.index()) }
                    return fail("differential-key-missing",i)
                }
            } else { return fail("differential-child",i) }
            var cv=retained_state_custom(&t, unsafe(model.get_ptr(k as size_t)[0]))
            if(cv is Option.Some) { var Some(v)=cv else return fail("differential-pattern",i); if(v != custom) { return fail("differential-state",i) } } else { return fail("differential-state-missing",i) }
            k=k+1u32
        }
        i=i+1u64
    }
    unsafe { printf("reconcile differential ops=10000 live=%u\n", retained_live(&t)) }
    return true
}

public func run() : bool {
    var ok = true
    if(!check_basic()) { unsafe { printf("reconcile branch FAIL basic\n") }; ok=false }
    if(!check_duplicate_stale_remove()) { unsafe { printf("reconcile branch FAIL dup/stale/remove\n") }; ok=false }
    if(!check_unkeyed_path()) { unsafe { printf("reconcile branch FAIL unkeyed\n") }; ok=false }
    if(!check_differential()) { unsafe { printf("reconcile branch FAIL differential\n") }; ok=false }
    if(!check_reorder_gate()) { unsafe { printf("reconcile branch FAIL reorder\n") }; ok=false }
    return ok
}

}
