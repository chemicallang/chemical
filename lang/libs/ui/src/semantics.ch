// P1.12 semantics tree and audit core. No protocol server or platform bridge.
using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::unordered_map;
using std::vector;

public enum SemanticsChange { Add, Update, Remove }

@direct_init
public struct SemanticsUpdate {
    var change : SemanticsChange
    var node : NodeId
    var key : Key
    var semantics : Semantics
    var bounds : Rect
}

public enum AuditSeverity { Info, Warning, Error }

@direct_init
public struct AuditFinding {
    var code : string_view
    var severity : AuditSeverity
    var node : NodeId
    var key : Key
    var source : Option<SourceLocation>
    var message : string_view
    var fix : string_view
}

@direct_init
public struct AuditReport {
    var findings : vector<AuditFinding>
    var errors : u32
    var warnings : u32
    var infos : u32
    var passed : bool
}

@direct_init
public struct AuditOptions {
    var min_target : float
    var preferred_target : float
    var audit_contrast : bool
    var audit_headings : bool
    @make func make() : AuditOptions {
        return AuditOptions { min_target : 24.0f, preferred_target : 44.0f,
            audit_contrast : true, audit_headings : true }
    }
}

public namespace ui_internal {

@direct_init
public struct SemanticsNode {
    var id : NodeId
    var key : Key
    var parent : i32
    var depth : u32
    var order : u32
    var bounds : Rect
    var device_bounds : Rect
    var semantics : Semantics
    var test_id : string_view
    var clipped : bool
    var focusable : bool
    var scroll_offset : Point
    var foreground : Color
    var background : Color
    var text_size : float
    var text_bold : bool
}

@direct_init
public struct SemanticsTree {
    var nodes : vector<SemanticsNode>
    var viewport : Size
    var focused_key : Option<Key>
    var text_arena : FrameArena
    public func new() : SemanticsTree {
        return SemanticsTree { nodes : vector<SemanticsNode>(), viewport : Size.zero(),
            focused_key : Option.None<Key>(), text_arena : FrameArena.new(1024u * 1024u) }
    }
}
func sem_copy(tree : &mut SemanticsTree, value : string_view) : string_view {
    if(value.size() == 0u) { return "" }
    var p = tree.text_arena.alloc(value.size(), 1u)
    if(p is Result.Err) { return "" }
    var Ok(dst) = p else return ""
    unsafe { memcpy(dst as *mut void, value.data() as *mut void, value.size()) }
    return unsafe(std::string_view(dst as *char, value.size()))
}

public func semantics_role_name(r : Role) : string_view { return sem_role_named(r) }
public func semantics_empty() : Semantics { return empty_semantics() }
public func semantics_role_color(r : Role) : Color {
    switch(r) {
        Role.Button, Role.Link, Role.Checkbox, Role.Radio, Role.Switch => return Color.new(37u8,99u8,235u8)
        Role.Text, Role.Label, Role.Heading => return Color.new(21u8,128u8,61u8)
        Role.Image => return Color.new(217u8,119u8,6u8)
        Role.Dialog, Role.AlertDialog, Role.Alert => return Color.new(220u8,38u8,38u8)
        Role.ScrollView, Role.List, Role.Table, Role.Grid => return Color.new(124u8,58u8,237u8)
        default => return Color.new(100u8,116u8,139u8)
    }
}
public func semantics_overlay_build(tree : &SemanticsTree, theme : &Theme) : DisplayState {
    var ds = DisplayState.new((tree.nodes.size() * 2u) + 4u)
    var p = new_painter(&raw mut ds.list, &raw mut theme, tree.viewport)
    p.rect(Rect.new(0.0f,0.0f,tree.viewport.width,tree.viewport.height), theme.color(ColorToken.Surface))
    var i : size_t = 0u
    while(i < tree.nodes.size()) {
        var n = unsafe(tree.nodes.get_ptr(i)[0])
        var c = semantics_role_color(n.semantics.role)
        p.rect(Rect.new(n.bounds.x, n.bounds.y, n.bounds.w, 2.0f), c)
        p.rect(Rect.new(n.bounds.x, n.bounds.y2() - 2.0f, n.bounds.w, 2.0f), c)
        p.rect(Rect.new(n.bounds.x, n.bounds.y, 2.0f, n.bounds.h), c)
        p.rect(Rect.new(n.bounds.x2() - 2.0f, n.bounds.y, 2.0f, n.bounds.h), c)
        // Text labels are carried in the semantics snapshot; production glyph
        // rasterization is P2.2, so the PNG visualizes bounds + role colors.
        i = i + 1u
    }
    return ds
}

func sem_role_interactive(r : Role) : bool {
    switch(r) {
        Role.Button, Role.Checkbox, Role.Radio, Role.Switch, Role.Slider, Role.TextField, Role.MultilineTextField,
        Role.Combobox, Role.Listbox, Role.Option, Role.Spinbutton, Role.SearchBox, Role.Link, Role.Tab, Role.MenuItem,
        Role.MenuButton, Role.TreeItem, Role.Cell, Role.ColumnHeader => return true
        default => return false
    }
}

func sem_role_named(r : Role) : string_view {
    switch(r) {
        Role.Button => return "Button"
        Role.Checkbox => return "Checkbox"
        Role.Radio => return "Radio"
        Role.Switch => return "Switch"
        Role.Slider => return "Slider"
        Role.TextField => return "TextField"
        Role.MultilineTextField => return "MultilineTextField"
        Role.Combobox => return "Combobox"
        Role.Listbox => return "Listbox"
        Role.Option => return "Option"
        Role.Spinbutton => return "Spinbutton"
        Role.SearchBox => return "SearchBox"
        Role.Heading => return "Heading"
        Role.Text => return "Text"
        Role.Image => return "Image"
        Role.Link => return "Link"
        Role.List => return "List"
        Role.ListItem => return "ListItem"
        Role.Table => return "Table"
        Role.Grid => return "Grid"
        Role.Row => return "Row"
        Role.Cell => return "Cell"
        Role.ColumnHeader => return "ColumnHeader"
        Role.Tree => return "Tree"
        Role.TreeItem => return "TreeItem"
        Role.Tab => return "Tab"
        Role.TabList => return "TabList"
        Role.TabPanel => return "TabPanel"
        Role.Menu => return "Menu"
        Role.MenuItem => return "MenuItem"
        Role.MenuBar => return "MenuBar"
        Role.MenuButton => return "MenuButton"
        Role.Toolbar => return "Toolbar"
        Role.Banner => return "Banner"
        Role.Main => return "Main"
        Role.Navigation => return "Navigation"
        Role.Dialog => return "Dialog"
        Role.AlertDialog => return "AlertDialog"
        Role.Alert => return "Alert"
        Role.Tooltip => return "Tooltip"
        Role.ProgressBar => return "ProgressBar"
        Role.Status => return "Status"
        Role.Group => return "Group"
        Role.Form => return "Form"
        Role.Label => return "Label"
        Role.Separator => return "Separator"
        Role.ScrollView => return "ScrollView"
        Role.Canvas => return "Canvas"
        Role.Custom => return "Custom"
        Role.Unknown => return "Unknown"
    }
    return "Unknown"
}

func sem_clip_rect(vp : Size) : Rect { return Rect.new(0.0f, 0.0f, vp.width, vp.height) }

func sem_node_at(layout : &LayoutState, i : u32) : LayoutNode {
    unsafe { return layout.result.nodes.get_ptr(i as size_t)[0] }
}

func sem_meta_of(b : &ViewBuilder, id : NodeId) : NodeMeta {
    if((id.index() as size_t) >= b.node_count_value + 1u32) { return empty_meta() }
    unsafe {
        var vn = b.node_base + (id.index() - 1u32)
        if(vn.meta_index == VIEW_NONE || (vn.meta_index as size_t) >= b.metas.size()) { return empty_meta() }
        return b.metas.get_ptr(vn.meta_index as size_t)[0]
    }
}

func sem_visit(tree : &mut SemanticsTree, b : &ViewBuilder, layout : &LayoutState,
               theme : &Theme, index : u32, parent : i32, depth : u32,
               parent_tr : Transform2D, parent_clip : Rect, has_parent_clip : bool,
               focus_key : Option<Key>) {
    if(index == 0u32 || index as size_t >= layout.result.nodes.size()) { return }
    var n = sem_node_at(layout, index)
    var meta = sem_meta_of(b, n.id)
    var sem = meta.semantics
    var has_explicit = meta.has_semantics
    var focusable = meta.focusable
    var disabled = meta.disabled || sem.states.disabled
    var role = sem.role
    var label = sem.label
    var alt = sem.alt
    if(!has_explicit) {
        sem = empty_semantics()
        if(n.kind == ViewKind.Text || n.kind == ViewKind.RichText) {
            role = Role.Text; label = n.text
        } else if(n.kind == ViewKind.Image || n.kind == ViewKind.Icon) {
            role = Role.Image
        } else if(n.kind == ViewKind.Scroll) {
            role = Role.ScrollView
        } else if(focusable) {
            role = Role.Unknown
        }
        sem.role = role; sem.label = label; sem.alt = alt
    }
    if(sem.states.hidden) { return }
    var tr = parent_tr
    if(meta.has_transform) { tr = parent_tr.multiply(meta.transform) }
    var clip = parent_clip
    var has_clip = has_parent_clip
    if(n.has_clip) {
        var c = n.clip
        if(has_clip) { c = display_rect_intersect(c, parent_clip, true) }
        clip = c; has_clip = true
    }
    if(n.spec.overflow == Overflow.Clip) {
        var c = n.rect
        if(has_clip) { c = display_rect_intersect(c, clip, true) }
        clip = c; has_clip = true
    }
    var dev = display_transform_rect(tr, n.rect)
    var dev_clip = Rect.zero()
    if(has_clip) { dev_clip = display_transform_rect(tr, clip) }
    var clipped = has_clip && !dev.intersects(dev_clip)
    var include = has_explicit || focusable || role == Role.Text || role == Role.Image || role == Role.ScrollView
    if(clipped && !focusable && !sem_role_interactive(role)) { include = false }
    if(include) {
        sem.states.focusable = focusable
        sem.states.disabled = disabled
        sem.states.focused = false
        switch(focus_key) {
            Some(fk) => { sem.states.focused = fk.equals_key(n.key) }
            None() => { }
        }
        if(focusable) { sem.actions.focus = true }
        if(sem_role_interactive(role) && !disabled) { sem.actions.press = true }
        var rs = style_resolve(Style { id : n.style_id }, theme)
        var bg = rs.background
        if(bg.a == 0u8) { bg = theme.color(ColorToken.Surface) }
        var fg = rs.foreground
        if(fg.a == 0u8) { fg = theme.color(ColorToken.OnSurface) }
        sem.label = sem_copy(tree, sem.label)
        sem.description = sem_copy(tree, sem.description)
        sem.value = sem_copy(tree, sem.value)
        sem.alt = sem_copy(tree, sem.alt)
        sem.lang = sem_copy(tree, sem.lang)
        sem.placeholder = sem_copy(tree, sem.placeholder)
        sem.error = sem_copy(tree, sem.error)
        var sid = sem_copy(tree, meta.test_id)
        var node = SemanticsNode {
            id : n.id, key : n.key, parent : parent, depth : depth, order : tree.nodes.size() as u32,
            bounds : n.rect, device_bounds : dev, semantics : sem, test_id : sid,
            clipped : clipped, focusable : focusable,
            scroll_offset : Point { x : n.scroll_x, y : n.scroll_y },
            foreground : fg, background : bg, text_size : rs.text.size,
            text_bold : rs.text.weight == FontWeight.Bold || rs.text.weight == FontWeight.Semibold
        }
        tree.nodes.push(node)
        parent = (tree.nodes.size() - 1u) as i32
        depth = depth + 1u32
    }
    var cur = n.first_child
    while(cur != 0u32) {
        var cp = sem_node_at(layout, cur)
        if(cp.kind != ViewKind.Overlay) { sem_visit(tree, b, layout, theme, cur, parent, depth, tr, clip, has_clip, focus_key) }
        cur = cp.next_sibling
    }
    var layer = 0u32
    while(layer < 5u32) {
        cur = n.first_child
        while(cur != 0u32) {
            var cp = sem_node_at(layout, cur)
            if(cp.kind == ViewKind.Overlay && (cp.overlay_layer as u32) == layer) {
                sem_visit(tree, b, layout, theme, cur, parent, depth, tr, Rect.zero(), false, focus_key)
            }
            cur = cp.next_sibling
        }
        layer = layer + 1u32
    }
}

public func semantics_build(b : &ViewBuilder, layout : &LayoutState, theme : &Theme,
                            viewport : Size, focus_key : Option<Key>) : SemanticsTree {
    var tree = SemanticsTree.new()
    tree.viewport = viewport
    tree.focused_key = focus_key
    sem_visit(&mut tree, b, layout, theme, layout.result.root_index, -1i32, 0u32,
              Transform2D.identity(), sem_clip_rect(viewport), true, focus_key)
    return tree
}

public func semantics_count(tree : &SemanticsTree) : u32 { return tree.nodes.size() as u32 }
public func semantics_node_at(tree : &SemanticsTree, i : u32) : SemanticsNode {
    return unsafe(tree.nodes.get_ptr(i as size_t)[0])
}
public func semantics_find_role(tree : &SemanticsTree, role : Role) : Option<NodeId> {
    var i : size_t = 0u
    while(i < tree.nodes.size()) { var n = unsafe(tree.nodes.get_ptr(i)[0]); if(n.semantics.role == role) { return Option.Some<NodeId>(n.id) }; i=i+1u }
    return Option.None<NodeId>()
}
public func semantics_find_name(tree : &SemanticsTree, name : string_view) : Option<NodeId> {
    var i : size_t = 0u
    while(i < tree.nodes.size()) { var n = unsafe(tree.nodes.get_ptr(i)[0]); if(n.semantics.label.equals(&name)) { return Option.Some<NodeId>(n.id) }; i=i+1u }
    return Option.None<NodeId>()
}
public func semantics_find_test_id(tree : &SemanticsTree, id : string_view) : Option<NodeId> {
    var i : size_t = 0u
    while(i < tree.nodes.size()) { var n = unsafe(tree.nodes.get_ptr(i)[0]); if(n.test_id.equals(&id)) { return Option.Some<NodeId>(n.id) }; i=i+1u }
    return Option.None<NodeId>()
}
public func semantics_bounds(tree : &SemanticsTree, id : NodeId) : Option<Rect> {
    var i : size_t = 0u
    while(i < tree.nodes.size()) { var n = unsafe(tree.nodes.get_ptr(i)[0]); if(n.id.index()==id.index() && n.id.generation()==id.generation()) { return Option.Some<Rect>(n.bounds) }; i=i+1u }
    return Option.None<Rect>()
}
public func semantics_focused(tree : &SemanticsTree) : Option<NodeId> {
    var i : size_t = 0u
    while(i < tree.nodes.size()) { var n = unsafe(tree.nodes.get_ptr(i)[0]); if(n.semantics.states.focused) { return Option.Some<NodeId>(n.id) }; i=i+1u }
    return Option.None<NodeId>()
}
public func semantics_scroll_offset(tree : &SemanticsTree, key : Key) : Option<Point> {
    var i : size_t = 0u
    while(i < tree.nodes.size()) { var n = unsafe(tree.nodes.get_ptr(i)[0]); if(n.key.equals_key(key)) { return Option.Some<Point>(n.scroll_offset) }; i=i+1u }
    return Option.None<Point>()
}

func sem_escape(out : &mut string, s : string_view) {
    var i : size_t = 0u
    while(i < s.size()) {
        var oc = s.get(i)
        if(oc.is_some()) {
            switch(oc) {
                Some(c) => {
                    if((c as u8) == 34u8) { out.append_view("\\\"") }
                    else if((c as u8) == 92u8) { out.append_view("\\\\") }
                    else if((c as u8) == 10u8) { out.append_view("\\n") }
                    else { out.append(c) }
                }
                None() => { }
            }
        }
        i = i + 1u
    }
}
func sem_bool(v : bool) : string_view { if(v) { return "1" } return "0" }
func sem_redact(secure : bool, value : string_view) : string_view {
    if(secure && value.size() > 0u) { return "[redacted]" }
    return value
}
public func semantics_snapshot(tree : &SemanticsTree) : string {
    var out = string()
    out.append_view("{\"nodes\":[")
    var i : size_t = 0u
    while(i < tree.nodes.size()) {
        var n = unsafe(tree.nodes.get_ptr(i)[0])
        if(i > 0u) { out.append(',') }
        out.append_view("{\"order\":"); out.append_integer(n.order as int)
        out.append_view(",\"role\":\""); out.append_view(sem_role_named(n.semantics.role)); out.append('"')
        out.append_view(",\"label\":\""); sem_escape(&mut out, sem_redact(n.semantics.secure, n.semantics.label)); out.append('"')
        out.append_view(",\"value\":\""); sem_escape(&mut out, sem_redact(n.semantics.secure, n.semantics.value)); out.append('"')
        out.append_view(",\"test\":\""); sem_escape(&mut out, n.test_id); out.append('"')
        out.append_view(",\"bounds\":["); out.append_float(n.bounds.x,2); out.append(','); out.append_float(n.bounds.y,2); out.append(','); out.append_float(n.bounds.w,2); out.append(','); out.append_float(n.bounds.h,2); out.append(']')
        out.append_view(",\"focusable\":"); out.append_view(sem_bool(n.semantics.states.focusable))
        out.append_view(",\"focused\":"); out.append_view(sem_bool(n.semantics.states.focused))
        out.append_view(",\"disabled\":"); out.append_view(sem_bool(n.semantics.states.disabled))
        out.append_view(",\"clipped\":"); out.append_view(sem_bool(n.clipped))
        out.append('}')
        i = i + 1u
    }
    out.append_view("]}")
    return out
}

func sem_lum_channel(v : u8) : float {
    // Deterministic 8-bit sRGB approximation; exact ICC conversion is not
    // required by the audit contract and no float power helper is linked.
    return (v as float) / 255.0f
}
func sem_luminance(c : Color) : float {
    return 0.2126f * sem_lum_channel(c.r) + 0.7152f * sem_lum_channel(c.g) + 0.0722f * sem_lum_channel(c.b)
}
func sem_contrast(a : Color, b : Color) : float {
    var l1 = sem_luminance(a); var l2 = sem_luminance(b)
    if(l1 < l2) { var t = l1; l1 = l2; l2 = t }
    return (l1 + 0.05f) / (l2 + 0.05f)
}

func sem_finding(report : &mut AuditReport, code : string_view, sev : AuditSeverity, n : &SemanticsNode, message : string_view, fix : string_view) {
    report.findings.push(AuditFinding { code : code, severity : sev, node : n.id, key : n.key,
        source : Option.None<SourceLocation>(), message : message, fix : fix })
    if(sev == AuditSeverity.Error) { report.errors = report.errors + 1u32 }
    else if(sev == AuditSeverity.Warning) { report.warnings = report.warnings + 1u32 }
    else { report.infos = report.infos + 1u32 }
}

public func semantics_audit(tree : &SemanticsTree, opts : AuditOptions) : AuditReport {
    var report = AuditReport { findings : vector<AuditFinding>(), errors : 0u32, warnings : 0u32, infos : 0u32, passed : true }
    var heading_level : u8 = 0u8
    var i : size_t = 0u
    while(i < tree.nodes.size()) {
        var n = unsafe(tree.nodes.get_ptr(i)[0])
        var interactive = sem_role_interactive(n.semantics.role)
        if(interactive && n.semantics.label.size() == 0u && n.semantics.value.size() == 0u) {
            sem_finding(&mut report, "UI-A11Y-001", AuditSeverity.Error, &n, "interactive node has no accessible name", "set a label or visible text")
        }
        if(n.focusable && n.semantics.role == Role.Unknown) {
            sem_finding(&mut report, "UI-A11Y-002", AuditSeverity.Error, &n, "focusable node has no semantic role", "assign a role and name")
        }
        if(n.focusable && n.semantics.states.disabled) {
            sem_finding(&mut report, "UI-A11Y-003", AuditSeverity.Error, &n, "focusable node is disabled", "remove focusability or enable the node")
        }
        if(interactive && n.clipped) {
            sem_finding(&mut report, "UI-A11Y-004", AuditSeverity.Warning, &n, "interactive node is clipped or outside its clip", "keep interactive targets inside their viewport/clip")
        }
        if(interactive && (n.bounds.w < opts.min_target || n.bounds.h < opts.min_target)) {
            sem_finding(&mut report, "UI-A11Y-005", AuditSeverity.Error, &n, "interactive target is below the 24px minimum", "enlarge the target to at least 24x24 logical px")
        } else if(interactive && (n.bounds.w < opts.preferred_target || n.bounds.h < opts.preferred_target)) {
            sem_finding(&mut report, "UI-A11Y-006", AuditSeverity.Warning, &n, "interactive target is below the preferred 44px target", "consider at least 44x44 logical px")
        }
        if(opts.audit_contrast && n.semantics.label.size() > 0u && (interactive || n.semantics.role == Role.Text || n.semantics.role == Role.Heading || n.semantics.role == Role.Label || n.semantics.role == Role.Link)) {
            var large = n.text_size >= 18.66f || (n.text_bold && n.text_size >= 14.0f)
            var need = 4.5f
            if(large) { need = 3.0f }
            if(sem_contrast(n.foreground, n.background) < need) {
                sem_finding(&mut report, "UI-A11Y-007", AuditSeverity.Error, &n, "text contrast is below the required ratio", "raise foreground/background contrast")
            }
        }
        if(n.semantics.role == Role.Image && n.semantics.alt.size() == 0u && n.semantics.label.size() == 0u) {
            sem_finding(&mut report, "UI-A11Y-008", AuditSeverity.Error, &n, "image or icon has no alt/name", "provide alt text or a label")
        }
        if(n.semantics.secure && n.semantics.value.size() > 0u) {
            sem_finding(&mut report, "UI-A11Y-009", AuditSeverity.Error, &n, "secure text leaks a value into semantics", "redact secure values before publishing semantics")
        }
        if(opts.audit_headings && n.semantics.role == Role.Heading) {
            if(n.semantics.level == 0u8 || (heading_level == 0u8 && n.semantics.level > 1u8) || (heading_level > 0u8 && n.semantics.level > heading_level + 1u8)) {
                sem_finding(&mut report, "UI-A11Y-010", AuditSeverity.Warning, &n, "heading hierarchy skips a level", "use the next heading level")
            }
            heading_level = n.semantics.level
        }
        i = i + 1u
    }
    // Duplicate key/test-id checks are emitted once per duplicated pair.
    var di : size_t = 0u
    while(di < tree.nodes.size()) {
        var a = unsafe(tree.nodes.get_ptr(di)[0])
        var dj = di + 1u
        while(dj < tree.nodes.size()) {
            var bb = unsafe(tree.nodes.get_ptr(dj)[0])
            if(a.key.equals_key(bb.key)) {
                sem_finding(&mut report, "UI-A11Y-011", AuditSeverity.Error, &a, "duplicate key in semantics tree", "give siblings unique keys")
            }
            if(a.test_id.size() > 0u && a.test_id.equals(&bb.test_id)) {
                sem_finding(&mut report, "UI-A11Y-012", AuditSeverity.Error, &a, "duplicate test id", "make test ids unique")
            }
            dj = dj + 1u
        }
        di = di + 1u
    }
    report.passed = report.errors == 0u32
    return report
}

public func semantics_update_at(updates : &[SemanticsUpdate], i : u32) : SemanticsUpdate {
    return unsafe(updates.data()[i as size_t])
}
public func semantics_diff(prev : &SemanticsTree, next : &SemanticsTree) : vector<SemanticsUpdate> {
    var out = vector<SemanticsUpdate>()
    var i : size_t = 0u
    while(i < next.nodes.size()) {
        var n = unsafe(next.nodes.get_ptr(i)[0])
        var found = false
        var j : size_t = 0u
        while(j < prev.nodes.size()) {
            var p = unsafe(prev.nodes.get_ptr(j)[0])
            if(p.key.equals_key(n.key)) {
                found = true
                if(!p.semantics.label.equals(&n.semantics.label) || p.semantics.role != n.semantics.role || !(p.bounds.x == n.bounds.x && p.bounds.y == n.bounds.y && p.bounds.w == n.bounds.w && p.bounds.h == n.bounds.h) || p.clipped != n.clipped) {
                    out.push(SemanticsUpdate { change : SemanticsChange.Update, node : n.id, key : n.key, semantics : n.semantics, bounds : n.bounds })
                }
                j = prev.nodes.size()
            }
            j = j + 1u
        }
        if(!found) { out.push(SemanticsUpdate { change : SemanticsChange.Add, node : n.id, key : n.key, semantics : n.semantics, bounds : n.bounds }) }
        i = i + 1u
    }
    i = 0u
    while(i < prev.nodes.size()) {
        var p = unsafe(prev.nodes.get_ptr(i)[0])
        var found = false
        var j : size_t = 0u
        while(j < next.nodes.size()) { var n = unsafe(next.nodes.get_ptr(j)[0]); if(n.key.equals_key(p.key)) { found = true; j = next.nodes.size() } else { j = j + 1u } }
        if(!found) { out.push(SemanticsUpdate { change : SemanticsChange.Remove, node : p.id, key : p.key, semantics : p.semantics, bounds : p.bounds }) }
        i = i + 1u
    }
    return out
}

public func audit_error_count(r : &AuditReport) : u32 { return r.errors }
public func audit_warning_count(r : &AuditReport) : u32 { return r.warnings }
public func audit_finding_count(r : &AuditReport) : u32 { return r.findings.size() as u32 }
public func audit_finding(r : &AuditReport, i : u32) : AuditFinding { return unsafe(r.findings.get_ptr(i as size_t)[0]) }
public func audit_passed(r : &AuditReport) : bool { return r.passed }

} // namespace ui_internal
