
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
        return !doneClasses.contains(hash)
    }

    func set_css_hash(&mut self, hash : size_t) {
        doneClasses.insert(hash, true)
    }

    func require_component(&self, hash : size_t) : bool {
        return !doneComponents.contains(hash)
    }

    func set_component_hash(&mut self, hash : size_t) {
        doneComponents.insert(hash, true)
    }

    func require_random_css_hash(&self, hash : size_t) : bool {
        return !doneRandomClasses.contains(hash)
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
            str.append_view(lang)
            str.append_view("\">")
        } else if(lang.empty()) {
            str.append_view("<html class=\"")
            str.append_view(htmlClass)
            str.append_view("\">")
        } else {
            str.append_view("<html lang=\"")
            str.append_view(lang)
            str.append_view("\" class=\"")
            str.append_view(htmlClass)
            str.append_view("\">")
        }
    }

    func appendBodyTagStart(str : &mut std::string, bodyClass : std::string_view = "") {
        if(bodyClass.empty()) {
            str.append_view("<body>")
        } else {
            str.append_view("<body class=\"")
            str.append_view(bodyClass)
            str.append_view("\">")
        }
    }

    func toString(&self, lang : std::string_view = "", htmlClass : std::string_view = "", bodyClass : std::string_view = "") : std::string {
        var str = std::string()
        str.reserve(pageHead.size() + pageCss.size() + pageHtml.size() + pageHeadJs.size() + pageJs.size() + 100)
        str.append_view(std::string_view("<!DOCTYPE html>"))
        appendHtmlTagStart(str, lang, htmlClass)
        str.append_view("<head>")
        str.append_string(pageHead)
        if(!pageCss.empty()) {
            str.append_view(std::string_view("<style>"))
            str.append_string(pageCss)
            str.append_view(std::string_view("</style>"))
        }
        if(!pageHeadJs.empty()) {
            str.append_view(std::string_view("<script>"))
            str.append_string(pageHeadJs)
            str.append_view(std::string_view("</script>"))
        }
        str.append_view(std::string_view("</head>"))
        appendBodyTagStart(str, bodyClass)
        str.append_string(pageHtml)
        var finalizedJs = getFinalizedPageJs()
        if(!finalizedJs.empty()) {
            str.append_view(std::string_view("<script>"))
            str.append_string(finalizedJs)
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
        str.append_string(pageHtml)
        return str;
    }

    func toStringCssOnly(&self) : std::string {
        var str = std::string()
        str.reserve(pageCss.size())
        str.append_string(pageCss)
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

    // TODO: why is the preact setup so large, fix that
    func defaultPreactSetup(&mut self) {
        pageHead.append_view(std::string_view("""<script src="https://unpkg.com/preact/dist/preact.min.js"></script><script src="https://unpkg.com/preact/hooks/dist/hooks.umd.js"></script>"""))
        pageHeadJs.append_view(std::string_view("""window.$_p = preact;window.$_ph = preactHooks; window.$_pm = (e,c,p) => {const P = document.createDocumentFragment(); $_p.render($_p.h(c, p || {}), P); e.replaceWith(P); };"""))
        // universal component bridge for preact
        pageHeadJs.append_view(std::string_view("""
function UniPreactBridge({ html, fnName, props }) {
    const ref = $_ph.useRef(null);
    $_ph.useLayoutEffect(() => {
        const target = ref.current;
        window.$__uni_dispatch(fnName, target, props);
    }, [fnName, props]);
    return $_p.h('div', {
        ref,
        style: "display: contents",
        dangerouslySetInnerHTML: { __html: html }
    });
}
const $p_uni_ch = (html, fnName, props) => $_p.h(UniPreactBridge, { html, fnName, props });
"""))
    }

    func reactHeadJs(&mut self) {
        pageHeadJs.append_view(std::string_view("""window.$_r = React; window.$_rd = ReactDOM; window.$_rm = (e, c, p) => { const P = document.createElement("div"); e.replaceWith(P); $_rd.createRoot(P).render($_r.createElement(c, p || {})); }"""));
        pageHeadJs.append_view(std::string_view("""
function UniReactBridge({ html, fnName, props }) {
    const ref = $_r.useRef(null);
    $_r.useLayoutEffect(() => {
        const target = ref.current;
        window.$__uni_dispatch(fnName, target, props);
    }, [fnName, props]);
    return $_r.createElement('div', {
        ref,
        style: { display: 'contents' },
        dangerouslySetInnerHTML: { __html: html }
    });
}
const $r_uni_ch = (html, fnName, props) => $_r.createElement(UniReactBridge, { html, fnName, props });
"""))
    }

    func defaultDevelopmentReactSetup(&mut self) {
        pageHead.append_view(std::string_view("""<script src="https://unpkg.com/react@18/umd/react.development.js"></script><script src="https://unpkg.com/react-dom@18/umd/react-dom.development.js"></script>"""))
        reactHeadJs()
    }

    func defaultReactSetup(&mut self) {
        pageHead.append_view(std::string_view("""<script src="https://unpkg.com/react@18/umd/react.production.min.js"></script><script src="https://unpkg.com/react-dom@18/umd/react-dom.production.min.js"></script>"""))
        reactHeadJs()
    }

    func defaultUniversalSetup(&mut self) {
        // we must not put anything else in the head js
        // everything else must go into body js
        // this is just required for react, solid and preact integration to work
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
        window.$__uni_mount(target, fn, props, mode);
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
            if(key === "class" && out[key] && part[k]) {
                out[key] = out[key] + " " + part[k];
            } else {
                out[key] = part[k];
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
    return {
        get value() {
            if(window.$__uni_current_tracker) window.$__uni_current_tracker(this);
            return val;
        },
        set value(n) {
            val = n;
            const snapshot = subs.slice();
            for(let i = 0; i < snapshot.length; i++) snapshot[i](val);
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
        for(let i = 0; i < children.length; i++) {
            if(children[i].$_uc_dispose) children[i].$_uc_dispose();
        }
        children = [];
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
window.$_r = {
    useEffect: (fn, deps) => {
        const inst = window.$__uni_current_instance;
        if(!inst) return;
        if(!inst.effects) inst.effects = [];
        inst.effects.push({ fn, deps, lastDeps: null, cleanup: null });
    },
    useLayoutEffect: (fn, deps) => {
        const inst = window.$__uni_current_instance;
        if(!inst) return;
        if(!inst.layoutEffects) inst.layoutEffects = [];
        inst.layoutEffects.push({ fn, deps, lastDeps: null, cleanup: null });
    }
}
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
            if(eff.cleanup) eff.cleanup();
            eff.cleanup = eff.fn();
            if(eff.deps) eff.lastDeps = eff.deps.map(window.$__uni_value);
            else eff.lastDeps = [];
        }
    }
})
window.$__uni_is_state = ((v) => !!(v && typeof v.subscribe === "function" && "value" in v))
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
        el.$__uni_events[eventName] = v;
        el.addEventListener(eventName, v);
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
        if(parent) {
            if(dom) { parent.insertBefore(end, dom); parent.insertBefore(start, end); }
            else { parent.appendChild(start); parent.appendChild(end); }
        }
        v.subscribe((next) => {
            while(start.nextSibling && start.nextSibling !== end) start.nextSibling.remove();
            start.after(window.$_urn(next));
        });
        start.after(window.$_urn(v.value));
        return dom;
    }
    if(typeof v === "string" || typeof v === "number") {
        if(dom && dom.nodeType === 3) {
            dom.textContent = "" + v;
            return dom.nextSibling;
        }
        const n = document.createTextNode("" + v);
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
    const out = comp(props || {});
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
        const obj = window.$__uni_hydration_queue[i];
        const fn = window[obj[0]];
        if(fn) {
            window.$__uni_mount(obj[1], fn, obj[2], obj[3])
        } else {
            window.$__uni_error("missing component function by name", obj[0]);
        }
    }
};
"""))
        pageJsEnd.append_view(std::string_view("window.$__universal_flush();"))
    }

    func asynchronousSolidSetup(&mut self) {
        pageHead.append_view(std::string_view("""<script type="module">
            import * as solid from 'https://esm.sh/solid-js@1.9.10';
            import * as web from 'https://esm.sh/solid-js@1.9.10/web'; 
            import h from 'https://esm.sh/solid-js@1.9.10/h';
            window.$_s = solid;
            window.$_sw = web;
            window.$_sh = h;
            if(window.$_sc) window.$_sc();
        </script> 
        <script>
            let $_sr_queue = [];
            function $_sr(e, comp, props) {
                const mount = document.createElement('div');
                e.replaceWith(mount);
                $_sw.render(() => comp(props || {}), mount);
            }
            window.$_sc = () => {
                for(var i = 0; i < $_sr_queue.length; i++) {
                    const q = $_sr_queue[i];
                    $_sr(q[0], q[1], q[2])
                }
                $_sr_queue = [];
            }
            window.$_sm = (e, comp, props) => {
                if(window.$_sw) {
                    $_sr(e, comp, props)
                } else {
                    $_sr_queue.push([e, comp, props])
                }
            };
        </script>"""))
    }

    func defaultSolidSetup(&mut self) {
        pageHead.append_view(std::string_view("""<script src="https://unpkg.com/solid-umd@1.9.10/dist/solid.min.js"></script><script src="https://unpkg.com/solid-umd@1.9.10/dist/solid-web.min.js"></script><script src="https://unpkg.com/solid-umd@1.9.10/dist/solid-h.min.js"></script>"""))
        pageHeadJs.append_view(std::string_view("""window.$_s = Solid; window.$_sw = SolidWeb; window.$_sh = SolidH.default || SolidH; window.$_sm = (e, comp, props) => { const mount = document.createElement('div'); e.replaceWith(mount); $_sw.render(() => comp(props || {}), mount); };"""))
        pageHeadJs.append_view(std::string_view("""
function UniSolidBridge(props) {
   let el;
   $_s.onMount(() => {
       window.$__uni_dispatch(props.fnName, el, props.props);
   });
   return $_sh("div", {
       ref: (r) => el = r,
       style: { display: "contents" },
       innerHTML: props.html
   });
}
const $s_uni_ch = (html, fnName, props) => $_sh(UniSolidBridge, { html, fnName, props });
"""))
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
        appendHtmlTagStart(str, lang, htmlClass)
        str.append_view("<head>")
        str.append_string(pageHead)
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
        appendBodyTagStart(str, bodyClass)
        str.append_string(pageHtml)
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
