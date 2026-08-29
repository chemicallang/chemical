
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
        try {
            window.$__uni_mount(target, fn, props, mode);
        } catch(err) {
            console.error("universal mount failed for component", fnName, err);
        }
    } else {
        window.$__uni_hydration_queue.push([ fnName, target, props, mode ]);
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
window.$_us = ((v) => {
    let val = v;
    const subs = [];
    const _inst = window.$__uni_current_instance;
    return {
        get value() {
            if(window.$__uni_current_tracker) window.$__uni_current_tracker(this);
            return val;
        },
        set value(n) {
            val = n;
            const snapshot = subs.slice();
            for(let i = 0; i < snapshot.length; i++) snapshot[i](val);
            if(_inst && !_inst._pendingEffects) {
                _inst._pendingEffects = true;
                Promise.resolve().then(() => {
                    _inst._pendingEffects = false;
                    if(_inst.effects && _inst.effects.length) window.$__uni_run_effects(_inst, _inst.effects);
                });
            }
        },
        subscribe(fn) {
            subs.push(fn);
            return () => {
                const idx = subs.indexOf(fn);
                if(idx >= 0) subs.splice(idx, 1);
            };
        }
    };
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
            subs.push(fn);
            return () => {
                const idx = subs.indexOf(fn);
                if(idx >= 0) subs.splice(idx, 1);
            };
        }
    };
    signal.$_uc_dispose = dispose;
    if(window.$__uni_child_tracker) window.$__uni_child_tracker(signal);
    return signal;
})
window.$__uni_current_instance = null;
window.$__uni_ctx = {}
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
                        if(!inst._pendingEffects) {
                            inst._pendingEffects = true;
                            Promise.resolve().then(() => {
                                inst._pendingEffects = false;
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
    useMemo: (fn) => window.$_ucs(() => fn()),
    useCallback: (fn) => fn,
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
    if(opts && opts.modal) container.setAttribute("data-uni-modal", "");
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
    for(let i = 0; i < effects.length; i++) {
        const eff = effects[i];
        let changed = !eff.lastDeps;
        if(!changed && eff.deps) {
            for(let j = 0; j < eff.deps.length; j++) {
                if(window.$__uni_value(eff.deps[j]) !== eff.lastDeps[j]) {
                    changed = true; break;
                }
            }
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
            if(eff.deps) eff.lastDeps = eff.deps.map(window.$__uni_value);
            else eff.lastDeps = [];
        }
    }
})
window.$__uni_is_state = ((v) => !!(v && typeof v.subscribe === "function" && "value" in v))
window.$__uni_warn_hydration = ((msg, expected, got) => {
    // Hydration mismatches are reported loudly in dev but never crash the
    // page in production: the runtime already self-corrects below. Guarded so
    // a busy page with many components doesn't spam thousands of duplicates.
    if(window.$__uni_hydration_warned) return;
    window.$__uni_hydration_warned = true;
    console.warn("[universal] hydration mismatch: " + msg, expected, got);
    console.warn("[universal] further hydration mismatch warnings suppressed; fix the component source (see components_e2e skill)");
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
    if(key === "children" || key == null) return;
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
        const wrapped = (e) => {
            try {
                v(e);
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
window.$_urn = ((v) => {
    if(v == null || v === false || v === true) return document.createTextNode("");
    if(window.$__uni_is_state(v)) {
        const start = document.createComment("s");
        const end = document.createComment("e");
        const f = document.createDocumentFragment();
        f.appendChild(start);
        f.appendChild(end);
        v.subscribe((next) => {
            while(start.nextSibling && start.nextSibling !== end) start.nextSibling.remove();
            start.after(window.$_urn(next));
        });
        start.after(window.$_urn(v.value));
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
        const e = document.createElement(v.t);
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
                while(start.nextSibling && start.nextSibling !== end) start.nextSibling.remove();
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
                while(start.nextSibling && start.nextSibling !== end) start.nextSibling.remove();
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
        if(parent) {
            if(dom) { parent.insertBefore(end, dom); parent.insertBefore(start, end); }
            else { parent.appendChild(start); parent.appendChild(end); }
        }
        v.subscribe((next) => {
            while(start.nextSibling && start.nextSibling !== end) start.nextSibling.remove();
            start.after(window.$_urn(next));
        });
        start.after(window.$_urn(v.value));
        // Remove original SSR node that was replaced by state markers to
        // prevent text/element doubling when hydration re-renders the value.
        if(dom && dom.parentNode === parent) dom.remove();
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
    // Set up instance tracking for effects
    const prevInstance = window.$__uni_current_instance;
    const inst = {};
    window.$__uni_current_instance = inst;
    let out;
    try {
        out = comp(props || {});
    } catch(err) {
        console.error("[universal] component render failed:", err);
        out = window.$__uni_render_fallback(inst, props, err);
    }
    window.$__uni_current_instance = prevInstance;
    if(mode === "root") {
        const parent = host.parentNode;
        if(!parent) {
            window.$__uni_error("cannot hydrate universal root without a parent element", host.tagName ? host.tagName.toLowerCase() : "unknown");
        }
        window.$__uni_hydrate_node(parent, host, out);
        if(inst.effects && inst.effects.length) window.$__uni_run_effects(inst, inst.effects);
        return;
    }
    window.$__uni_hydrate_children(host, [ out ]);
    if(inst.effects && inst.effects.length) window.$__uni_run_effects(inst, inst.effects);
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
            window.$__uni_error("missing component function by name", obj[0]);
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
