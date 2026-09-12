
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

    func capture_html_delta_to_js(&mut self, index : ubigint) {
        const delta_size = pageHtml.size() - index;
        if(delta_size == 0) return;
        const delta = pageHtml.data() + index;
        for(var i = 0u; i < delta_size; i++) {
            const c = delta[i];
            if(c == '`') pageJs.append_view("\\`")
            else if(c == '$' && i + 1 < delta_size && delta[i+1] == '{') pageJs.append_view("\\$")
            else if(c == '\\') pageJs.append_view("\\\\")
            else if(c == '<' && i + 6 < delta_size && delta[i+1] == '/' && delta[i+2] == 's' && delta[i+3] == 'c' && delta[i+4] == 'r' && delta[i+5] == 'i' && delta[i+6] == 'p' && delta[i+7] == 't') {
                pageJs.append_view("\\u003C/script>")
                i += 7
            }
            else if(c == '\n') pageJs.append_view("\\n")
            else if(c == '\r') pageJs.append_view("\\r")
            else pageJs.append(c)
        }
        pageHtml.resize(index);
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
        window.$__uni_error("universal mount target is missing", fnName);
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
            if(key === "class" && out[key] && v) {
                out[key] = window.$__uni_value(out[key]) + " " + window.$__uni_value(v);
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
        const prev = window.$__uni_current_tracker;
        window.$__uni_current_tracker = (dep) => {
            if(dep && deps.indexOf(dep) < 0) deps.push(dep);
        };
        const prevChild = window.$__uni_child_tracker;
        window.$__uni_child_tracker = (child) => {
            if(child && children.indexOf(child) < 0) children.push(child);
        };
        cached = fn();
        window.$__uni_child_tracker = prevChild;
        window.$__uni_current_tracker = prev;
        for(let i = 0; i < deps.length; i++) {
            const dep = deps[i];
            if(dep && typeof dep.subscribe === "function") {
                depUnsubs.push(dep.subscribe(() => recompute()));
            }
        }
        emit();
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
window.$__uni_current_instance = null;
window.$__uni_current_boundary = null;
window.$__uni_render_instance = null;
window.$__uni_ctx = {}
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
        inst.layoutEffects.push({ fn, deps, lastDeps: null, cleanup: null });
    },
    useState: (initial) => {
        const s = window.$_us(initial);
        return [ s, (next) => { s.value = next; } ];
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
    // Name-keyed context registry. Provider and consumer components derive the
    // same key from a shared `name` prop ("rg-" + props.name), so no module-level
    // declarations are needed - each component's JS function is only emitted
    // when used, and the registry itself always lives in the runtime. Reading
    // `.value` inside a $_ucs() computed subscribes like any other signal;
    // assigning a signal to `.value` wires the context to follow it (the
    // provider publishes its state signal).
    createContext: (name, defaultValue) => {
        let entry = window.$__uni_ctx[name];
        if(!entry) {
            const sig = window.$_us(defaultValue);
            entry = {
                name,
                get value() {
                    return sig.value;
                },
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
            window.$__uni_ctx[name] = entry;
        }
        return entry;
    },
    useContext: (name) => {
        if(!window.$__uni_ctx[name]) {
            window.$__uni_ctx[name] = window.$_r.createContext(name, undefined);
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
    // Set the current boundary to this instance so effect errors can propagate
    const prevBoundary = window.$__uni_current_boundary;
    window.$__uni_current_boundary = inst;
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
    window.$__uni_current_boundary = prevBoundary;
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
window.$_uc_h = ((html, name, props) => ({ t: "__uni_uc", p: { html, name, props } }))
window.$__uni_value = ((v) => window.$__uni_is_state(v) ? v.value : v)
window.$__uni_html = ((html) => ({ __uni_html: html || "" }))
window.$__uni_is_active_editable = ((el) => !!(el && el.isContentEditable && document.activeElement === el))
window.$__uni_assign_ref = ((el, refValue) => {
    if(refValue == null || refValue === false) return;
    if(typeof refValue === "function") {
        refValue(el);
        return;
    }
    if(typeof refValue === "object" && "current" in refValue) {
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
            for(const sk in v) el.style[sk] = window.$__uni_value(v[sk]);
        } else {
            window.$__uni_error("invalid style value", typeof v + " on <" + el.tagName.toLowerCase() + ">");
        }
        return;
    }
    if(key.length > 2 && key[0] === "o" && key[1] === "n") {
        const eventName = key.substring(2).toLowerCase();
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
    if(window.$__uni_is_state(value)) {
        value.subscribe((next) => window.$__uni_set_prop(el, key, next));
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
// Reconcile a vnode array into the comment-delimited DOM range
// (start, end). Keyed arrays match old nodes by `__uni_vnode_key` and move them
// (preserving identity, focus, and input values); unkeyed arrays are replaced.
// Returns the vnode array to track for the next reconcile. Shared by fresh
// renders ($_urn) and hydration adoption so both use identical semantics.
window.$__uni_reconcile_list = ((start, end, next, oldVnodes) => {
    const isKeyedArray = Array.isArray(next) && next.length > 0 && next[0] && next[0].p && next[0].p.key != null;
    if(!isKeyedArray) {
        window.$__uni_clear_range(start, end);
        start.after(window.$_urn(next));
        return null;
    }
    const oldMap = new Map();
    if(oldVnodes && start.parentNode) {
        for(let i = 0; i < oldVnodes.length; i++) {
            const ov = oldVnodes[i];
            if(ov && ov.p && ov.p.key != null) {
                let el = start.nextSibling;
                while(el && el !== end) {
                    if(el.__uni_vnode_key === ov.p.key) { oldMap.set(ov.p.key, { vnode: ov, el: el }); break; }
                    el = el.nextSibling;
                }
            }
        }
    }
    let anchor = start;
    const newVnodes = [];
    for(let i = 0; i < next.length; i++) {
        const nv = next[i];
        const nk = nv && nv.p ? nv.p.key : null;
        if(nk != null && oldMap.has(nk)) {
            const entry = oldMap.get(nk);
            const el = entry.el;
            oldMap.delete(nk);
            el.__uni_vnode_key = nk;
            const props = nv.p || {};
            for(const pk in props) window.$__uni_set_prop(el, pk, props[pk]);
            const oldChildren = [];
            let c = el.firstChild;
            while(c) { oldChildren.push(c); c = c.nextSibling; }
            for(let ci = 0; ci < oldChildren.length; ci++) { window.$__uni_dispose_subtree(oldChildren[ci]); oldChildren[ci].remove(); }
            const children = nv.c || [];
            for(let ci = 0; ci < children.length; ci++) el.appendChild(window.$_urn(children[ci]));
            if(el.nextSibling !== anchor.nextSibling) {
                el.remove();
                anchor.after(el);
            }
            anchor = el;
            newVnodes.push(nv);
        } else {
            const rendered = window.$_urn(nv);
            if(nk != null) {
                let tempEl = rendered;
                if(rendered.nodeType === 11) tempEl = rendered.firstChild;
                if(tempEl && tempEl.nodeType === 1) tempEl.__uni_vnode_key = nk;
            }
            anchor.after(rendered);
            anchor = anchor.nextSibling;
            while(anchor && anchor !== end && anchor.nodeType !== 1) anchor = anchor.nextSibling;
            if(!anchor || anchor === end) anchor = end.previousSibling || start;
            newVnodes.push(nv);
        }
    }
    oldMap.forEach((entry) => { window.$__uni_dispose_subtree(entry.el); entry.el.remove(); });
    return newVnodes;
})
window.$_urn = ((v) => {
    if(v == null || v === false || v === true) return document.createTextNode("");
    if(window.$__uni_is_state(v)) {
        const start = document.createComment("s");
        const end = document.createComment("e");
        const f = document.createDocumentFragment();
        f.appendChild(start);
        f.appendChild(end);
        let oldVnodes = null;
        v.subscribe((next) => {
            oldVnodes = window.$__uni_reconcile_list(start, end, next, oldVnodes);
        });
        oldVnodes = window.$__uni_reconcile_list(start, end, v.value, oldVnodes);
        return f;
    }
    if(v.nodeType) return v;
    if(Array.isArray(v)) {
        const f = document.createDocumentFragment();
        for(let i = 0; i < v.length; i++) f.appendChild(window.$_urn(v[i]));
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
            const { html, name, props } = v.p;
            const container = document.createElement("div");
            if (html) {
                container.innerHTML = html;
                window.$__uni_dispatch(name, container, props);
                const f = document.createDocumentFragment();
                while(container.firstChild) f.appendChild(container.firstChild);
                return f;
            }
            window.$__uni_dispatch(name, container, props);
            return container;
        }
        if(v.t === window.$_ur.Fragment) {
            const f = document.createDocumentFragment();
            for(let i = 0; i < (v.c || []).length; i++) f.appendChild(window.$_urn(v.c[i]));
            return f;
        }
        if(v.t === "__uni_portal") {
            const container = document.createElement("div");
            document.body.appendChild(container);
            window.$__uni_tag_portal(container, v.p);
            const children = v.c || [];
            for(let i = 0; i < children.length; i++) container.appendChild(window.$_urn(children[i]));
            return container;
        }
        if(typeof v.t === "function") {
            const nextProps = v.p ? { ...v.p } : {};
            if(v.c && v.c.length) nextProps.children = v.c.length === 1 ? v.c[0] : v.c;
            return window.$_urn(v.t(nextProps));
        }
        const ns = window.$__uni_ns[v.t];
        const e = ns ? document.createElementNS(ns, v.t) : document.createElement(v.t);
        const props = v.p || {};
        for(const k in props) window.$__uni_apply_prop(e, k, props[k]);
        const children = v.c || [];
        for(let i = 0; i < children.length; i++) e.appendChild(window.$_urn(children[i]));
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
        // markers: adopt the existing element in place instead of appending a
        // duplicate copy. Wrap it in markers so later updates can swap it out.
        if(dom && dom.nodeType === 1 && stateVal && typeof stateVal === "object" && stateVal.t !== undefined) {
            if(parent) {
                parent.insertBefore(start, dom);
                parent.insertBefore(end, dom.nextSibling);
            }
            v.subscribe((next) => {
                window.$__uni_clear_range(start, end);
                start.after(window.$_urn(next));
            });
            if(stateVal.t === "__uni_uc") {
                window.$__uni_dispatch(stateVal.p.name, dom, stateVal.p.props, "root");
            } else if(stateVal.t === window.$_ur.Fragment) {
                window.$__uni_hydrate_node(parent, dom, stateVal.c || []);
            } else if(typeof stateVal.t === "function") {
                const nextProps = stateVal.p ? { ...stateVal.p } : {};
                if(stateVal.c && stateVal.c.length) nextProps.children = stateVal.c.length === 1 ? stateVal.c[0] : stateVal.c;
                window.$__uni_hydrate_node(parent, dom, stateVal.t(nextProps));
            } else {
                const props = stateVal.p || {};
                for(const k in props) window.$__uni_apply_prop(dom, k, props[k]);
                if(stateVal.c && stateVal.c.length) window.$__uni_hydrate_children(dom, stateVal.c);
            }
            return end.nextSibling;
        }
        // List state: adopt the SSR-rendered range in place instead of
        // re-rendering. Only when the first child is an element (a real SSR
        // list item); an empty server list leaves a text node, which must fall
        // through to the fresh-render path or the list would double up.
        if(Array.isArray(stateVal) && dom && dom.nodeType === 1) {
            if(parent) parent.insertBefore(start, dom);
            let cur = dom;
            const adopted = [];
            for(let i = 0; i < stateVal.length; i++) {
                const nv = stateVal[i];
                const node = cur;
                if(nv && nv.p && nv.p.key != null && node && node.nodeType === 1) {
                    node.__uni_vnode_key = nv.p.key;
                }
                cur = window.$__uni_hydrate_node(parent, cur, nv);
                adopted.push(nv);
            }
            if(parent) parent.insertBefore(end, cur);
            let tracked = adopted;
            v.subscribe((next) => {
                tracked = window.$__uni_reconcile_list(start, end, next, tracked);
            });
            return end.nextSibling;
        }
        if(parent) {
            if(dom) { parent.insertBefore(end, dom); parent.insertBefore(start, end); }
            else { parent.appendChild(start); parent.appendChild(end); }
        }
        v.subscribe((next) => {
            window.$__uni_clear_range(start, end);
            start.after(window.$_urn(next));
        });
        start.after(window.$_urn(v.value));
        // Remove original SSR node that was replaced by state markers to
        // prevent text/element doubling when hydration re-renders the value.
        if(dom && dom.parentNode === parent) { window.$__uni_dispose_subtree(dom); dom.remove(); }
        return end.nextSibling;
    }
    if(typeof v === "string" || typeof v === "number") {
        const nextText = "" + v;
        if(dom && dom.nodeType === 3) {
            if(dom.textContent !== nextText) {
                window.$__uni_warn_hydration("text node differs from SSR", nextText, dom.textContent);
            }
            dom.textContent = nextText;
            return dom.nextSibling;
        }
        const n = document.createTextNode(nextText);
        if(parent) { if(dom) parent.insertBefore(n, dom); else parent.appendChild(n); }
        return dom;
    }
    if(v && v.__uni_html !== undefined) return dom; // SSRed content handled by parent
    if(v && v.t !== undefined) {
        if(v.t === "__uni_uc") {
            const { name, props } = v.p;
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
            const children = v.c || [];
            if(!dom) {
                for(let i = 0; i < children.length; i++) container.appendChild(window.$_urn(children[i]));
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
    // Ownership-driven remount: if this host already owns an instance (e.g. a
    // re-dispatch of the same boundary), dispose it first so effects and
    // subscriptions from the previous instance cannot leak.
    if(host.$__uni_instance) {
        window.$__uni_dispose(host.$__uni_instance);
        host.$__uni_instance = null;
    }
    // Set up instance tracking for effects
    const prevInstance = window.$__uni_current_instance;
    const inst = { parent: prevInstance, children: [], _disposables: [], _resources: [] };
    host.$__uni_instance = inst;
    if(prevInstance) prevInstance.children.push(inst);
    window.$__uni_current_instance = inst;
    // Set this as the nearest error boundary for child renders
    const prevBoundary = window.$__uni_current_boundary;
    window.$__uni_current_boundary = inst;
    // Resources (signals/computeds) created while the component renders are
    // attributed to this instance and disposed on unmount.
    const prevRenderInstance = window.$__uni_render_instance;
    window.$__uni_render_instance = inst;
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
    window.$__uni_render_instance = prevRenderInstance;
    window.$__uni_current_boundary = prevBoundary;
    // Ref forwarding: if the parent passed a ref prop, forward it to the
    // component's root DOM element after hydration
    const refVal = props && props.ref ? props.ref : null;
    // Keep current_instance = inst during hydration so child components
    // dispatched via $_uni_dispatch correctly parent to this instance
    if(mode === "root") {
        window.$__uni_current_instance = prevInstance;
        const parent = host.parentNode;
        if(!parent) {
            window.$__uni_error("cannot hydrate universal root without a parent element", host.tagName ? host.tagName.toLowerCase() : "unknown");
        }
        window.$__uni_hydrate_node(parent, host, out);
        // Track instance for unmount cleanup via MutationObserver
        window.$__uni_track_instance(host, inst);
        // Layout effects run synchronously before paint
        if(inst.layoutEffects && inst.layoutEffects.length) window.$__uni_run_effects(inst, inst.layoutEffects);
        if(inst.effects && inst.effects.length) window.$__uni_run_effects(inst, inst.effects);
        // Ref forwarding for root mode
        if(refVal) window.$__uni_assign_ref(host, refVal);
        return;
    }
    window.$__uni_hydrate_children(host, [ out ]);
    // Restore instance AFTER hydration -- child components dispatched during
    // hydration need $_uni_current_instance set to this inst for parent linking
    window.$__uni_current_instance = prevInstance;
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
})
// Owner tree cleanup: dispose all effects, subscriptions, and child instances
window.$__uni_dispose = ((inst) => {
    if(!inst) return;
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
window.$__uni_dispose_subtree = ((node) => {
    if(!node || node.nodeType !== 1) return;
    const observed = window.$__uni_cleanup_observer && window.$__uni_cleanup_observer.observed;
    const disposeHost = (el) => {
        if(el && el.$__uni_instance) {
            window.$__uni_dispose(el.$__uni_instance);
            el.$__uni_instance = null;
        }
        if(observed) {
            const inst = observed.get(el);
            if(inst) { window.$__uni_dispose(inst); observed.delete(el); }
        }
    };
    disposeHost(node);
    const spans = node.querySelectorAll ? node.querySelectorAll("[data-chx-i]") : [];
    for(let i = 0; i < spans.length; i++) disposeHost(spans[i]);
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
                // Check for component boundary spans
                const spans = node.querySelectorAll ? node.querySelectorAll("[data-chx-i]") : [];
                for(let k = 0; k < spans.length; k++) {
                    const inst = observed.get(spans[k]);
                    if(inst) {
                        window.$__uni_dispose(inst);
                        observed.delete(spans[k]);
                    }
                }
                // Check the node itself
                const inst = observed.get(node);
                if(inst) {
                    window.$__uni_dispose(inst);
                    observed.delete(node);
                }
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
            const cssFile = std::string(path.data(), path.size())
            cssFile.append('/');
            cssFile.append_view(name)
            cssFile.append_view(".css")
            fs::write_text_file(cssFile.data(), pageCss.data() as *u8, pageCss.size())
        }

        // {name}_head.js
        if(!pageHeadJs.empty()) {
            const jsHeadFile = std::string(path.data(), path.size())
            jsHeadFile.append('/');
            jsHeadFile.append_view(name)
            jsHeadFile.append_view("_head.js")
            fs::write_text_file(jsHeadFile.data(), pageHeadJs.data() as *u8, pageHeadJs.size())
        }

        // {name}.js
        var finalizedJs = getFinalizedPageJs()
        if(!finalizedJs.empty()) {
            const jsFile = std::string(path.data(), path.size())
            jsFile.append('/');
            jsFile.append_view(name)
            jsFile.append_view(".js")
            fs::write_text_file(jsFile.data(), finalizedJs.data() as *u8, finalizedJs.size())
        }

    }

}
