
// Explicit HTML escaping. The #html macro does NOT auto-escape interpolated values,
// so callers must escape any untrusted / user-provided string before embedding it in
// markup: #html { <div>{escape_html(userValue)}</div> }. Escaping is explicit to keep
// the framework fast and avoid escaping values that are already safe (e.g. a single
// *char placed into <head>).
public func escape_html(value : *char) : std::string {
    var out = std::string()
    if(value == null) { return out }
    var i : size_t = 0
    while(value[i] != '\0') {
        var c = value[i]
        if(c == '<') { out.append_view(std::string_view("&lt;")) }
        else if(c == '>') { out.append_view(std::string_view("&gt;")) }
        else if(c == '&') { out.append_view(std::string_view("&amp;")) }
        else if(c == '"') { out.append_view(std::string_view("&quot;")) }
        else if(c == '\'') { out.append_view(std::string_view("&#39;")) }
        else { out.append(c) }
        i = i + 1
    }
    return out
}

public func escape_html_view(value : std::string_view) : std::string {
    var out = std::string()
    var i : size_t = 0
    while(i < value.size()) {
        var c = value.get(i)
        if(c == '<') { out.append_view(std::string_view("&lt;")) }
        else if(c == '>') { out.append_view(std::string_view("&gt;")) }
        else if(c == '&') { out.append_view(std::string_view("&amp;")) }
        else if(c == '"') { out.append_view(std::string_view("&quot;")) }
        else if(c == '\'') { out.append_view(std::string_view("&#39;")) }
        else { out.append(c) }
        i = i + 1
    }
    return out
}

public struct HtmlPage {

    var pageHead : std::string

    var pageHtml : std::string

    var pageCss : std::string

    var pageJs : std::string

    var pageHeadJs : std::string

    var pageJsEnd : std::string
    var js_hoist_pos : ubigint = 0

    // When true, a generated universal component server function emits only its
    // client JS (its `require_component` block + hoisting) and skips SSR markup.
    // The converter sets this around the child server-function call it makes
    // during the client-JS pass, so a nested component's subtree is server
    // rendered exactly once (in the later HTML pass) instead of twice. Without
    // it, SSR work grows exponentially with component-tree depth.
    var render_js_only : bool = false

    // we track which classes are done through this unordered map
    // TODO using ubigint, instead need to use size_t
    var doneClasses : std::unordered_map<ubigint, bool>

    // track random CSS classes (for dynamic values) to prevent duplicates
    var doneRandomClasses : std::unordered_map<ubigint, bool>

    var doneComponents : std::unordered_map<ubigint, bool>

    func getHead(&self) : std::string_view {
        return pageHead.to_view()
    }

    func getHtml(&self) : std::string_view {
        return pageHtml.to_view();
    }

    func getCss(&self) : std::string_view {
        return pageCss.to_view()
    }

    func getJs(&self) : std::string_view {
        return pageJs.to_view()
    }

    func getHeadJs(&self) : std::string_view {
        return pageHeadJs.to_view()
    }

    func append_html(&mut self, value : *char, len : size_t) {
        pageHtml.append_with_len(value, len);
    }

    func append_html_char_ptr(&mut self, value : *char) {
        pageHtml.append_char_ptr(value);
    }

    func append_html_char(&mut self, value : char) {
        pageHtml.append(value)
    }

    func append_html_integer(&mut self, value : bigint) {
        pageHtml.append_integer(value)
    }

    func append_html_uinteger(&mut self, value : ubigint) {
        pageHtml.append_uinteger(value)
    }

    func append_html_float(&mut self, value : float) {
        pageHtml.append_float(value, 3)
    }

    func append_html_double(&mut self, value : double) {
        pageHtml.append_double(value, 3)
    }

    func get_html_size(&self) : ubigint {
        return pageHtml.size();
    }

    func truncate_html(&mut self, size : ubigint) {
        pageHtml.resize(size);
    }

    func append_head(&mut self, value : *char, len : size_t) {
        pageHead.append_with_len(value, len);
    }

    func append_head_view(&mut self, value : &std::string_view) {
        pageHead.append_view(value);
    }

    func append_head_char_ptr(&mut self, value : *char) {
        pageHead.append_char_ptr(value);
    }

    func append_head_char(&mut self, value : char) {
        pageHead.append(value)
    }

    func append_head_integer(&mut self, value : bigint) {
        pageHead.append_integer(value)
    }

    func append_head_uinteger(&mut self, value : ubigint) {
        pageHead.append_uinteger(value)
    }

    func append_head_float(&mut self, value : float) {
        pageHead.append_float(value, 3)
    }

    func append_head_double(&mut self, value : double) {
        pageHead.append_double(value, 3)
    }

    func append_css(&mut self, value : *char, len : size_t) {
        pageCss.append_with_len(value, len);
    }

    func append_css_view(&mut self, value : &std::string_view) {
        pageCss.append_with_len(value.data(), value.size())
    }

    func require_css_hash(&self, hash : size_t) : bool {
        return !doneClasses.contains(&hash)
    }

    func set_css_hash(&mut self, hash : size_t) {
        doneClasses.insert(hash, true)
    }

    func require_component(&self, hash : size_t) : bool {
        return !doneComponents.contains(&hash)
    }

    func set_component_hash(&mut self, hash : size_t) {
        doneComponents.insert(hash, true)
    }

    func require_random_css_hash(&self, hash : size_t) : bool {
        return !doneRandomClasses.contains(&hash)
    }

    func set_random_css_hash(&mut self, hash : size_t) {
        doneRandomClasses.insert(hash, true)
    }

    func append_css_char_ptr(&mut self, value : *char) {
        pageCss.append_char_ptr(value);
    }

    func append_css_char(&mut self, value : char) {
        pageCss.append(value)
    }

    func append_css_integer(&mut self, value : bigint) {
        pageCss.append_integer(value)
    }

    func append_css_uinteger(&mut self, value : ubigint) {
        pageCss.append_uinteger(value)
    }

    func append_css_float(&mut self, value : float) {
        pageCss.append_float(value, 3)
    }

    func append_css_double(&mut self, value : double) {
        pageCss.append_double(value, 3)
    }

    func append_js(&mut self, value : *char, len : size_t) {
        pageJs.append_with_len(value, len);
    }

    func append_js_char_ptr(&mut self, value : *char) {
        pageJs.append_char_ptr(value);
    }

    // Escapes the string for embedding inside a JS string literal: quotes,
    // backslashes, newlines/control chars and the `</` sequence that could
    // otherwise break out of an inline <script> block.
    func append_js_escaped_char_ptr(&mut self, value : *char) {
        const view = std::string_view(value, strlen(value))
        appendJsEscaped(&mut pageJs, &view)
    }

    func append_js_escaped(&mut self, value : *char, len : size_t) {
        const view = std::string_view(value, len)
        appendJsEscaped(&mut pageJs, &view)
    }

    func append_js_char(&mut self, value : char) {
        pageJs.append(value)
    }

    func append_js_integer(&mut self, value : bigint) {
        pageJs.append_integer(value)
    }

    func append_js_uinteger(&mut self, value : ubigint) {
        pageJs.append_uinteger(value)
    }

    func append_js_float(&mut self, value : float) {
        pageJs.append_float(value, 3)
    }

    func append_js_double(&mut self, value : double) {
        pageJs.append_double(value, 3)
    }

    func get_js_pos(&self) : ubigint {
        return pageJs.size();
    }

    func move_js_range(&mut self, fromStart : ubigint, fromEnd : ubigint, index : ubigint) {
        if (fromStart >= fromEnd || fromEnd > pageJs.size() || index > pageJs.size()) return;
        if (index >= fromStart && index <= fromEnd) return;
        
        pageJs.reserve(pageJs.size());
        
        var range_len = fromEnd - fromStart;
        var p_buf = pageJs.mutable_data();

        var stack_buf : [1024]char;
        var p_stack = &mut stack_buf[0];

        if (index < fromStart) {
            var m_start = index;
            var m_len = fromStart - index;
            if (m_len <= range_len) {
                var temp = if (m_len <= 1024) (p_stack as *mut char) else (malloc(m_len) as *mut char);
                memcpy(temp as *mut void, (p_buf + m_start) as *void, m_len);
                memmove((p_buf + m_start) as *mut void, (p_buf + fromStart) as *void, range_len);
                memcpy((p_buf + m_start + range_len) as *mut void, temp as *void, m_len);
                if (m_len > 1024) free(temp as *mut void);
            } else {
                var temp = if (range_len <= 1024) (p_stack as *mut char) else (malloc(range_len) as *mut char);
                memcpy(temp as *mut void, (p_buf + fromStart) as *void, range_len);
                memmove((p_buf + m_start + range_len) as *mut void, (p_buf + m_start) as *void, m_len);
                memcpy((p_buf + m_start) as *mut void, temp as *void, range_len);
                if (range_len > 1024) free(temp as *mut void);
            }
        } else {
            var m_start = fromEnd;
            var m_len = index - fromEnd;
            if (range_len <= m_len) {
                var temp = if (range_len <= 1024) (p_stack as *mut char) else (malloc(range_len) as *mut char);
                memcpy(temp as *mut void, (p_buf + fromStart) as *void, range_len);
                memmove((p_buf + fromStart) as *mut void, (p_buf + m_start) as *void, m_len);
                memcpy((p_buf + fromStart + m_len) as *mut void, temp as *void, range_len);
                if (range_len > 1024) free(temp as *mut void);
            } else {
                var temp = if (m_len <= 1024) (p_stack as *mut char) else (malloc(m_len) as *mut char);
                memcpy(temp as *mut void, (p_buf + m_start) as *void, m_len);
                memmove((p_buf + fromStart + m_len) as *mut void, (p_buf + fromStart) as *void, range_len);
                memcpy((p_buf + fromStart) as *mut void, temp as *void, range_len);
                if (m_len > 1024) free(temp as *mut void);
            }
        }
    }

    func append_head_js(&mut self, value : *char, len : size_t) {
        pageHeadJs.append_with_len(value, len);
    }

    func append_head_js_char_ptr(&mut self, value : *char) {
        pageHeadJs.append_char_ptr(value);
    }

    func append_head_js_char(&mut self, value : char) {
        pageHeadJs.append(value)
    }

    func append_head_js_integer(&mut self, value : bigint) {
        pageHeadJs.append_integer(value)
    }

    func append_head_js_uinteger(&mut self, value : ubigint) {
        pageHeadJs.append_uinteger(value)
    }

    func append_head_js_float(&mut self, value : float) {
        pageHeadJs.append_float(value, 3)
    }

    func append_head_js_double(&mut self, value : double) {
        pageHeadJs.append_double(value, 3)
    }

    func appendHtmlTagStart(str : &mut std::string, lang : std::string_view = "", htmlClass : std::string_view = "") {
        if(htmlClass.empty() && lang.empty()) {
            str.append_view("<html>")
        } else if(htmlClass.empty()) {
            str.append_view("<html lang=\"")
            str.append_view(&lang)
            str.append_view("\">")
        } else if(lang.empty()) {
            str.append_view("<html class=\"")
            str.append_view(&htmlClass)
            str.append_view("\">")
        } else {
            str.append_view("<html lang=\"")
            str.append_view(&lang)
            str.append_view("\" class=\"")
            str.append_view(&htmlClass)
            str.append_view("\">")
        }
    }

    func appendBodyTagStart(str : &mut std::string, bodyClass : std::string_view = "") {
        if(bodyClass.empty()) {
            str.append_view("<body>")
        } else {
            str.append_view("<body class=\"")
            str.append_view(&bodyClass)
            str.append_view("\">")
        }
    }

    func toString(&self, lang : std::string_view = "", htmlClass : std::string_view = "", bodyClass : std::string_view = "") : std::string {
        var str = std::string()
        str.reserve(pageHead.size() + pageCss.size() + pageHtml.size() + pageHeadJs.size() + pageJs.size() + 100)
        str.append_view(std::string_view("<!DOCTYPE html>"))
        appendHtmlTagStart(&mut str, lang, htmlClass)
        str.append_view("<head>")
        str.append_string(&pageHead)
        if(!pageCss.empty()) {
            str.append_view(std::string_view("<style>"))
            str.append_string(&pageCss)
            str.append_view(std::string_view("</style>"))
        }
        if(!pageHeadJs.empty()) {
            str.append_view(std::string_view("<script>"))
            str.append_string(&pageHeadJs)
            str.append_view(std::string_view("</script>"))
        }
        str.append_view(std::string_view("</head>"))
        appendBodyTagStart(&mut str, bodyClass)
        str.append_string(&pageHtml)
        var finalizedJs = getFinalizedPageJs()
        if(!finalizedJs.empty()) {
            str.append_view(std::string_view("<script>"))
            str.append_string(&finalizedJs)
            str.append_view(std::string_view("</script>"))
        }
        str.append_view(std::string_view("</body></html>"))
        return str;
    }

    func toStringHeadOnly(&self) : std::string {
        return pageHead.copy();
    }

    func toStringHeadJsOnly(&self) : std::string {
        return pageHeadJs.copy();
    }

    func toStringHtmlOnly(&self) : std::string {
        var str = std::string()
        str.reserve(pageHtml.size())
        str.append_string(&pageHtml)
        return str;
    }

    func toStringCssOnly(&self) : std::string {
        var str = std::string()
        str.reserve(pageCss.size())
        str.append_string(&pageCss)
        return str;
    }

    func toStringJsOnly(&self) : std::string {
        return pageJs.copy()
    }

    func appendTitle(&mut self, view : &std::string_view) {
        pageHead.append_view("<title>")
        pageHead.append_view(view)
        pageHead.append_view("</title>")
    }

    func appendFavicon(&mut self, type : &std::string_view, path : &std::string_view) {
        pageHead.append_view("<link rel=\"icon\" type=\"")
        pageHead.append_view(type)
        pageHead.append_view("\" href=\"")
        pageHead.append_view(path)
        pageHead.append_view("\">")
    }

    func appendPngFavicon(&mut self, path : &std::string_view) {
        appendFavicon(std::string_view("image/png"), path)
    }

    func appendViewportMeta(&mut self) {
        pageHead.append_view("""<meta name="viewport" content="width=device-width, initial-scale=1.0">""")
    }

    func appendCharsetUTF8Meta(&mut self) {
        pageHead.append_view("""<meta charset="utf-8">""")
    }

    func defaultPrepare(&mut self) {
        appendCharsetUTF8Meta();
        appendViewportMeta();
        // Default no-op favicon so the browser does not auto-request
        // /favicon.ico (a 404 with no asset). A page that calls appendFavicon
        // later appends its own link, which takes precedence.
        pageHead.append_view(std::string_view("<link rel=\"icon\" href=\"data:,\">"));
    }

    func defaultUniversalSetup(&mut self) {
        // Hydration boundary: [data-chx-i] spans are layout-invisible
        // so their children become direct layout children of the parent.
        // This fixes table/inline contexts where a wrapper element is invalid.
        pageCss.append_view(std::string_view("[data-chx-i]{display:contents}"))
        // we must not put anything else in the head js
        // everything else must go into body js
        // universal component hydration runtime
        pageHeadJs.append_view(std::string_view("""
window.$__uni_hydration_queue = []
window.$__uni_batch_depth = 0
window.$__uni_pending_instances = []
window.$__uni_error = ((message, details = "", cause = null) => {
    const suffix = details ? ": " + details : "";
    const err = new Error(message + suffix);
    if(cause) err.cause = cause;
    throw err;
})
window.$__uni_dispatch = ((fnName, target, props, mode = "children") => {
    if(!target) {
        // A dispatch can miss its target when the server did not render the
        // component's boundary (e.g. an SSR bug, or a client-only subtree).
        // Never throw here: dispatches are emitted as sequential statements, so
        // an uncaught error would abort every later component's hydration.
        console.error("universal dispatch: mount target is missing for " + fnName + " (component was not server-rendered; skipping)");
        return;
    }
    const fn = window[fnName]
    if(fn) {
        // Memo check: if component is memoized and props haven't changed, skip mount
        if(fn.__uni_memo && target.__uni_prev_props) {
            const eq = fn.__uni_are_equal(target.__uni_prev_props, props || {});
            if(eq) return;
        }
        if(target) target.__uni_prev_props = props ? { ...props } : {};
        try {
            window.$__uni_mount(target, fn, props, mode);
        } catch(err) {
            console.error("universal mount failed for component", fnName, err);
        }
    } else {
        window.$__uni_hydration_queue.push([ fnName, target, props, mode ]);
    }
})
window.$__uni_flush_batch = (() => {
    const pending = window.$__uni_pending_instances;
    for(let i = 0; i < pending.length; i++) {
        const inst = pending[i];
        inst._pendingEffects = false;
        if(inst.layoutEffects && inst.layoutEffects.length) window.$__uni_run_effects(inst, inst.layoutEffects);
        if(inst.effects && inst.effects.length) window.$__uni_run_effects(inst, inst.effects);
    }
    pending.length = 0;
})
window.$__uni_batch = ((fn) => {
    window.$__uni_batch_depth++;
    try {
        fn();
    } finally {
        window.$__uni_batch_depth--;
        if(window.$__uni_batch_depth === 0) window.$__uni_flush_batch();
    }
})
"""))
        pageJs.append_view(std::string_view("""
// Normalizes one class value to a trimmed string (falsy values contribute
// nothing instead of the literal "false"/"null").
window.$__uni_class_part = ((v) => {
    const x = window.$__uni_value(v);
    return (x == null || x === false || x === "") ? "" : "" + x;
})
// Merges two class bindings. When either side is a state/computed signal the
// result is a combined computed so the binding stays live after the spread
// merge (previously the values were unwrapped with $__uni_value, freezing the
// class to its initial value). A falsy later part no longer wipes an earlier
// one.
window.$__uni_class_merge = ((a, b) => {
    if(window.$__uni_is_state(a) || window.$__uni_is_state(b)) {
        return window.$_ucs(() => {
            const as = window.$__uni_class_part(a);
            const bs = window.$__uni_class_part(b);
            if(as && bs) return as + " " + bs;
            return as || bs;
        });
    }
    const as = window.$__uni_class_part(a);
    const bs = window.$__uni_class_part(b);
    if(as && bs) return as + " " + bs;
    return as || bs;
})
window.$_um = ((...parts) => {
    const out = {};
    for(let i = 0; i < parts.length; i++) {
        const part = parts[i];
        if(!part) continue;
        for(const k in part) {
            const key = (k === "className" || k === "class") ? "class" : k;
            const raw = part[k];
            // Preserve state/signal values so reactive attribute bindings
            // (style, checked, value, ...) survive the merge and hydrate
            // subscribes to them. Unwrapping here froze the binding at mount.
            const v = window.$__uni_is_state(raw) ? raw : window.$__uni_value(raw);
            if(key === "class" && out[key] != null) {
                out[key] = window.$__uni_class_merge(out[key], v);
            } else {
                out[key] = v;
            }
        }
    }
    return out;
})
window.$_ur = {
    Fragment: {},
    createElement: (t, p, ...c) => ({ t, p: p || {}, c })
}
window.$__uni_register_resource = ((resource) => {
    const owner = window.$__uni_render_instance;
    if(!owner || !resource) return;
    if(!owner._resources) owner._resources = [];
    owner._resources.push(resource);
})
// Portal containers are appended to document.body, outside the DOM subtree of
// the component that owns them, so subtree disposal cannot find them. Register
// the container with the rendering instance so $__uni_dispose can remove it
// (otherwise a portaled Dialog/Select leaves its container in <body> forever).
window.$__uni_register_portal = ((container) => {
    const owner = window.$__uni_render_instance || window.$__uni_current_instance;
    if(!owner || !container) return;
    if(!owner._portals) owner._portals = [];
    owner._portals.push(container);
})
window.$_us = ((v) => {
    let val = v;
    const subs = [];
    const _inst = window.$__uni_current_instance;
    const signal = {
        get value() {
            if(window.$__uni_current_tracker) window.$__uni_current_tracker(this);
            return val;
        },
        set value(n) {
            // Bail out when the value is unchanged (Object.is-ish, like React).
            // Without this a redundant write (e.g. `n = n` in an event handler)
            // still notified every subscriber, recomputing dependent slots and
            // rebuilding their DOM -- dropping focus/input state for no reason.
            if(n === val) return;
            val = n;
            const snapshot = subs.slice();
            for(let i = 0; i < snapshot.length; i++) snapshot[i](val);
            // Layout effects run synchronously before paint (like React useLayoutEffect)
            if(_inst && _inst.layoutEffects && _inst.layoutEffects.length) {
                window.$__uni_run_effects(_inst, _inst.layoutEffects);
            }
            // During a batch, defer effect scheduling to batch end
            if(window.$__uni_batch_depth > 0) {
                if(_inst && !_inst._pendingEffects) {
                    _inst._pendingEffects = true;
                    window.$__uni_pending_instances.push(_inst);
                }
            } else if(_inst && !_inst._pendingEffects) {
                _inst._pendingEffects = true;
                Promise.resolve().then(() => {
                    _inst._pendingEffects = false;
                    if(_inst.effects && _inst.effects.length) window.$__uni_run_effects(_inst, _inst.effects);
                });
            }
        },
        subscribe(fn) {
            // Deduplicate: if the same function is already subscribed, don't add it again
            if(subs.indexOf(fn) === -1) subs.push(fn);
            return () => {
                const idx = subs.indexOf(fn);
                if(idx >= 0) subs.splice(idx, 1);
            };
        },
        // Ownership-driven disposal: drop every subscriber so computeds/effects
        // bound to this signal can be garbage collected when the owning
        // component instance is unmounted.
        $_dispose() {
            subs.length = 0;
        }
    };
    window.$__uni_register_resource(signal);
    return signal;
})
window.$_ucs = ((fn) => {
    let cached;
    const subs = [];
    let depUnsubs = [];
    let children = [];
    const emit = () => {
        const snapshot = subs.slice();
        for(let i = 0; i < snapshot.length; i++) snapshot[i](cached);
    };
    const dispose = () => {
        for(let i = 0; i < depUnsubs.length; i++) depUnsubs[i]();
        depUnsubs = [];
        for(let i = 0; i < children.length; i++) {
            if(children[i].$_uc_dispose) children[i].$_uc_dispose();
        }
        children = [];
    };
    const recompute = () => {
        for(let i = 0; i < children.length; i++) {
            if(children[i].$_uc_dispose) children[i].$_uc_dispose();
        }
        children = [];
        for(let i = 0; i < depUnsubs.length; i++) depUnsubs[i]();
        depUnsubs = [];
        const deps = [];
        window.$__uni_push_ctx({
            tracker: (dep) => {
                if(dep && deps.indexOf(dep) < 0) deps.push(dep);
            },
            childTracker: (child) => {
                if(child && children.indexOf(child) < 0) children.push(child);
            }
        });
        // try/finally: a computed body can throw (e.g. reading a property of
        // undefined). Without the finally the pushed context frame leaked, so
        // every later context restore was off by one and the render stack grew.
        let next;
        try {
            next = fn();
        } finally {
            window.$__uni_pop_ctx();
        }
        for(let i = 0; i < deps.length; i++) {
            const dep = deps[i];
            if(dep && typeof dep.subscribe === "function") {
                depUnsubs.push(dep.subscribe(() => recompute()));
            }
        }
        // Only notify when the value actually changed. Without this a computed
        // that recomputes to the same value (e.g. a derived string like a note's
        // type) still notified its subscribers, so a `{cond ? A : B}` reactive
        // slot re-evaluated, produced a fresh vnode and clear-and-rebuilt its
        // subtree -- dropping focus/input state on every unrelated update.
        const changed = next !== cached;
        cached = next;
        if(changed) emit();
    };
    recompute();
    const signal = {
        get value() {
            if(window.$__uni_current_tracker) window.$__uni_current_tracker(this);
            return cached;
        },
        subscribe(fn) {
            // Deduplicate: if the same function is already subscribed, don't add it again
            if(subs.indexOf(fn) === -1) subs.push(fn);
            return () => {
                const idx = subs.indexOf(fn);
                if(idx >= 0) subs.splice(idx, 1);
            };
        }
    };
    signal.$_uc_dispose = dispose;
    // A signal's own subscribers must also be dropped on disposal so that
    // effects/computeds holding it do not keep each other alive.
    signal.$_dispose = () => {
        subs.length = 0;
        dispose();
    };
    window.$__uni_register_resource(signal);
    if(window.$__uni_child_tracker) window.$__uni_child_tracker(signal);
    return signal;
})
// Render context stack. The current instance / boundary / resource owner and
// the dependency trackers used to live in flat globals that every entry point
// saved and restored by hand. A stack makes push/pop atomic and re-entrant: a
// nested mount, dispatch, or effect run cannot leave a stale context behind
// (each frame captures all five values and restores exactly what it replaced).
window.$__uni_current_instance = null;
window.$__uni_current_boundary = null;
window.$__uni_render_instance = null;
window.$__uni_current_tracker = null;
window.$__uni_child_tracker = null;
window.$__uni_render_stack = [];
// Pushes a frame capturing the current render context, then applies any
// overrides (fields left undefined keep their current value). Returns nothing;
// call $__uni_pop_ctx to restore.
window.$__uni_push_ctx = ((overrides) => {
    window.$__uni_render_stack.push({
        instance: window.$__uni_current_instance,
        boundary: window.$__uni_current_boundary,
        renderInstance: window.$__uni_render_instance,
        tracker: window.$__uni_current_tracker,
        childTracker: window.$__uni_child_tracker
    });
    if(overrides) {
        if(overrides.instance !== undefined) window.$__uni_current_instance = overrides.instance;
        if(overrides.boundary !== undefined) window.$__uni_current_boundary = overrides.boundary;
        if(overrides.renderInstance !== undefined) window.$__uni_render_instance = overrides.renderInstance;
        if(overrides.tracker !== undefined) window.$__uni_current_tracker = overrides.tracker;
        if(overrides.childTracker !== undefined) window.$__uni_child_tracker = overrides.childTracker;
    }
})
window.$__uni_pop_ctx = (() => {
    const frame = window.$__uni_render_stack.pop();
    if(!frame) return;
    window.$__uni_current_instance = frame.instance;
    window.$__uni_current_boundary = frame.boundary;
    window.$__uni_render_instance = frame.renderInstance;
    window.$__uni_current_tracker = frame.tracker;
    window.$__uni_child_tracker = frame.childTracker;
})
// The frame at the top of the stack (the values the enclosing context will be
// restored to). Used by $__uni_mount to drop the render owner/boundary after a
// component body runs while keeping current_instance set through hydration.
window.$__uni_peek_ctx = (() => {
    return window.$__uni_render_stack.length ? window.$__uni_render_stack[window.$__uni_render_stack.length - 1] : null;
})
window.$__uni_ctx = {}
// Builds one context-scope entry. Each provider instance gets its own entry
// (see $_r.createContext) so two providers that share a name do not collide;
// consumers resolve the nearest provider by walking the instance parent chain.
window.$__uni_ctx_entry = ((name, defaultValue) => {
    const sig = window.$_us(defaultValue);
    const entry = {
        name,
        get value() { return sig.value; },
        set value(n) {
            if(n && typeof n.subscribe === "function") {
                if(entry._unsub) entry._unsub();
                entry._unsub = n.subscribe((v) => { sig.value = v; });
                sig.value = n.value;
            } else {
                sig.value = n;
            }
        }
    };
    return entry;
})
// Nearest ancestor component instance for a host element. The server dispatches
// some children (e.g. ToggleGroup items) as independent top-level boundaries
// whose only nesting is in the DOM. Linking them to the nearest ancestor
// instance lets context resolve down the tree even without a mount-stack parent.
window.$__uni_find_parent_instance = ((host) => {
    let el = host && host.parentElement ? host.parentElement : null;
    while(el) {
        if(el.$__uni_instance) return el.$__uni_instance;
        el = el.parentElement;
    }
    return null;
})
// Shallow equality check for memoization and effect deps
window.$__uni_shallow_equal = ((a, b) => {
    if(a === b) return true;
    if(!a || !b || a.length !== b.length) return false;
    for(let i = 0; i < a.length; i++) {
        if(a[i] !== b[i]) return false;
    }
    return true;
})
// SVG/MathML namespace map
window.$__uni_ns = {
    "svg": "http://www.w3.org/2000/svg",
    "math": "http://www.w3.org/1998/Math/MathML"
}
window.$_r = {
    useEffect: (fn, deps) => {
        const inst = window.$__uni_current_instance;
        if(!inst) return;
        if(!inst.effects) inst.effects = [];
        const eff = { fn, deps, lastDeps: null, cleanup: null, depUnsubs: [] };
        inst.effects.push(eff);
        // Effects must re-run when a reactive dependency changes, not only when
        // the component's own state is assigned. Subscribe to every state /
        // computed in the deps array so controlled props (e.g. `open` passed
        // from a parent) still trigger the effect.
        if(deps) {
            for(let i = 0; i < deps.length; i++) {
                const d = deps[i];
                if(d && typeof d.subscribe === "function") {
                    eff.depUnsubs.push(d.subscribe(() => {
                        // During a batch, defer effect scheduling to batch end
                        if(window.$__uni_batch_depth > 0) {
                            if(!inst._pendingEffects) {
                                inst._pendingEffects = true;
                                window.$__uni_pending_instances.push(inst);
                            }
                        } else if(!inst._pendingEffects) {
                            inst._pendingEffects = true;
                            Promise.resolve().then(() => {
                                inst._pendingEffects = false;
                                // Layout effects run synchronously before regular effects
                                if(inst.layoutEffects && inst.layoutEffects.length) window.$__uni_run_effects(inst, inst.layoutEffects);
                                if(inst.effects && inst.effects.length) window.$__uni_run_effects(inst, inst.effects);
                            });
                        }
                    }));
                }
            }
        }
    },
    useLayoutEffect: (fn, deps) => {
        const inst = window.$__uni_current_instance;
        if(!inst) return;
        if(!inst.layoutEffects) inst.layoutEffects = [];
        const eff = { fn, deps, lastDeps: null, cleanup: null, depUnsubs: [] };
        inst.layoutEffects.push(eff);
        // Like useEffect, subscribe to every state/computed in the deps array
        // so a layout effect whose dependency is a PARENT-controlled prop signal
        // still re-runs when the parent's state changes (the owning instance's
        // own writes drain layout effects synchronously in the $_us setter, but
        // a dependency owned by another instance has no such drain).
        if(deps) {
            for(let i = 0; i < deps.length; i++) {
                const d = deps[i];
                if(d && typeof d.subscribe === "function") {
                    eff.depUnsubs.push(d.subscribe(() => {
                        if(window.$__uni_batch_depth > 0) {
                            if(!inst._pendingEffects) {
                                inst._pendingEffects = true;
                                window.$__uni_pending_instances.push(inst);
                            }
                        } else if(!inst._pendingEffects) {
                            inst._pendingEffects = true;
                            Promise.resolve().then(() => {
                                inst._pendingEffects = false;
                                if(inst.layoutEffects && inst.layoutEffects.length) window.$__uni_run_effects(inst, inst.layoutEffects);
                                if(inst.effects && inst.effects.length) window.$__uni_run_effects(inst, inst.effects);
                            });
                        }
                    }));
                }
            }
        }
    },
    useState: (initial) => {
        const s = window.$_us(initial);
        // React-style functional updater: `setN(v => v + 1)`. Without this the
        // setter stored the function itself as the new state value.
        return [ s, (next) => { s.value = (typeof next === "function") ? next(s.value) : next; } ];
    },
    useRef: (initial) => ({ current: initial }),
    useMemo: (fn, deps) => {
        const inst = window.$__uni_current_instance;
        if(!inst) return fn();
        if(!inst._memos) inst._memos = [];
        const idx = inst._memos.length;
        inst._memos.push({ fn, deps, value: undefined, initialized: false });
        const memo = inst._memos[idx];
        if(!memo.initialized || !window.$__uni_shallow_equal(deps, memo.deps)) {
            memo.deps = deps ? deps.slice() : null;
            memo.value = fn();
            memo.initialized = true;
        }
        return memo.value;
    },
    useCallback: (fn, deps) => {
        const inst = window.$__uni_current_instance;
        if(!inst || !deps) return fn;
        if(!inst._callbacks) inst._callbacks = [];
        const idx = inst._callbacks.length;
        inst._callbacks.push({ fn, deps, cached: fn, initialized: false });
        const cb = inst._callbacks[idx];
        if(!cb.initialized || !window.$__uni_shallow_equal(deps, cb.deps)) {
            cb.deps = deps.slice();
            cb.cached = fn;
            cb.initialized = true;
        }
        return cb.cached;
    },
    useUnmount: (fn) => {
        const inst = window.$__uni_current_instance;
        if(inst && inst._disposables) inst._disposables.push(fn);
    },
    useReducer: (reducer, initial) => {
        const state = window.$_us(initial);
        const dispatch = (action) => { state.value = reducer(state.value, action); };
        return [ state, dispatch ];
    },
    // Scoped context registry. A `createContext(name, default)` call in a
    // component body owns a provider scope for that component instance
    // (`inst._contexts[name]`), so two instances of the same provider - or two
    // providers that share a name - no longer collide. `useContext(name)` walks
    // the instance parent chain to the nearest provider. When no provider is in
    // scope it falls back to a process-wide default entry, preserving React's
    // `createContext(default)` semantics. Reading `.value` inside a $_ucs()
    // computed subscribes like any other signal; assigning a signal to `.value`
    // wires the scope to follow it (the provider publishes its state).
    createContext: (name, defaultValue) => {
        if(!window.$__uni_ctx[name]) {
            window.$__uni_ctx[name] = window.$__uni_ctx_entry(name, defaultValue);
        }
        const inst = window.$__uni_current_instance;
        if(inst) {
            if(!inst._contexts) inst._contexts = {};
            if(!inst._contexts[name]) inst._contexts[name] = window.$__uni_ctx_entry(name, defaultValue);
            return inst._contexts[name];
        }
        return window.$__uni_ctx[name];
    },
    useContext: (name) => {
        let inst = window.$__uni_current_instance;
        while(inst) {
            if(inst._contexts && inst._contexts[name]) return inst._contexts[name];
            inst = inst.parent;
        }
        if(!window.$__uni_ctx[name]) {
            window.$__uni_ctx[name] = window.$__uni_ctx_entry(name, undefined);
        }
        return window.$__uni_ctx[name];
    },
    createPortal: (children, opts) => ({ t: "__uni_portal", p: opts || {}, c: Array.isArray(children) ? children : [ children ] }),
    useErrorBoundary: (fallback) => {
        const inst = window.$__uni_current_instance;
        if(!inst) return;
        inst.errorFallback = typeof fallback === "function" ? fallback : null;
    }
}
// Memoization wrapper: wraps a component factory so it only re-renders
// when props change (shallow comparison). Usage:
//   window.$__uni_memo((props) => $_ur.createElement(...), areEqual?)
window.$__uni_memo = ((factory, areEqual) => {
    const check = areEqual || window.$__uni_shallow_equal;
    const wrapped = (props) => factory(props);
    wrapped.__uni_memo = true;
    wrapped.__uni_are_equal = (prev, next) => {
        if(!prev || !next) return false;
        const prevKeys = Object.keys(prev);
        const nextKeys = Object.keys(next);
        if(prevKeys.length !== nextKeys.length) return false;
        for(let i = 0; i < prevKeys.length; i++) {
            const k = prevKeys[i];
            if(prev[k] !== next[k]) return false;
        }
        return true;
    };
    return wrapped;
})
// Default fallback UI rendered in place of a universal component whose render
// threw. Components can supply their own via useErrorBoundary(fallback).
window.$__uni_default_fallback = ((props, err) => {
    const msg = (err && err.message) ? err.message : "component error";
    return window.$_ur.createElement("div", {
        "class": "chx-error-boundary",
        "role": "alert",
        "data-error": "true"
    }, "Something went wrong rendering this section.");
})
// Renders a component's registered error fallback, or the default UI. The
// fallback receives the same props the component would have received, plus the
// caught error.
window.$__uni_render_fallback = ((inst, props, err) => {
    if(inst && inst.errorFallback) {
        try {
            const out = inst.errorFallback(props, err);
            if(out) return out;
        } catch(e2) {
            console.error("[universal] error boundary fallback itself failed", e2);
        }
    }
    return window.$__uni_default_fallback(props, err);
})
// Positions a portaled menu/overlay relative to its trigger using the trigger's
// current viewport rect. Returns a cleanup that removes the scroll/resize
// listeners. Used by components that render into document.body via createPortal
// (Select menu, DropdownMenu, etc.) so they escape overflow/transform clipping.
window.$__uni_floating = ((trigger, menu, opts = {}) => {
    const gap = opts.gap || 6;
    // Estimate the portaled content's height once it is visible. Falls back to
    // a viewport-relative guess (half the viewport) when it is hidden, so menus
    // near the bottom edge still flip above instead of opening off-screen.
    const measureHeight = () => {
        const style = window.getComputedStyle(menu);
        if(style.display !== "none" && menu.offsetHeight > 0) {
            return menu.offsetHeight;
        }
        return Math.round(window.innerHeight * 0.5);
    };
    const update = () => {
        if(!trigger || !trigger.isConnected) return;
        const r = trigger.getBoundingClientRect();
        const menuHeight = measureHeight();
        const spaceBelow = window.innerHeight - r.bottom;
        const spaceAbove = r.top;
        const placeAbove = spaceBelow < menuHeight + gap && spaceAbove > spaceBelow;
        menu.style.position = "fixed";
        menu.style.left = r.left + "px";
        menu.style.minWidth = (opts.minWidth || r.width) + "px";
        menu.style.margin = "0";
        if(placeAbove) {
            menu.style.top = "auto";
            menu.style.bottom = (window.innerHeight - r.top + gap) + "px";
            menu.style.maxHeight = (spaceAbove - gap) + "px";
        } else {
            menu.style.top = (r.bottom + gap) + "px";
            menu.style.bottom = "auto";
            menu.style.maxHeight = (spaceBelow - gap) + "px";
        }
    };
    update();
    window.addEventListener("scroll", update, true);
    window.addEventListener("resize", update);
    return () => {
        window.removeEventListener("scroll", update, true);
        window.removeEventListener("resize", update);
    };
})
// Tags a portal container so the inert manager can exempt it (and the modal
// manager can find it). `opts.modal` marks modal overlays (Dialog/Sheet) whose
// visibility locks the background.
window.$__uni_tag_portal = ((container, opts = {}) => {
    container.setAttribute("data-uni-portal", "");
    if(opts && opts.modal) {
        container.setAttribute("data-uni-modal", "");
        // Recompute inert state whenever the modal's visibility toggles. The
        // open/close state is applied as a reactive `style` on the modal's
        // root element, so watching style mutations on the subtree is enough.
        // Doing this via MutationObserver (not a component effect) keeps it
        // robust: hydration can remount a component and dispose its effects,
        // but the DOM binding and this observer survive.
        if(typeof MutationObserver !== "undefined") {
            const mo = new MutationObserver(() => window.$__uni_inert_scan());
            mo.observe(container, { attributes: true, subtree: true, attributeFilter: [ "style" ] });
            container.$__uni_inert_observer = mo;
        }
        window.$__uni_inert_scan();
    }
})
// Modal overlay support (WAI-ARIA dialog pattern): while ANY modal portal is
// visible, everything in <body> except the portal containers becomes inert
// (not focusable, not clickable, hidden from the a11y tree). Non-modal portals
// (Select menu, DropdownMenu) stay interactive even inside an open modal.
// Components opt in via createPortal(children, { modal: true }) and call this
// from an effect keyed on their open state (open -> lock, close -> unlock).
window.$__uni_inert_scan = (() => {
    const scan = () => {
        let active = false;
        const modals = document.querySelectorAll("[data-uni-modal]");
        for(let i = 0; i < modals.length; i++) {
            const first = modals[i].firstElementChild;
            if(!first) continue;
            const st = window.getComputedStyle(first);
            if(st.display !== "none" && st.visibility !== "hidden") {
                active = true;
                break;
            }
        }
        const kids = document.body.children;
        for(let i = 0; i < kids.length; i++) {
            const kid = kids[i];
            const isPortal = kid.hasAttribute("data-uni-portal");
            kid.inert = active && !isPortal;
        }
    };
    return scan;
})()
window.$__uni_run_effects = ((inst, effects) => {
    if(!effects) return;
    // Run effects inside the owning instance's render context so error
    // boundaries resolve to it and any signals/hooks created inside an effect
    // are attributed to (and disposed with) the same instance.
    window.$__uni_push_ctx({ instance : inst, boundary : inst, renderInstance : inst });
    for(let i = 0; i < effects.length; i++) {
        const eff = effects[i];
        // Resolve signal deps to their current values for comparison. Comparing
        // the raw deps array (signal objects) against the previous run's values
        // would always report "changed" and over-run every effect.
        const resolved = eff.deps ? eff.deps.map(window.$__uni_value) : null;
        let changed = !eff.lastDeps;
        if(!changed && resolved) {
            changed = !window.$__uni_shallow_equal(resolved, eff.lastDeps);
        }
        if(changed) {
            if(eff.cleanup) {
                try { eff.cleanup(); } catch(err) { console.error("[universal] effect cleanup failed:", err); }
            }
            try {
                eff.cleanup = eff.fn();
            } catch(err) {
                console.error("[universal] effect failed:", err);
                eff.cleanup = null;
            }
            eff.lastDeps = resolved || [];
            eff.ran = true;
        }
    }
    window.$__uni_pop_ctx();
})
window.$__uni_is_state = ((v) => !!(v && typeof v.subscribe === "function" && "value" in v))
// Development diagnostics toggle. Enabled by default for actionable developer
// feedback; a production build can set `window.$__uni_prod = true` (or set
// `window.$__uni_dev = false`) before the runtime loads to get single-shot,
// no-metadata warnings instead.
window.$__uni_dev = window.$__uni_prod ? false : (window.$__uni_dev !== false);
window.$__uni_dev_assert = ((cond, msg) => {
    if(window.$__uni_dev && !cond) console.warn("[universal] assertion failed: " + msg);
    return !!cond;
})
window.$__uni_warn_hydration = ((msg, expected, got) => {
    if(window.$__uni_dev) {
        window.$__uni_hydration_warn_count = (window.$__uni_hydration_warn_count || 0) + 1;
        if(window.$__uni_hydration_warn_count > 25) {
            if(window.$__uni_hydration_warn_count === 26) {
                console.warn("[universal] further hydration warnings suppressed (dev build)");
            }
            return;
        }
        console.warn("[universal] hydration mismatch: " + msg, expected, got);
        return;
    }
    // Hydration mismatches are reported once in production but never crash the
    // page: the runtime self-corrects below. Guarded so a busy page does not
    // spam thousands of duplicate messages or leak expected/got values.
    if(window.$__uni_hydration_warned) return;
    window.$__uni_hydration_warned = true;
    console.warn("[universal] hydration mismatch: " + msg);
})
// Universal component vnode that references the client component function directly
// instead of a name + SSR HTML snapshot. This is the Phase 2 hydration boundary:
// the server-rendered DOM is located and hydrated in place, so no SSR markup is
// transported through the JavaScript bundle.
window.$_uc_c = ((comp, props) => ({ t: "__uni_uc", p: { comp, props } }))
window.$__uni_value = ((v) => window.$__uni_is_state(v) ? v.value : v)
window.$__uni_html = ((html, count) => ({ __uni_html: html || "", __uni_count: count || 0 }))
// Resolves a hydration boundary id to either an element (the legacy
// `<span data-chx-i>` wrapper) or a comment marker (`<!--u{id}-->`). Comment
// boundaries are used inside table structure, where a wrapper element is
// invalid HTML and would be foster-parented by the parser. The marker index is
// built once; SSR markers are all present before the page JS runs.
window.$__uni_boundary = ((id) => {
    const el = document.getElementById(id);
    if(el) return el;
    let map = window.$__uni_boundary_map;
    if(!map) {
        map = Object.create(null);
        if(typeof document.createTreeWalker === "function") {
            const w = document.createTreeWalker(document.documentElement, 128 /* SHOW_COMMENT */);
            while(w.nextNode()) {
                const v = w.currentNode.nodeValue;
                if(v && v.charCodeAt(0) === 117 /* 'u' */) map[v] = w.currentNode;
            }
        }
        window.$__uni_boundary_map = map;
    }
    return map[id] || null;
})
window.$__uni_is_active_editable = ((el) => !!(el && el.isContentEditable && document.activeElement === el))
window.$__uni_assign_ref = ((el, refValue) => {
    if(!el) return;
    if(refValue == null || refValue === false) return;
    if(typeof refValue === "function") {
        // Remember the callback on the element so it can be invoked with null
        // when the element is removed (React ref contract).
        el.$__uni_ref_fn = refValue;
        refValue(el);
        return;
    }
    if(typeof refValue === "object" && "current" in refValue) {
        el.$__uni_ref_obj = refValue;
        refValue.current = el;
    }
})
window.$__uni_inner_html_value = ((v) => {
    if(v && typeof v === "object" && v.__html !== undefined) return v.__html == null ? "" : "" + v.__html;
    if(v == null || v === false) return "";
    return "" + v;
})
window.$__uni_apply_inner_html = ((el, nextHtml) => {
    if(!el) return;
    if(el.innerHTML !== nextHtml) el.innerHTML = nextHtml;
    el.$__uni_last_inner_html = nextHtml;
})
window.$__uni_flush_pending_inner_html = ((el) => {
    if(!el) return;
    const pending = el.$__uni_pending_inner_html;
    if(pending === undefined) return;
    delete el.$__uni_pending_inner_html;
    window.$__uni_apply_inner_html(el, pending);
})
window.$__uni_set_prop = ((el, key, value) => {
    if(!el) {
        window.$__uni_error("cannot set property on missing element", "" + key);
    }
    const v = window.$__uni_value(value);
    if(key === "children" || key === "key" || key == null) return;
    if(key === "ref") {
        el.$__uni_ref = v;
        return;
    }
    if(key === "dangerouslySetInnerHTML") {
        const nextHtml = window.$__uni_inner_html_value(v);
        if(el.isContentEditable && !el.$__uni_inner_html_guard) {
            el.$__uni_inner_html_guard = true;
            el.addEventListener("blur", () => window.$__uni_flush_pending_inner_html(el));
        }
        if(window.$__uni_is_active_editable(el)) {
            el.$__uni_pending_inner_html = nextHtml;
            return;
        }
        if(el.$__uni_pending_inner_html !== undefined) delete el.$__uni_pending_inner_html;
        window.$__uni_apply_inner_html(el, nextHtml);
        return;
    }
    if(key === "className" || key === "class") {
        if(v == null || v === false) el.removeAttribute("class");
        else el.setAttribute("class", "" + v);
        return;
    }
    if(key === "htmlFor" || key === "for") {
        if(v == null || v === false) el.removeAttribute("for");
        else el.setAttribute("for", "" + v);
        return;
    }
    if(key === "style") {
        if(v == null || v === false) {
            el.removeAttribute("style");
        } else if(typeof v === "string") {
            el.style.cssText = v;
        } else if(typeof v === "object") {
            el.removeAttribute("style");
            for(const sk in v) {
                const sv = window.$__uni_value(v[sk]);
                if(sv == null || sv === false) continue;
                // CSS custom properties (`--foo`) are not camelCase JS style
                // properties: `el.style["--foo"] = v` is a silent no-op. They
                // must be set through setProperty (and keep their `--` name).
                if(sk.charCodeAt(0) === 45 /* '-' */) el.style.setProperty(sk, "" + sv);
                else el.style[sk] = sv;
            }
        } else {
            window.$__uni_error("invalid style value", typeof v + " on <" + el.tagName.toLowerCase() + ">");
        }
        return;
    }
    if(key.length > 2 && key[0] === "o" && key[1] === "n") {
        let eventName = key.substring(2).toLowerCase();
        // React's `onDoubleClick` maps to the DOM `dblclick` event; lowercasing
        // the prop name produces "doubleclick", which never fires.
        if(eventName === "doubleclick") eventName = "dblclick";
        // React's `onChange` on an <input>/<textarea> is the DOM `input` event
        // (fires on every keystroke), not the native `change` event (which only
        // fires on blur/commit). <select> and everything else keep `change`.
        // Input/TextArea forward `onChange`, so the standard controlled-input
        // pattern was otherwise dead until the field lost focus.
        if(eventName === "change" && el.tagName) {
            const tag = el.tagName.toLowerCase();
            if(tag === "input" || tag === "textarea") eventName = "input";
        }
        if(!el.$__uni_events) el.$__uni_events = {};
        const prev = el.$__uni_events[eventName];
        if(prev) el.removeEventListener(eventName, prev);
        if(v == null || v === false) {
            delete el.$__uni_events[eventName];
            return;
        }
        if(typeof v !== "function") {
            window.$__uni_error("event handler must be a function", key + " on <" + el.tagName.toLowerCase() + ">");
        }
        // Wrap handlers so a throwing handler is logged and contained instead
        // of taking down the whole page (error-boundary contract).
        // Also wrap in automatic batching so multiple state updates in one
        // event handler are coalesced into a single effect flush.
        const wrapped = (e) => {
            try {
                window.$__uni_batch(() => v(e));
            } catch(err) {
                console.error("[universal] event handler failed:", err);
            }
        };
        el.$__uni_events[eventName] = wrapped;
        el.addEventListener(eventName, wrapped);
        return;
    }
    const propType = typeof el[key];
    if(v == null || v === false) {
        if(key in el && typeof el[key] !== "function") {
            if(propType === "boolean") el[key] = false;
            else el[key] = "";
        }
        el.removeAttribute(key);
        return;
    }
    // SVG elements expose most geometry/presentation attributes as read-only
    // accessor properties (`width`, `height`, `viewBox`, `x`, `cx`, `r`, ...),
    // so `el[key] = v` silently does nothing and the attribute is dropped --
    // icons then render at the SVG default 300x150. Always set SVG attributes
    // with `setAttribute` (case-sensitive, which `viewBox` requires).
    if(el.namespaceURI === "http://www.w3.org/2000/svg") {
        el.setAttribute(key, "" + v);
        return;
    }
    if(key in el && propType === "boolean") {
        el[key] = !!v;
        if(v) el.setAttribute(key, "");
        else el.removeAttribute(key);
        return;
    }
    if(key in el && key !== "list" && key !== "type") {
        try {
            el[key] = v;
            return;
        } catch(err) {
            // fall through to setAttribute below
        }
    }
    el.setAttribute(key, "" + v);
})
window.$__uni_apply_prop = ((el, key, value) => {
    window.$__uni_set_prop(el, key, value);
    if(!el) return;
    const had = el.$__uni_prop_subs ? el.$__uni_prop_subs[key] : null;
    if(window.$__uni_is_state(value)) {
        if(!el.$__uni_prop_subs) el.$__uni_prop_subs = {};
        // Replace any previous subscription for this key, so re-applying a
        // reactive prop does not stack subscribers (and a stale element signal
        // binding is dropped when the prop becomes static).
        if(had) { try { had(); } catch(err) {} }
        el.$__uni_prop_subs[key] = value.subscribe((next) => window.$__uni_set_prop(el, key, next));
    } else if(had) {
        try { had(); } catch(err) {}
        delete el.$__uni_prop_subs[key];
    }
})
// Remove and dispose every node in the comment-delimited range (start, end).
window.$__uni_clear_range = ((start, end) => {
    while(start.nextSibling && start.nextSibling !== end) {
        const n = start.nextSibling;
        window.$__uni_dispose_subtree(n);
        n.remove();
    }
})
// True when the element tag is a real DOM element (not a runtime pseudo-node
// such as "__uni_uc" / "__uni_portal", nor a Fragment/function component).
window.$__uni_is_element_tag = ((t) => {
    return typeof t === "string" && t.charCodeAt(0) !== 95;
})
// Key of a vnode. Element and fragment vnodes carry it in `p.key`; component
// vnodes emitted by JSX (`$_uc_c` -> `{t:"__uni_uc", p:{comp, props}}`) carry
// it one level deeper, in `p.props.key`.
window.$__uni_vnode_key = ((v) => {
    if(!v || typeof v !== "object" || !v.p) return null;
    if(v.p.key != null) return v.p.key;
    if(v.p.props && v.p.props.key != null) return v.p.props.key;
    return null;
})
// Persistent key -> {first,last} (inclusive) DOM range map for a keyed
// container. Stored on the comment `start` node (state arrays) or the parent
// element (children) so multi-node keyed items -- fragments and component
// vnodes whose root is a fragment -- move and are removed as a unit instead of
// being rebuilt.
window.$__uni_key_ranges = ((owner) => {
    if(!owner.__uni_key_ranges) owner.__uni_key_ranges = new Map();
    return owner.__uni_key_ranges;
})
window.$__uni_range_move = ((range, before) => {
    if(!range || !range.first || !before || before === range.first) return;
    const parent = before.parentNode;
    if(!parent) return;
    let n = range.first;
    while(n) {
        const next = n.nextSibling;
        parent.insertBefore(n, before);
        if(n === range.last) break;
        n = next;
    }
})
window.$__uni_range_remove = ((range) => {
    if(!range || !range.first) return;
    let n = range.first;
    while(n) {
        const next = n.nextSibling;
        window.$__uni_dispose_subtree(n);
        if(n.parentNode) n.parentNode.removeChild(n);
        if(n === range.last) break;
        n = next;
    }
})
// Two component vnodes are the "same item" when they render the same component
// factory with equal scalar props. `children`/`key`/`ref` are recreated on every
// render, so they are excluded. Used to keep a keyed component item's DOM and
// instance intact across reorders.
window.$__uni_uc_props_equal = ((ov, nv) => {
    if(!ov || !nv || ov.t !== "__uni_uc" || nv.t !== "__uni_uc") return false;
    if(ov.p.comp !== nv.p.comp) return false;
    const a = ov.p.props || {};
    const b = nv.p.props || {};
    for(const k in a) {
        if(k === "children" || k === "key" || k === "ref") continue;
        if(a[k] !== b[k]) return false;
    }
    for(const k in b) {
        if(k === "children" || k === "key" || k === "ref") continue;
        if(a[k] !== b[k]) return false;
    }
    return true;
})
// Update an existing keyed range's content to `nv`. Returns the (possibly new)
// range. Patchable single-element/text items are patched in place; same-item
// component and fragment vnodes keep their range so the item can simply be
// moved; anything else is rebuilt within the range.
window.$__uni_update_keyed_range = ((range, ov, nv) => {
    if(!range || !range.first) return range;
    const single = range.first === range.last;
    if(single) {
        const dom = range.first;
        if(ov && nv && ov.t === "__uni_uc" && nv.t === "__uni_uc") {
            // Same component with equal props: keep the DOM and instance so a
            // reorder moves rather than re-mounts it.
            if(window.$__uni_uc_props_equal(ov, nv)) return range;
        } else if(dom.nodeType === 1 && nv && window.$__uni_is_element_tag(nv.t)
                  && dom.tagName.toLowerCase() === ("" + nv.t).toLowerCase()) {
            window.$__uni_patch_node(dom, ov, nv);
            return range;
        } else if(dom.nodeType === 3 && (typeof nv === "string" || typeof nv === "number")) {
            window.$__uni_patch_node(dom, ov, nv);
            return range;
        }
    } else if(ov && nv && ov.t === "__uni_uc" && nv.t === "__uni_uc"
              && window.$__uni_uc_props_equal(ov, nv)) {
        return range;
    } else if(ov && nv && ov.t === window.$_ur.Fragment && nv.t === window.$_ur.Fragment) {
        // Keyed fragment: keep the range so a reorder moves it intact.
        return range;
    }
    // Rebuild the range in place: render the new content before the old range,
    // then drop the old range.
    const parent = range.first.parentNode;
    const rendered = window.$_urn(nv);
    let first = null, last = null;
    if(rendered.nodeType === 11) { first = rendered.firstChild; last = rendered.lastChild; }
    else { first = last = rendered; }
    if(parent) {
        parent.insertBefore(rendered, range.first);
    } else if(first && !first.parentNode) {
        // No parent (detached list): fall back to returning the old range.
        return range;
    }
    window.$__uni_range_remove(range);
    return first ? { first, last } : range;
})
// Patch one existing DOM node in place to match `newV` instead of destroying and
// re-creating it. This preserves node identity: input focus/value, scroll
// position, and child component instances survive list updates and re-orders.
// Falls back to a full rebuild (via $_urn + replaceChild) when the shape is not
// patchable (different tag, fragment, component vnode, state, primitive change).
// Returns the node now occupying the old position.
window.$__uni_patch_node = ((dom, oldV, newV) => {
    if(dom && dom.nodeType === 3) {
        if(newV == null || newV === false || newV === true) {
            if(dom.nodeValue !== "") dom.nodeValue = "";
            return dom;
        }
        if(typeof newV === "string" || typeof newV === "number") {
            const s = "" + newV;
            if(dom.nodeValue !== s) dom.nodeValue = s;
            return dom;
        }
    } else if(dom && dom.nodeType === 1 && newV && window.$__uni_is_element_tag(newV.t)
            && dom.tagName.toLowerCase() === newV.t.toLowerCase()) {
        const props = newV.p || {};
        for(const k in props) window.$__uni_apply_prop(dom, k, props[k]);
        window.$__uni_patch_children(dom, oldV ? oldV.c : null, newV.c || []);
        if(dom.$__uni_ref !== undefined) {
            window.$__uni_assign_ref(dom, dom.$__uni_ref);
            delete dom.$__uni_ref;
        }
        return dom;
    }
    // Unpatchable: rebuild this single node and splice it in.
    const parent = dom ? dom.parentNode : null;
    const before = dom ? dom.previousSibling : null;
    const rendered = window.$_urn(newV);
    if(parent && dom) {
        parent.replaceChild(rendered, dom);
        const first = before ? before.nextSibling : parent.firstChild;
        return first;
    }
    return rendered;
})
// Patch a parent element's children against the previously rendered child vnode
// array. Keyed children are matched by `__uni_vnode_key` (O(n) map build);
// unkeyed children are patched positionally when the DOM shape is 1:1.
// Falls back to clearing and re-rendering when the shape cannot be reconciled.
window.$__uni_patch_children = ((parent, oldChildren, newChildren) => {
    oldChildren = oldChildren || [];
    newChildren = newChildren || [];
    let anyKey = false;
    for(let i = 0; i < newChildren.length; i++) {
        if(window.$__uni_vnode_key(newChildren[i]) != null) { anyKey = true; break; }
    }
    if(anyKey) {
        // Keyed reconciliation over persistent DOM ranges (see
        // $__uni_reconcile_list). Ranges are stored on the parent element so
        // multi-node items (fragments, component vnodes) are moved, not
        // rebuilt, and component instances survive reorders.
        const ranges = window.$__uni_key_ranges(parent);
        const oldByKey = new Map();
        for(let i = 0; i < oldChildren.length; i++) {
            const k = window.$__uni_vnode_key(oldChildren[i]);
            if(k != null) oldByKey.set(k, oldChildren[i]);
        }
        let anchor = null;
        const kept = new Set();
        for(let i = 0; i < newChildren.length; i++) {
            const nc = newChildren[i];
            const nk = window.$__uni_vnode_key(nc);
            const target = anchor ? anchor.nextSibling : parent.firstChild;
            let range = nk != null ? ranges.get(nk) : null;
            if(nk != null && range && oldByKey.has(nk)) {
                const oc = oldByKey.get(nk);
                oldByKey.delete(nk);
                range = window.$__uni_update_keyed_range(range, oc, nc);
                ranges.set(nk, range);
                if(range.first !== target) window.$__uni_range_move(range, target);
                if(range.first && range.first.nodeType === 1) range.first.__uni_vnode_key = nk;
                kept.add(nk);
                anchor = range.last;
            } else {
                const rendered = window.$_urn(nc);
                let first = rendered, last = rendered;
                if(rendered.nodeType === 11) { first = rendered.firstChild; last = rendered.lastChild; }
                parent.insertBefore(rendered, target);
                if(nk != null && first) ranges.set(nk, { first, last });
                if(nk != null && first && first.nodeType === 1) first.__uni_vnode_key = nk;
                if(nk != null) kept.add(nk);
                if(last) anchor = last;
            }
        }
        const stale = [];
        ranges.forEach((r, k) => { if(!kept.has(k)) stale.push(k); });
        for(const k of stale) { window.$__uni_range_remove(ranges.get(k)); ranges.delete(k); }
        return;
    }
    // Unkeyed positional reconciliation. When the previous render mapped each
    // vnode to exactly one DOM node (verified by comparing the live child count
    // to the previous vnode count), patch the common prefix in place, append the
    // new tail, and remove the stale tail. This preserves node identity, focus,
    // and input values across insertions and removals instead of rebuilding the
    // whole subtree. Shapes where a vnode rendered to multiple DOM nodes (a
    // fragment or state marker range) fail the 1:1 check and fall back.
    if(oldChildren.length === parent.childNodes.length && parent.childNodes.length > 0 && newChildren.length > 0) {
        const doms = [];
        let ch = parent.firstChild;
        while(ch) { doms.push(ch); ch = ch.nextSibling; }
        const common = Math.min(doms.length, newChildren.length);
        for(let i = 0; i < common; i++) window.$__uni_patch_node(doms[i], oldChildren[i], newChildren[i]);
        for(let i = common; i < newChildren.length; i++) parent.appendChild(window.$_urn(newChildren[i]));
        for(let i = common; i < doms.length; i++) {
            window.$__uni_dispose_subtree(doms[i]);
            doms[i].remove();
        }
        return;
    }
    while(parent.firstChild) {
        window.$__uni_dispose_subtree(parent.firstChild);
        parent.removeChild(parent.firstChild);
    }
    for(let i = 0; i < newChildren.length; i++) parent.appendChild(window.$_urn(newChildren[i]));
})
// Reconcile a vnode array into the comment-delimited DOM range
// (start, end). Keyed arrays match old nodes by `__uni_vnode_key` and move them
// (preserving identity, focus, and input values); unkeyed arrays with a stable
// shape are patched in place, otherwise replaced.
// Returns the vnode array to track for the next reconcile. Shared by fresh
// renders ($_urn) and hydration adoption so both use identical semantics.
window.$__uni_reconcile_list = ((start, end, next, oldVnodes) => {
    if(!Array.isArray(next)) {
        if(start.__uni_key_ranges) start.__uni_key_ranges.clear();
        window.$__uni_clear_range(start, end);
        start.after(window.$_urn(next));
        return null;
    }
    let anyKey = false;
    for(let i = 0; i < next.length; i++) {
        if(window.$__uni_vnode_key(next[i]) != null) { anyKey = true; break; }
    }
    if(!anyKey) {
        // Unkeyed: patch positionally when the previous render mapped each vnode
        // to exactly one DOM node (oldVnodes length === live node count), then
        // append the new tail / remove the stale tail. Preserves focus/inputs on
        // insert and remove; falls back to a rebuild for multi-node shapes.
        const doms = [];
        let n = start.nextSibling;
        while(n && n !== end) { doms.push(n); n = n.nextSibling; }
        if(Array.isArray(oldVnodes) && oldVnodes.length === doms.length) {
            const common = Math.min(doms.length, next.length);
            for(let i = 0; i < common; i++) window.$__uni_patch_node(doms[i], oldVnodes[i], next[i]);
            for(let i = common; i < next.length; i++) end.parentNode.insertBefore(window.$_urn(next[i]), end);
            for(let i = common; i < doms.length; i++) {
                window.$__uni_dispose_subtree(doms[i]);
                doms[i].remove();
            }
            return next;
        }
        if(start.__uni_key_ranges) start.__uni_key_ranges.clear();
        window.$__uni_clear_range(start, end);
        start.after(window.$_urn(next));
        return next;
    }
    // Keyed: match by key against persistent DOM ranges (key -> {first,last}).
    // Ranges let multi-node items -- fragments and component vnodes whose root
    // is a fragment or several elements -- move and be removed as a unit, so
    // component instances, focus, and input values survive reorders instead of
    // being rebuilt. Ranges are seeded during hydration (see $__uni_hydrate_node)
    // and created here for fresh renders.
    const ranges = window.$__uni_key_ranges(start);
    const oldByKey = new Map();
    if(Array.isArray(oldVnodes)) {
        for(let i = 0; i < oldVnodes.length; i++) {
            const k = window.$__uni_vnode_key(oldVnodes[i]);
            if(k != null) oldByKey.set(k, oldVnodes[i]);
        }
    }
    let anchor = start;
    const newVnodes = [];
    const kept = new Set();
    for(let i = 0; i < next.length; i++) {
        const nv = next[i];
        const nk = window.$__uni_vnode_key(nv);
        const before = anchor.nextSibling ? anchor.nextSibling : end;
        let range = nk != null ? ranges.get(nk) : null;
        if(nk != null && range && oldByKey.has(nk)) {
            const ov = oldByKey.get(nk);
            oldByKey.delete(nk);
            range = window.$__uni_update_keyed_range(range, ov, nv);
            ranges.set(nk, range);
            if(range.first !== before) window.$__uni_range_move(range, before);
            if(range.first && range.first.nodeType === 1) range.first.__uni_vnode_key = nk;
            kept.add(nk);
            anchor = range.last;
        } else {
            const rendered = window.$_urn(nv);
            let first = rendered, last = rendered;
            if(rendered.nodeType === 11) { first = rendered.firstChild; last = rendered.lastChild; }
            start.parentNode.insertBefore(rendered, before);
            if(nk != null && first) ranges.set(nk, { first, last });
            if(nk != null && first && first.nodeType === 1) first.__uni_vnode_key = nk;
            if(nk != null) kept.add(nk);
            anchor = last || anchor;
        }
        newVnodes.push(nv);
    }
    const stale = [];
    ranges.forEach((r, k) => { if(!kept.has(k)) stale.push(k); });
    for(const k of stale) { window.$__uni_range_remove(ranges.get(k)); ranges.delete(k); }
    return newVnodes;
})
window.$_urn = ((v, parentNs) => {
    if(v == null || v === false || v === true) return document.createTextNode("");
    if(window.$__uni_is_state(v)) {
        const start = document.createComment("s");
        const end = document.createComment("e");
        const f = document.createDocumentFragment();
        f.appendChild(start);
        f.appendChild(end);
        let oldVnodes = null;
        v.subscribe((next) => {
            if(start.__uni_slot_inst) { window.$__uni_dispose(start.__uni_slot_inst); start.__uni_slot_inst = null; }
            window.$__uni_last_mount_instance = null;
            oldVnodes = window.$__uni_reconcile_list(start, end, next, oldVnodes);
            if(window.$__uni_last_mount_instance) start.__uni_slot_inst = window.$__uni_last_mount_instance;
        });
        window.$__uni_last_mount_instance = null;
        oldVnodes = window.$__uni_reconcile_list(start, end, v.value, oldVnodes);
        if(window.$__uni_last_mount_instance) start.__uni_slot_inst = window.$__uni_last_mount_instance;
        return f;
    }
    if(v.nodeType) return v;
    if(Array.isArray(v)) {
        const f = document.createDocumentFragment();
        for(let i = 0; i < v.length; i++) f.appendChild(window.$_urn(v[i], parentNs));
        return f;
    }
    if(typeof v === "string" || typeof v === "number") return document.createTextNode("" + v);
    if(v && v.__uni_html !== undefined) {
        const tpl = document.createElement("template");
        tpl.innerHTML = v.__uni_html;
        return tpl.content.cloneNode(true);
    }
    if(v && v.t !== undefined) {
        if(v.t === "__uni_uc") {
            const { name, props, comp } = v.p;
            const container = document.createElement("div");
            if(comp) {
                window.$__uni_mount(container, comp, props);
                const f = document.createDocumentFragment();
                while(container.firstChild) f.appendChild(container.firstChild);
                return f;
            }
            // Legacy name-dispatch vnode (no client function reference).
            window.$__uni_dispatch(name, container, props);
            return container;
        }
        if(v.t === window.$_ur.Fragment) {
            const f = document.createDocumentFragment();
            for(let i = 0; i < (v.c || []).length; i++) f.appendChild(window.$_urn(v.c[i], parentNs));
            return f;
        }
        if(v.t === "__uni_portal") {
            const container = document.createElement("div");
            document.body.appendChild(container);
            window.$__uni_tag_portal(container, v.p);
            window.$__uni_register_portal(container);
            const children = v.c || [];
            for(let i = 0; i < children.length; i++) container.appendChild(window.$_urn(children[i]));
            // Rescan after the children exist: tag_portal's scan ran while the
            // container was still empty, so a modal would never lock the page.
            window.$__uni_inert_scan();
            return container;
        }
        if(typeof v.t === "function") {
            const nextProps = v.p ? { ...v.p } : {};
            if(v.c && v.c.length) nextProps.children = v.c.length === 1 ? v.c[0] : v.c;
            return window.$_urn(v.t(nextProps), parentNs);
        }
        // Inherit the SVG/MathML namespace from the parent when the tag is not
        // itself a namespace root, so `<svg><path/></svg>` creates the `path`
        // in the SVG namespace (otherwise it is an inert HTML element and the
        // icon renders blank).
        const ns = window.$__uni_ns[v.t] || parentNs || null;
        const e = ns ? document.createElementNS(ns, v.t) : document.createElement(v.t);
        const props = v.p || {};
        for(const k in props) window.$__uni_apply_prop(e, k, props[k]);
        const children = v.c || [];
        for(let i = 0; i < children.length; i++) e.appendChild(window.$_urn(children[i], ns));
        if(e.$__uni_ref !== undefined) {
            window.$__uni_assign_ref(e, e.$__uni_ref);
            delete e.$__uni_ref;
        }
        return e;
    }
    return document.createTextNode("" + v);
})
window.$__uni_hydrate_children = ((parent, values) => {
    if(!parent) return;
    const list = Array.isArray(values) ? values : [ values ];
    let dom = parent.firstChild;
    for(let i = 0; i < list.length; i++) {
        dom = window.$__uni_hydrate_node(parent, dom, list[i]);
    }
})
window.$__uni_hydrate_node = ((parent, dom, v) => {
    if(v == null || v === false || v === true) return dom;
    if(Array.isArray(v)) {
        if(dom && dom.nodeType === 8 && dom.nodeValue === "s") {
            const start = dom;
            let cur = start.nextSibling;
            while(cur && (cur.nodeType !== 8 || cur.nodeValue !== "e")) cur = cur.nextSibling;
            const end = cur;
            cur = start.nextSibling;
            for(let i = 0; i < v.length; i++) {
                cur = window.$__uni_hydrate_node(parent, cur, v[i]);
            }
            return end ? end.nextSibling : cur;
        }
        let cur = dom;
        if (!cur && parent) {
            parent.appendChild(window.$_urn(v));
            return null;
        }
        for(let i = 0; i < v.length; i++) {
            cur = window.$__uni_hydrate_node(parent, cur, v[i]);
        }
        return cur;
    }
    if(window.$__uni_is_state(v)) {
        if(dom && dom.nodeType === 8 && dom.nodeValue === "s") {
            const start = dom;
            let cur = start.nextSibling;
            while(cur && (cur.nodeType !== 8 || cur.nodeValue !== "e")) cur = cur.nextSibling;
            const end = cur;
            v.subscribe((next) => {
                window.$__uni_clear_range(start, end);
                start.after(window.$_urn(next));
            });
            return end ? end.nextSibling : null;
        }
        const start = document.createComment("s");
        const end = document.createComment("e");
        const stateVal = v.value;
        // SSR'd vnode (universal child or plain element) rendered inline without
        // markers: adopt the existing nodes in place instead of appending a
        // duplicate copy. Wrap them in markers so later updates can swap them out.
        // A component/fragment value can render SEVERAL SSR nodes for one vnode,
        // so the end marker must close after the whole range -- hydrating first
        // and placing `end` at dom.nextSibling cut the range short and shifted
        // every following sibling onto the wrong node.
        const stateIsVnode = stateVal && typeof stateVal === "object" && stateVal.t !== undefined;
        const stateVnodeIsMulti = stateVal && (stateVal.t === "__uni_uc" || stateVal.t === window.$_ur.Fragment || typeof stateVal.t === "function");
        if(dom && stateIsVnode && (stateVnodeIsMulti || dom.nodeType === 1)) {
            if(parent) parent.insertBefore(start, dom);
            let after;
            if(stateVal.t === "__uni_uc") {
                if(stateVal.p.comp) {
                    after = window.$__uni_mount(dom, stateVal.p.comp, stateVal.p.props, "root");
                    // Record the instance mounted for this slot value so replacing
                    // the slot can dispose it. A portal moves its DOM out of the
                    // range, so clear_range alone cannot reach (and unmount) it.
                    if(window.$__uni_last_mount_instance) start.__uni_slot_inst = window.$__uni_last_mount_instance;
                } else {
                    window.$__uni_dispatch(stateVal.p.name, dom, stateVal.p.props, "root");
                }
            } else if(stateVal.t === window.$_ur.Fragment) {
                after = window.$__uni_hydrate_node(parent, dom, stateVal.c || []);
            } else if(typeof stateVal.t === "function") {
                const nextProps = stateVal.p ? { ...stateVal.p } : {};
                if(stateVal.c && stateVal.c.length) nextProps.children = stateVal.c.length === 1 ? stateVal.c[0] : stateVal.c;
                after = window.$__uni_hydrate_node(parent, dom, stateVal.t(nextProps));
            } else {
                const props = stateVal.p || {};
                for(const k in props) window.$__uni_apply_prop(dom, k, props[k]);
                if(stateVal.c && stateVal.c.length) window.$__uni_hydrate_children(dom, stateVal.c);
                // An element rendered through a reactive slot needs the same ref
                // handling as the generic element branch, otherwise a `ref`
                // callback on such an element is never invoked at hydration.
                if(dom.$__uni_ref !== undefined) {
                    window.$__uni_assign_ref(dom, dom.$__uni_ref);
                    delete dom.$__uni_ref;
                }
                after = dom.nextSibling;
            }
            if(after === undefined) after = dom.nextSibling;
            if(parent) parent.insertBefore(end, after || null);
            v.subscribe((next) => {
                if(start.__uni_slot_inst) { window.$__uni_dispose(start.__uni_slot_inst); start.__uni_slot_inst = null; }
                window.$__uni_clear_range(start, end);
                window.$__uni_last_mount_instance = null;
                start.after(window.$_urn(next));
                // Remember the instance mounted for the NEW value so the next
                // replacement can dispose it (a portal's DOM is not in the range).
                if(window.$__uni_last_mount_instance) start.__uni_slot_inst = window.$__uni_last_mount_instance;
            });
            return end.nextSibling;
        }
        // Reactive value that is an SSR children blob (`window.$__uni_html`):
        // adopt the server-rendered nodes in place and wrap them in markers.
        // Fresh-rendering the blob (the generic path below) sets innerHTML from
        // the blob's template text, which replaces the real SSR DOM -- including
        // nested component boundary elements -- with literal template tags, so
        // the nested components' dispatches then find no target.
        if(stateVal && stateVal.__uni_html !== undefined && dom && dom.nodeType === 1) {
            if(parent) parent.insertBefore(start, dom);
            let cur = dom;
            let nblob = stateVal.__uni_count || 0;
            while(nblob > 0 && cur) { cur = cur.nextSibling; nblob--; }
            if(parent) parent.insertBefore(end, cur);
            v.subscribe((next) => {
                window.$__uni_clear_range(start, end);
                start.after(window.$_urn(next));
            });
            return end.nextSibling;
        }
        // List state: adopt the SSR-rendered range in place instead of
        // re-rendering. Usually the first SSR node is an element; it can be a
        // text node when the first item is a component/fragment whose root starts
        // with text (leading whitespace). A list that SSR did not render at all
        // (e.g. a computed list the server could not fold) leaves only
        // whitespace there, so for plain-element items a text node means "no SSR
        // range" and must fall through to the fresh-render path. Empty lists
        // render nothing and must not consume the following sibling's node.
        const firstListItem = (stateVal && stateVal.length > 0) ? stateVal[0] : null;
        const firstItemCanStartWithText = firstListItem && typeof firstListItem === "object"
            && (firstListItem.t === "__uni_uc" || firstListItem.t === window.$_ur.Fragment || typeof firstListItem.t === "function");
        if(Array.isArray(stateVal) && stateVal.length > 0 && dom && (dom.nodeType === 1 || firstItemCanStartWithText)) {
            if(parent) parent.insertBefore(start, dom);
            let cur = dom;
            const adopted = [];
            // Record each keyed item's adopted DOM range so subsequent client
            // updates can move/patch items by key instead of rebuild.
            const ranges = window.$__uni_key_ranges(start);
            for(let i = 0; i < stateVal.length; i++) {
                const nv = stateVal[i];
                const first = cur;
                const nk = window.$__uni_vnode_key(nv);
                if(nk != null && first && first.nodeType === 1) {
                    first.__uni_vnode_key = nk;
                }
                cur = window.$__uni_hydrate_node(parent, cur, nv);
                if(nk != null && first && first.parentNode === parent && cur !== first) {
                    const last = cur ? cur.previousSibling : parent.lastChild;
                    if(last && last !== start) ranges.set(nk, { first, last });
                }
                adopted.push(nv);
            }
            if(parent) parent.insertBefore(end, cur);
            let tracked = adopted;
            v.subscribe((next) => {
                if(start.__uni_slot_inst) { window.$__uni_dispose(start.__uni_slot_inst); start.__uni_slot_inst = null; }
                window.$__uni_last_mount_instance = null;
                tracked = window.$__uni_reconcile_list(start, end, next, tracked);
                if(window.$__uni_last_mount_instance) start.__uni_slot_inst = window.$__uni_last_mount_instance;
            });
            return end.nextSibling;
        }
        // A reactive value that rendered nothing on the server (a null/false/true
        // conditional, or an empty list) owns no SSR node, so `dom` belongs to
        // the NEXT client vnode: place empty markers without consuming it.
        // Removing it here deleted the following whitespace/Field node and
        // shifted hydration onto the Button (the Field mounted into the
        // <button> root).
        const emptyVal = v.value == null || v.value === false || v.value === true || (Array.isArray(v.value) && v.value.length === 0);
        if(parent) {
            if(dom) { parent.insertBefore(end, dom); parent.insertBefore(start, end); }
            else { parent.appendChild(start); parent.appendChild(end); }
        }
        // An array that had no SSR range (e.g. a checklist empty at render) must
        // reconcile on update instead of clear-and-rebuild: rebuilding replaces
        // the DOM nodes, so an input inside the list loses focus on every
        // keystroke. Scalars keep the clear-and-rebuild path.
        let tracked = Array.isArray(v.value) ? v.value : null;
        v.subscribe((next) => {
            if(start.__uni_slot_inst) { window.$__uni_dispose(start.__uni_slot_inst); start.__uni_slot_inst = null; }
            if(Array.isArray(next)) {
                window.$__uni_last_mount_instance = null;
                tracked = window.$__uni_reconcile_list(start, end, next, tracked);
                if(window.$__uni_last_mount_instance) start.__uni_slot_inst = window.$__uni_last_mount_instance;
            } else {
                window.$__uni_clear_range(start, end);
                window.$__uni_last_mount_instance = null;
                start.after(window.$_urn(next));
                if(window.$__uni_last_mount_instance) start.__uni_slot_inst = window.$__uni_last_mount_instance;
            }
        });
        if(!emptyVal) {
            window.$__uni_last_mount_instance = null;
            start.after(window.$_urn(v.value));
            if(window.$__uni_last_mount_instance) start.__uni_slot_inst = window.$__uni_last_mount_instance;
            // Remove original SSR node that was replaced by state markers to
            // prevent text/element doubling when hydration re-renders the value.
            if(dom && dom.parentNode === parent) { window.$__uni_dispose_subtree(dom); dom.remove(); }
        }
        return end.nextSibling;
    }
    if(typeof v === "string" || typeof v === "number") {
        const nextText = "" + v;
        if(dom && dom.nodeType === 3) {
            const got = dom.textContent;
            if(got === nextText) return dom.nextSibling;
            // The HTML parser merges adjacent server text nodes, so one server
            // text node can hold several client text vnodes' worth of text.
            // Consume only this vnode's prefix and leave the remainder for the
            // next vnode; overwriting here misaligned every following node
            // (source of the "text node differs" flood and div-vs-p mismatches).
            if(nextText.length > 0 && got.length > nextText.length && got.indexOf(nextText) === 0) {
                const rest = dom.nextSibling;
                dom.textContent = nextText;
                const remainder = document.createTextNode(got.slice(nextText.length));
                if(parent) parent.insertBefore(remainder, rest); else dom.after(remainder);
                return remainder;
            }
            // The opposite: this vnode's text spans more than the server node
            // (server kept them separate). Absorb following text siblings.
            if(nextText.length > got.length && nextText.indexOf(got) === 0) {
                let acc = got;
                let cur = dom.nextSibling;
                while(cur && cur.nodeType === 3 && acc.length < nextText.length) {
                    acc += cur.textContent;
                    const nx = cur.nextSibling;
                    cur.remove();
                    cur = nx;
                }
                if(acc === nextText) {
                    dom.textContent = nextText;
                    return dom.nextSibling;
                }
            }
            window.$__uni_warn_hydration("text node differs from SSR", nextText, got);
            dom.textContent = nextText;
            return dom.nextSibling;
        }
        const n = document.createTextNode(nextText);
        if(parent) { if(dom) parent.insertBefore(n, dom); else parent.appendChild(n); }
        return dom;
    }
    if(v && v.__uni_html !== undefined) {
        // Adopt the server-rendered children blob. Advance past `__uni_count`
        // top-level nodes so following sibling vnodes stay aligned (previously
        // this returned the cursor unchanged, so the next sibling mis-adopted
        // the children's DOM — e.g. Tooltip lost its trigger button).
        let n = v.__uni_count || 0;
        let cur = dom;
        while(n > 0 && cur) { cur = cur.nextSibling; n--; }
        return cur;
    }
    if(v && v.t !== undefined) {
        if(v.t === "__uni_uc") {
            const { name, props, comp } = v.p;
            if(comp) {
                // Hydrate the component's whole SSR node range. `dom` may be a
                // text node when the component's root is a fragment/multi-node
                // (e.g. <>{a}{b}</>), so do not require it to be an element.
                if(dom) {
                    const next = window.$__uni_mount(dom, comp, props, "root");
                    return next || dom.nextSibling;
                }
                const container = document.createElement("div");
                if(parent) parent.insertBefore(container, dom);
                window.$__uni_mount(container, comp, props);
                return dom;
            }
            if(dom && dom.nodeType === 1) {
                window.$__uni_dispatch(name, dom, props, "root");
                return dom.nextSibling;
            }
            const container = document.createElement("div");
            if(parent) parent.insertBefore(container, dom);
            window.$__uni_dispatch(name, container, props);
            return dom;
        }
        if(v.t === "__uni_portal") {
            // SSR renders portal children inline (no body on the server). During
            // hydration the SSR'd nodes sit at `dom`; hydrate them in place, then
            // MOVE that range into a container appended to document.body so the
            // content escapes overflow/transform clipping by its ancestors.
            const container = document.createElement("div");
            document.body.appendChild(container);
            window.$__uni_tag_portal(container, v.p);
            window.$__uni_register_portal(container);
            const children = v.c || [];
            if(!dom) {
                for(let i = 0; i < children.length; i++) container.appendChild(window.$_urn(children[i]));
                window.$__uni_inert_scan();
                return dom;
            }
            const startDom = dom;
            let cur = startDom;
            for(let i = 0; i < children.length; i++) {
                cur = window.$__uni_hydrate_node(parent, cur, children[i]);
            }
            let node = startDom;
            while(node && node !== cur) {
                const next = node.nextSibling;
                if(window.$__uni_moving_nodes) window.$__uni_moving_nodes.add(node);
                container.appendChild(node);
                node = next;
            }
            // Rescan now that the moved modal content is in place (see $_urn).
            window.$__uni_inert_scan();
            return cur;
        }
        if(v.t === window.$_ur.Fragment) return window.$__uni_hydrate_node(parent, dom, v.c || []);
        if(typeof v.t === "function") {
            const nextProps = v.p ? { ...v.p } : {};
            if(v.c && v.c.length) nextProps.children = v.c.length === 1 ? v.c[0] : v.c;
            return window.$__uni_hydrate_node(parent, dom, v.t(nextProps));
        }
            if(!dom || dom.nodeType !== 1) {
                if(!dom && parent) {
                    const e = window.$_urn(v);
                    parent.appendChild(e);
                }
                return dom;
            }
            const e = dom;
        const expectedTag = typeof v.t === "string" ? v.t : null;
        if(expectedTag && e.tagName && e.tagName.toLowerCase() !== expectedTag) {
            window.$__uni_warn_hydration("element tag differs from SSR (" + expectedTag + " vs " + e.tagName.toLowerCase() + ")", expectedTag, e.tagName.toLowerCase());
            // Self-correct: replace the wrong SSR element (and its subtree) with
            // a freshly rendered one so the DOM shape matches the client vnode.
            // Previously the vnode's props/children were applied to the wrong
            // tag, so a rich-text <div> stayed the SSR's "Add item" <button>.
            const fresh = window.$_urn(v);
            if(e.parentNode) {
                e.parentNode.replaceChild(fresh, e);
            }
            return fresh ? fresh.nextSibling : null;
        }
        const props = v.p || {};
        for(const k in props) window.$__uni_apply_prop(e, k, props[k]);
        if(v.c && v.c.length) {
            window.$__uni_hydrate_children(e, v.c);
        }
        if(e.$__uni_ref !== undefined) {
            window.$__uni_assign_ref(e, e.$__uni_ref);
            delete e.$__uni_ref;
        }
        return e.nextSibling;
    }
    return dom;
})
window.$__uni_mount = ((host, comp, props, mode = "children") => {
    if(!host) {
        window.$__uni_error("cannot mount universal component without a host");
    }
    if(typeof comp !== "function") {
        window.$__uni_error("universal component factory is invalid", typeof comp);
    }
    // Set up instance tracking for effects. Prefer the mount-stack parent; when
    // a component is dispatched as an independent top-level boundary (no stack
    // parent), derive the parent from DOM ancestry so context and disposal still
    // follow the rendered tree.
    const prevInstance = window.$__uni_current_instance;
    let parentInstance = prevInstance;
    if(!parentInstance && host && host.parentElement) {
        parentInstance = window.$__uni_find_parent_instance(host);
    }
    // Ownership-driven remount: if this host already owns an instance (e.g. a
    // re-dispatch of the same boundary), dispose it first so effects and
    // subscriptions from the previous instance cannot leak. Exception: when the
    // existing owner IS the instance currently rendering us, the host element is
    // shared because this component's root is another component (root = InputGroup).
    // Disposing it would clear the parent's state-signal subscribers, freezing the
    // parent's DOM bindings after hydration.
    if(host.$__uni_instance && host.$__uni_instance !== parentInstance) {
        window.$__uni_dispose(host.$__uni_instance);
        host.$__uni_instance = null;
    }
    const inst = { parent: parentInstance, children: [], _disposables: [], _resources: [], _contexts: {}, host: host };
    host.$__uni_instance = inst;
    if(parentInstance) parentInstance.children.push(inst);
    // Own the render context for the component body: current instance, error
    // boundary, and resource owner all become `inst`, saved atomically.
    window.$__uni_push_ctx({ instance : inst, boundary : inst, renderInstance : inst });
    let out;
    try {
        out = comp(props || {});
    } catch(err) {
        // Use console.warn (not error) so the error boundary catch doesn't
        // trigger Playwright's pageerror listener -- the boundary handles it.
        console.warn("[universal] component render failed:", err.message || err);
        // Look for the nearest error boundary: start with this instance,
        // then walk up the parent chain
        let boundary = inst;
        while(boundary) {
            if(boundary.errorFallback) {
                out = window.$__uni_render_fallback(boundary, props, err);
                break;
            }
            boundary = boundary.parent;
        }
        if(!out) {
            out = window.$__uni_default_fallback(props, err);
        }
    }
    // The body has run: drop render ownership and boundary, but keep
    // current_instance = inst through hydration so child dispatches parent here.
    const frame = window.$__uni_peek_ctx();
    window.$__uni_render_instance = frame ? frame.renderInstance : null;
    window.$__uni_current_boundary = frame ? frame.boundary : null;
    // Ref forwarding: if the parent passed a ref prop, forward it to the
    // component's root DOM element after hydration
    const refVal = props && props.ref ? props.ref : null;
    // Keep current_instance = inst during hydration so child components
    // dispatched via $_uni_dispatch correctly parent to this instance
    if(mode === "root") {
        const parent = host.parentNode;
        if(!parent) {
            window.$__uni_error("cannot hydrate universal root without a parent element", host.tagName ? host.tagName.toLowerCase() : "unknown");
        }
        // Comment boundary (table-context components): the component's own root
        // element starts right after the marker. Text-node hosts (fragment /
        // multi-node root) and element hosts hydrate from the host itself.
        const isCommentHost = host.nodeType === 8;
        const startDom = isCommentHost ? host.nextSibling : host;
        // Keep current_instance = inst while hydrating the component's own
        // output so nested child components parent to this instance (needed for
        // scoped context and disposal). Restore afterwards.
        const next = window.$__uni_hydrate_node(parent, startDom, out);
        window.$__uni_pop_ctx();
        // `startDom` may be a text node when the component's SSR range starts
        // with text (a fragment / multi-node root). Track the first element
        // inside the hydrated range so disposal still works; fall back to the
        // parent.
        let trackedEl = startDom;
        if(!startDom || startDom.nodeType !== 1) {
            let scan = startDom;
            while(scan && scan !== next && scan.nodeType !== 1) scan = scan.nextSibling;
            trackedEl = (scan && scan !== next) ? scan : parent;
        }
        // Expose the instance on the tracked root element so descendant
        // components resolve their parent via DOM ancestry (context/disposal).
        if(trackedEl && trackedEl.nodeType === 1) trackedEl.$__uni_instance = inst;
        window.$__uni_track_instance(trackedEl, inst);
        // Layout effects run synchronously before paint
        if(inst.layoutEffects && inst.layoutEffects.length) window.$__uni_run_effects(inst, inst.layoutEffects);
        if(inst.effects && inst.effects.length) window.$__uni_run_effects(inst, inst.effects);
        // Ref forwarding for root mode
        if(refVal) window.$__uni_assign_ref(trackedEl, refVal);
        window.$__uni_last_mount_instance = inst;
        return next;
    }
    window.$__uni_hydrate_children(host, [ out ]);
    // Restore instance AFTER hydration -- child components dispatched during
    // hydration need $_uni_current_instance set to this inst for parent linking
    window.$__uni_pop_ctx();
    // Track instance for unmount cleanup via MutationObserver.
    // During SSR hydration, prefer the [data-chx-i] boundary element.
    // During dynamic re-renders (via $_urn), `host` is a temporary container
    // that never enters the DOM -- track its first element child instead.
    let trackedEl = host;
    if(host.querySelector) {
        trackedEl = host.querySelector("[data-chx-i]");
    }
    if(!trackedEl || trackedEl === host) {
        // Dynamic component: find the first element child (the component root)
        let child = host.firstChild;
        while(child && child.nodeType !== 1) child = child.nextSibling;
        if(child) trackedEl = child;
    }
    window.$__uni_track_instance(trackedEl, inst);
    // Layout effects run synchronously before paint
    if(inst.layoutEffects && inst.layoutEffects.length) window.$__uni_run_effects(inst, inst.layoutEffects);
    if(inst.effects && inst.effects.length) window.$__uni_run_effects(inst, inst.effects);
    // Ref forwarding: assign the ref prop to the component's root DOM element
    if(refVal && trackedEl && trackedEl !== host) {
        window.$__uni_assign_ref(trackedEl, refVal);
    }
    window.$__uni_last_mount_instance = inst;
})
// Owner tree cleanup: dispose all effects, subscriptions, and child instances
window.$__uni_dispose = ((inst) => {
    if(!inst) return;
    // Guard against re-entrant/cyclic disposal: removing a portal container
    // disposes the instances inside it, which can point back at `inst`.
    if(inst._disposed) return;
    inst._disposed = true;
    // Dispose children first (depth-first)
    for(let i = 0; i < inst.children.length; i++) {
        window.$__uni_dispose(inst.children[i]);
    }
    inst.children = [];
    // Run cleanup functions for effects
    if(inst.effects) {
        for(let i = 0; i < inst.effects.length; i++) {
            const eff = inst.effects[i];
            if(eff.cleanup) {
                try { eff.cleanup(); } catch(err) { console.error("[universal] effect cleanup failed:", err); }
            }
            // Unsubscribe from deps
            if(eff.depUnsubs) {
                for(let j = 0; j < eff.depUnsubs.length; j++) {
                    try { eff.depUnsubs[j](); } catch(err) {}
                }
            }
        }
        inst.effects = [];
    }
    if(inst.layoutEffects) {
        for(let i = 0; i < inst.layoutEffects.length; i++) {
            const eff = inst.layoutEffects[i];
            if(eff.cleanup) {
                try { eff.cleanup(); } catch(err) { console.error("[universal] layout effect cleanup failed:", err); }
            }
            if(eff.depUnsubs) {
                for(let j = 0; j < eff.depUnsubs.length; j++) {
                    try { eff.depUnsubs[j](); } catch(err) {}
                }
            }
        }
        inst.layoutEffects = [];
    }
    // Dispose custom disposables (registered via $_r.useUnmount or similar)
    if(inst._disposables) {
        for(let i = 0; i < inst._disposables.length; i++) {
            try { inst._disposables[i](); } catch(err) {}
        }
        inst._disposables = [];
    }
    // Remove portal containers owned by this component. They live in
    // document.body, so they are not part of the disposed DOM subtree.
    if(inst._portals) {
        let removedAny = false;
        for(let i = 0; i < inst._portals.length; i++) {
            const container = inst._portals[i];
            try {
                if(container) {
                    if(container.hasAttribute("data-uni-modal")) removedAny = true;
                    if(container.$__uni_inert_observer) { container.$__uni_inert_observer.disconnect(); container.$__uni_inert_observer = null; }
                    window.$__uni_dispose_deep(container);
                    if(container.parentNode) container.parentNode.removeChild(container);
                }
            } catch(err) {}
        }
        inst._portals = [];
        // A removed modal container no longer reports as visible, but nothing
        // triggers a rescan by itself (the tag-portal observer watched the
        // container that just left the DOM). Recompute so the background does
        // not stay inert after the modal is gone.
        if(removedAny) window.$__uni_inert_scan();
    }
    // Dispose render-scoped resources (state signals and computeds created by
    // this component). Dropping their subscribers lets the whole graph become
    // unreachable; without this, a long-lived signal would retain every
    // computed/effect ever bound to a removed component.
    if(inst._resources) {
        for(let i = 0; i < inst._resources.length; i++) {
            const res = inst._resources[i];
            try { if(res && res.$_dispose) res.$_dispose(); } catch(err) {}
        }
        inst._resources = [];
    }
    // Remove from parent
    if(inst.parent && inst.parent.children) {
        const idx = inst.parent.children.indexOf(inst);
        if(idx >= 0) inst.parent.children.splice(idx, 1);
    }
    inst.parent = null;
})
// Disposal driven by the reconciler: before the reconciler removes DOM it
// explicitly disposes every component instance in the affected subtree. This
// makes teardown deterministic (triggered by the operation that removes the
// nodes) instead of relying solely on the MutationObserver heuristic below.
// The observer remains as a safety net for removals that bypass the runtime.
// Disposes every component instance owned by `node`'s subtree. Instances are
// found via the `$__uni_instance` property (set on the hydration boundary or
// the component's root element), so this works for both the `<span data-chx-i>`
// wrapper and a comment/table boundary. Falls back to the observer map for
// instances whose host element is no longer reachable.
window.$__uni_dispose_deep = ((node) => {
    const observed = window.$__uni_cleanup_observer && window.$__uni_cleanup_observer.observed;
    const walk = (el) => {
        if(!el || el.nodeType !== 1) return;
        if(el.$__uni_instance) {
            window.$__uni_dispose(el.$__uni_instance);
            el.$__uni_instance = null;
        }
        if(observed) {
            const inst = observed.get(el);
            if(inst) { window.$__uni_dispose(inst); observed.delete(el); }
        }
        // Release refs held by removed elements: a callback ref is invoked with
        // null (and an object ref's `.current` cleared), matching React.
        if(el.$__uni_ref_fn) {
            const refFn = el.$__uni_ref_fn;
            delete el.$__uni_ref_fn;
            try { refFn(null); } catch(err) { console.error("[universal] ref callback failed on unmount:", err); }
        }
        if(el.$__uni_ref_obj) {
            const refObj = el.$__uni_ref_obj;
            delete el.$__uni_ref_obj;
            try { refObj.current = null; } catch(err) {}
        }
        // Drop reactive prop subscriptions made by $__uni_apply_prop. Without
        // this, a removed element kept a live subscription to a (parent-owned)
        // signal and was patched forever after -- a subscriber leak that grows
        // with every mount/unmount cycle.
        if(el.$__uni_prop_subs) {
            const subs = el.$__uni_prop_subs;
            el.$__uni_prop_subs = null;
            for(const pk in subs) { try { subs[pk](); } catch(err) {} }
        }
        for(let c = el.firstChild; c; c = c.nextSibling) {
            if(c.nodeType === 1) walk(c);
        }
    };
    walk(node);
})
window.$__uni_dispose_subtree = ((node) => {
    if(!node || node.nodeType !== 1) return;
    window.$__uni_dispose_deep(node);
})
// MutationObserver to detect DOM removal and clean up owner trees
window.$__uni_cleanup_observer = (() => {
    if(typeof MutationObserver === "undefined") return null;
    const observed = new Map();
    // Nodes that are being relocated on purpose (e.g. portal content moved
    // into a body-appended container during hydration). A move shows up as a
    // childList removal, which would otherwise be mistaken for an unmount and
    // dispose the freshly-hydrated owner instance.
    window.$__uni_moving_nodes = new Set();
    const observer = new MutationObserver((mutations) => {
        for(let i = 0; i < mutations.length; i++) {
            const removed = mutations[i].removedNodes;
            for(let j = 0; j < removed.length; j++) {
                const node = removed[j];
                if(node.nodeType !== 1) continue;
                if(window.$__uni_moving_nodes.has(node)) {
                    window.$__uni_moving_nodes.delete(node);
                    continue;
                }
                // Dispose every component instance in the removed subtree
                // (wrapper spans and comment/table boundaries alike).
                window.$__uni_dispose_deep(node);
            }
        }
    });
    observer.observe(document.body, { childList: true, subtree: true });
    return { observer, observed };
})()
window.$__uni_track_instance = ((host, inst) => {
    if(window.$__uni_cleanup_observer) {
        window.$__uni_cleanup_observer.observed.set(host, inst);
    }
})
window.$_uc = ((factory, props) => {
    if(typeof factory !== "function") {
        window.$__uni_error("universal factory must be a function", typeof factory);
    }
    return window.$_urn(factory(props || {}));
})
window.$__universal_flush = function() {
    const q = window.$__uni_hydration_queue;
    for(let i = 0; i < q.length; i++) {
        const obj = q[i];
        const fn = window[obj[0]];
        if(fn) {
            try {
                window.$__uni_mount(obj[1], fn, obj[2], obj[3])
            } catch(err) {
                console.error("universal hydration failed for component", obj[0], err);
            }
        } else {
            // Component function not yet registered -- log and continue instead
            // of throwing, which would kill the entire flush loop.
            console.error("universal flush: missing component function", obj[0], "- dispatch was queued but fn was never registered");
        }
    }
};
"""))
        pageJsEnd.append_view(std::string_view("window.$__universal_flush();"))
    }

    func getFinalizedPageJs(&self) : std::string {
        var str = std::string();
        str.reserve(pageJs.size() + pageJsEnd.size())
        str.append_view(pageJs.to_view())
        str.append_view(pageJsEnd.to_view())
        return str;
    }

    // given name -> {name}.css, {name}_head.js, {name}.js assets are assumed to exist
    func htmlPageToString(&self, name : &std::string_view, lang : std::string_view = "", htmlClass : std::string_view = "", bodyClass : std::string_view = "") : std::string {
        var str = std::string()
        str.reserve(pageHead.size() + pageHtml.size() + 128)
        str.append_view(std::string_view("<!DOCTYPE html>"))
        appendHtmlTagStart(&mut str, lang, htmlClass)
        str.append_view("<head>")
        str.append_string(&pageHead)
        if(!pageCss.empty()) {
            str.append_view(std::string_view("<link rel=\"stylesheet\" href=\""));
            str.append_view(name)
            str.append_view(std::string_view(".css\">"));
        }
        if(!pageHeadJs.empty()) {
            str.append_view(std::string_view("<script src=\""));
            str.append_view(name)
            str.append_view(std::string_view("_head.js\"></script>"));
        }
        str.append_view(std::string_view("</head>"))
        appendBodyTagStart(&mut str, bodyClass)
        str.append_string(&pageHtml)
        if(!pageJs.empty()) {
            str.append_view(std::string_view("<script src=\""));
            str.append_view(name)
            str.append_view(std::string_view(".js\"></script>"));
        }
        str.append_view(std::string_view("</body></html>"))
        return str;
    }

    func writeToFile(&self, path : &std::string_view) {
        var completePage = toString();
        fs::write_text_file(path.data(), completePage.data() as *u8, completePage.size())
    }

    // given name -> {name}.css, {name}_head.js, {name}.js assets maybe generated
    func writeToDirectory(&self, path : &std::string_view, name : &std::string_view, lang : std::string_view = "", htmlClass : std::string_view = "", bodyClass : std::string_view = "") {

        // TODO only if not exists
        fs::mkdir(path.data());

        // creating the route file at
        var htmlFile = std::string(path.data(), path.size())
        htmlFile.append('/');
        htmlFile.append_view(name);
        htmlFile.append_char_ptr(".html")

        // writing only html to route
        var htmlPage = htmlPageToString(name, lang, htmlClass, bodyClass)
        fs::write_text_file(htmlFile.data(), htmlPage.data() as *u8, htmlPage.size())

        // {name}.css
        if(!pageCss.empty()) {
            // mutated through `append` below, so must be a `var`
            var cssFile = std::string(path.data(), path.size())
            cssFile.append('/');
            cssFile.append_view(name)
            cssFile.append_view(".css")
            fs::write_text_file(cssFile.data(), pageCss.data() as *u8, pageCss.size())
        }

        // {name}_head.js
        if(!pageHeadJs.empty()) {
            var jsHeadFile = std::string(path.data(), path.size())
            jsHeadFile.append('/');
            jsHeadFile.append_view(name)
            jsHeadFile.append_view("_head.js")
            fs::write_text_file(jsHeadFile.data(), pageHeadJs.data() as *u8, pageHeadJs.size())
        }

        // {name}.js
        var finalizedJs = getFinalizedPageJs()
        if(!finalizedJs.empty()) {
            var jsFile = std::string(path.data(), path.size())
            jsFile.append('/');
            jsFile.append_view(name)
            jsFile.append_view(".js")
            fs::write_text_file(jsFile.data(), finalizedJs.data() as *u8, finalizedJs.size())
        }

    }

}
