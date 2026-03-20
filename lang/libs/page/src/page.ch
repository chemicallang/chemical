
public struct HtmlPage {

    var pageHead : std::string

    var pageHtml : std::string

    var pageCss : std::string

    var pageJs : std::string

    var pageHeadJs : std::string

    var pageJsEnd : std::string

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
        pageHtml.resize_unsafe(size);
    }

    func append_head(&mut self, value : *char, len : size_t) {
        pageHead.append_with_len(value, len);
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
window.$__uni_dispatch = ((fnName, target, props) => {
    const fn = window[fnName]
    if(fn) {
        window.$__uni_mount(target, fn, props);
    } else {
        window.$__uni_hydration_queue.push([ fnName, target, props ]);
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
        get value() { return val; },
        set value(n) {
            val = n;
            for(let i = 0; i < subs.length; i++) subs[i](val);
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
window.$__uni_is_state = ((v) => !!(v && typeof v.subscribe === "function" && "value" in v))
window.$__uni_value = ((v) => window.$__uni_is_state(v) ? v.value : v)
window.$__uni_html = ((html) => ({ __uni_html: html || "" }))
window.$__uni_set_prop = ((el, key, value) => {
    const v = window.$__uni_value(value);
    if(key === "children" || key === "ref" || key == null) return;
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
            for(const sk in v) el.style[sk] = window.$__uni_value(v[sk]);
        }
        return;
    }
    if(key.length > 2 && key[0] === "o" && key[1] === "n" && typeof v === "function") {
        const eventName = key.substring(2).toLowerCase();
        if(!el.$__uni_events) el.$__uni_events = {};
        const prev = el.$__uni_events[eventName];
        if(prev) el.removeEventListener(eventName, prev);
        el.$__uni_events[eventName] = v;
        el.addEventListener(eventName, v);
        return;
    }
    if(v == null || v === false) {
        if(key in el && typeof el[key] !== "function") {
            try { el[key] = ""; } catch(_) {}
        }
        el.removeAttribute(key);
        return;
    }
    if(key in el && key !== "list" && key !== "type") {
        try {
            el[key] = v;
            return;
        } catch(_) {}
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
        const n = document.createTextNode(v.value == null ? "" : "" + v.value);
        v.subscribe((next) => { n.textContent = next == null ? "" : "" + next; });
        return n;
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
        if(v.t === window.$_ur.Fragment) {
            const f = document.createDocumentFragment();
            for(let i = 0; i < v.c.length; i++) f.appendChild(window.$_urn(v.c[i]));
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
        for(let i = 0; i < v.c.length; i++) e.appendChild(window.$_urn(v.c[i]));
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
    if(v == null || v === false || v === true) {
        return dom;
    }
    if(Array.isArray(v)) {
        let cur = dom;
        for(let i = 0; i < v.length; i++) {
            cur = window.$__uni_hydrate_node(parent, cur, v[i]);
        }
        return cur;
    }
    if(window.$__uni_is_state(v)) {
        if(dom && dom.nodeType === 3) {
            dom.textContent = v.value == null ? "" : "" + v.value;
            v.subscribe((next) => { dom.textContent = next == null ? "" : "" + next; });
            return dom.nextSibling;
        }
        const n = document.createTextNode(v.value == null ? "" : "" + v.value);
        if(parent) {
            if(dom) parent.insertBefore(n, dom);
            else parent.appendChild(n);
        }
        v.subscribe((next) => { n.textContent = next == null ? "" : "" + next; });
        return dom;
    }
    if(typeof v === "string" || typeof v === "number") {
        if(dom && dom.nodeType === 3) {
            dom.textContent = "" + v;
            return dom.nextSibling;
        }
        const n = document.createTextNode("" + v);
        if(parent) {
            if(dom) parent.insertBefore(n, dom);
            else parent.appendChild(n);
        }
        return dom;
    }
    if(v && v.__uni_html !== undefined) {
        // Children already exist in SSR HTML. Leave them in place and let
        // nested dispatches hydrate any component boundaries inside.
        return null;
    }
    if(v && v.t !== undefined) {
        if(v.t === window.$_ur.Fragment) {
            return window.$__uni_hydrate_node(parent, dom, v.c || []);
        }
        if(typeof v.t === "function") {
            const nextProps = v.p ? { ...v.p } : {};
            if(v.c && v.c.length) nextProps.children = v.c.length === 1 ? v.c[0] : v.c;
            return window.$__uni_hydrate_node(parent, dom, v.t(nextProps));
        }
        if(!dom || dom.nodeType !== 1) return dom;
        const e = dom;
        const props = v.p || {};
        for(const k in props) window.$__uni_apply_prop(e, k, props[k]);
        if(v.c && v.c.length) {
            window.$__uni_hydrate_children(e, v.c);
        }
        return e.nextSibling;
    }
    return dom;
})
window.$__uni_mount = ((host, comp, props) => {
    if(!host || !comp) return;
    const out = comp(props || {});
    window.$__uni_hydrate_children(host, [ out ]);
})
window.$_uc = ((factory, props) => {
    if(!factory) return null;
    return window.$_urn(factory(props || {}));
})
window.$__universal_flush = function() {
    const q = window.$__uni_hydration_queue;
    for(let i = 0; i < q.length; i++) {
        const obj = window.$__uni_hydration_queue[i];
        const fn = window[obj[0]];
        if(fn) {
            window.$__uni_mount(obj[1], fn, obj[2])
        } else {
            console.error("missing component function by name", obj[0]);
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
