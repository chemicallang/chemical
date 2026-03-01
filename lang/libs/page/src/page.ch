
public struct HtmlPage {

    var pageHead : std::string

    var pageHtml : std::string

    var pageCss : std::string

    var pageJs : std::string

    var pageHeadJs : std::string

    // we track which classes are done through this unordered map
    // TODO using ubigint, instead need to use size_t
    var doneClasses : std::unordered_map<ubigint, bool>

    // track random CSS classes (for dynamic values) to prevent duplicates
    var doneRandomClasses : std::unordered_map<ubigint, bool>

    var doneComponents : std::unordered_map<ubigint, bool>

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

    func toString(&self) : std::string {
        var str = std::string()
        str.reserve(pageHead.size() + pageCss.size() + pageHtml.size() + pageHeadJs.size() + pageJs.size() + 100)
        str.append_view(std::string_view("<!DOCTYPE html><html><head>"))
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
        str.append_view(std::string_view("</head><body>"))
        str.append_string(pageHtml)
        if(!pageJs.empty()) {
            str.append_view(std::string_view("<script>"))
            str.append_string(pageJs)
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

    func defaultPreactSetup(&mut self) {
        pageHead.append_view(std::string_view("""<script src="https://unpkg.com/preact/dist/preact.min.js"></script><script src="https://unpkg.com/preact/hooks/dist/hooks.umd.js"></script><script>window.$_p = preact;window.$_ph = preactHooks; window.$_pm = (e,c,p) => {const P = document.createDocumentFragment(); $_p.render($_p.h(c, p || {}), P); e.replaceWith(P); }; window.$_pu = window.$_pu || ((compRef, props, ...children) => { const pp = props ? { ...props } : {}; if(children && children.length) pp.children = children.length === 1 ? children[0] : children; const U = (p2) => { const ref = $_ph.useRef(null); $_ph.useLayoutEffect(() => { const host = ref.current; if(!host) return; let stop = false; let h = 0; const resolve = () => { if(typeof compRef === 'string') { if(window.$_u && window.$_u[compRef]) return window.$_u[compRef]; if(window[compRef]) return window[compRef]; return null; } return compRef; }; const mount = () => { if(stop) return; const comp = resolve(); if(!comp) { h = window.requestAnimationFrame ? window.requestAnimationFrame(mount) : setTimeout(mount, 16); return; } const node = window.$_uc ? window.$_uc(comp, p2 || {}) : null; host.innerHTML = ''; if(node) host.appendChild(node); }; mount(); return () => { stop = true; if(window.cancelAnimationFrame && window.requestAnimationFrame && h) window.cancelAnimationFrame(h); else if(h) clearTimeout(h); }; }, [p2]); return $_p.h('span', { ref }); }; return $_p.h(U, pp); });</script>"""))
    }

    func defaultDevelopmentReactSetup(&mut self) {
        pageHead.append_view(std::string_view("""<script src="https://unpkg.com/react@18/umd/react.development.js"></script><script src="https://unpkg.com/react-dom@18/umd/react-dom.development.js"></script><script>window.$_r = React; window.$_rd = ReactDOM; window.$_rm = (e, c, p) => { const P = document.createElement("div"); e.replaceWith(P); $_rd.createRoot(P).render($_r.createElement(c, p || {})); }</script>"""))
    }

    func defaultReactSetup(&mut self) {
        pageHead.append_view(std::string_view("""<script src="https://unpkg.com/react@18/umd/react.production.min.js"></script><script src="https://unpkg.com/react-dom@18/umd/react-dom.production.min.js"></script><script>window.$_r = React; window.$_rd = ReactDOM; window.$_rm = (e, c, p) => { const P = document.createElement("div"); e.replaceWith(P); $_rd.createRoot(P).render($_r.createElement(c, p || {})); }</script>"""))
    }

    func defaultUniversalSetup(&mut self) {
        pageHead.append_view(std::string_view("""<script>
            window.$_ut = window.$_ut || ((e, x) => {
                for(let i = 0; i < x.length; i++) e = e.children[x[i]];
                return e;
            });
            window.$_u = window.$_u || {};
            window.$_uq = window.$_uq || [];
            window.$_ureg = window.$_ureg || ((name, comp) => {
                window.$_u[name] = comp;
                if(window.$_uf) window.$_uf();
            });
            window.$_ucn = window.$_ucn || ((name, props) => {
                const comp = window.$_u[name];
                if(!comp) return null;
                return comp(props || {});
            });
            window.$_uf = window.$_uf || (() => {
                if(!window.$_uq || !window.$_uq.length) return;
                const next = [];
                for(let i = 0; i < window.$_uq.length; i++) {
                    const q = window.$_uq[i];
                    const id = q[0];
                    const name = q[1];
                    const props = q[2] || {};
                    const host = document.getElementById(id);
                    const comp = window.$_u[name];
                    if(!host || !comp) {
                        next.push(q);
                        continue;
                    }
                    const root = host.firstElementChild || host.firstChild;
                    if(root && comp.__hydrate) {
                        comp.__hydrate(root, props);
                    } else if(comp) {
                        const node = window.$_uc ? window.$_uc(comp, props) : comp(props);
                        if(node) {
                            host.innerHTML = "";
                            host.appendChild(node);
                        }
                    }
                }
                window.$_uq = next;
            });
            if(document.readyState === "loading") {
                document.addEventListener("DOMContentLoaded", () => window.$_uf && window.$_uf(), { once: true });
            } else {
                window.$_uf();
            }
            window.$_um = window.$_um || ((e, comp, props) => {
                const data = props || {};
                const out = comp(data);
                if(out == null) {
                    e.remove();
                    return;
                }
                if(out.nodeType) {
                    e.replaceWith(out);
                    return;
                }
                if(out.root && out.root.nodeType) {
                    const root = out.root;
                    e.replaceWith(root);
                    if(out.initialize) out.initialize(root, data);
                    return;
                }
                if(typeof out === "string" || out.html !== undefined) {
                    const tpl = document.createElement("template");
                    tpl.innerHTML = typeof out === "string" ? out : out.html;
                    const root = tpl.content.firstElementChild || tpl.content.firstChild;
                    if(!root) {
                        e.remove();
                        return;
                    }
                    e.replaceWith(root);
                    if(out.initialize) out.initialize(root, data);
                    return;
                }
                if(out && out.t !== undefined && window.$_urn) {
                    e.replaceWith(window.$_urn(out));
                    return;
                }
                e.remove();
            });
            window.$_uc = window.$_uc || ((factory, props) => {
                const out = factory(props || {});
                if(out == null) return null;
                if(out.nodeType) return out;
                if(out.root && out.root.nodeType) return out.root;
                if(typeof out === "string" || out.html !== undefined) {
                    const tpl = document.createElement("template");
                    tpl.innerHTML = typeof out === "string" ? out : out.html;
                    const root = tpl.content.firstElementChild || tpl.content.firstChild;
                    if(!root) return null;
                    if(out.initialize) out.initialize(root, props || {});
                    return root;
                }
                if(out && out.t !== undefined && window.$_urn) return window.$_urn(out);
                return null;
            });
            window.$_pu = window.$_pu || ((compRef, props, ...children) => {
                const p = props ? { ...props } : {};
                if(children && children.length) p.children = children.length === 1 ? children[0] : children;
                const U = (pp) => {
                    const ref = window.$_ph && window.$_ph.useRef ? window.$_ph.useRef(null) : { current: null };
                    const effect = window.$_ph && window.$_ph.useLayoutEffect ? window.$_ph.useLayoutEffect : ((fn) => fn());
                    effect(() => {
                        const host = ref.current;
                        if(!host) return;
                        let stop = false;
                        let h = 0;
                        const resolve = () => {
                            if(typeof compRef === "string") {
                                if(window.$_u && window.$_u[compRef]) return window.$_u[compRef];
                                if(window[compRef]) return window[compRef];
                                return null;
                            }
                            return compRef;
                        };
                        const mount = () => {
                            if(stop) return;
                            const comp = resolve();
                            if(!comp) {
                                h = window.requestAnimationFrame ? window.requestAnimationFrame(mount) : setTimeout(mount, 16);
                                return;
                            }
                            const node = window.$_uc ? window.$_uc(comp, pp || {}) : null;
                            host.innerHTML = "";
                            if(node) host.appendChild(node);
                        };
                        mount();
                        return () => {
                            stop = true;
                            if(window.cancelAnimationFrame && window.requestAnimationFrame && h) window.cancelAnimationFrame(h);
                            else if(h) clearTimeout(h);
                        };
                    }, [p]);
                    return window.$_p ? window.$_p.h("span", { ref }) : null;
                };
                return window.$_p ? window.$_p.h(U, p) : null;
            });
            window.$_su = window.$_su || ((compRef, props, ...children) => {
                const p = props ? { ...props } : {};
                if(children && children.length) p.children = children.length === 1 ? children[0] : children;
                const host = document.createElement("span");
                let stop = false;
                let h = 0;
                const resolve = () => {
                    if(typeof compRef === "string") {
                        if(window.$_u && window.$_u[compRef]) return window.$_u[compRef];
                        if(window[compRef]) return window[compRef];
                        return null;
                    }
                    return compRef;
                };
                const mount = () => {
                    if(stop) return;
                    const comp = resolve();
                    if(!comp) {
                        h = window.requestAnimationFrame ? window.requestAnimationFrame(mount) : setTimeout(mount, 16);
                        return;
                    }
                    const node = window.$_uc ? window.$_uc(comp, p) : null;
                    host.innerHTML = "";
                    if(node) host.appendChild(node);
                };
                mount();
                return host;
            });
        </script>"""))
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
        pageHead.append_view(std::string_view("""<script src="https://unpkg.com/solid-umd@1.9.10/dist/solid.min.js"></script><script src="https://unpkg.com/solid-umd@1.9.10/dist/solid-web.min.js"></script><script src="https://unpkg.com/solid-umd@1.9.10/dist/solid-h.min.js"></script><script>window.$_s = Solid; window.$_sw = SolidWeb; window.$_sh = SolidH.default || SolidH; window.$_sm = (e, comp, props) => { const mount = document.createElement('div'); e.replaceWith(mount); $_sw.render(() => comp(props || {}), mount); }; window.$_su = window.$_su || ((compRef, props, ...children) => { const p = props ? { ...props } : {}; if(children && children.length) p.children = children.length === 1 ? children[0] : children; const host = document.createElement('span'); let stop = false; let h = 0; const resolve = () => { if(typeof compRef === 'string') { if(window.$_u && window.$_u[compRef]) return window.$_u[compRef]; if(window[compRef]) return window[compRef]; return null; } return compRef; }; const mount = () => { if(stop) return; const comp = resolve(); if(!comp) { h = window.requestAnimationFrame ? window.requestAnimationFrame(mount) : setTimeout(mount, 16); return; } const node = window.$_uc ? window.$_uc(comp, p) : null; host.innerHTML = ''; if(node) host.appendChild(node); }; mount(); return host; });</script>"""))
    }

    // given name -> {name}.css, {name}_head.js, {name}.js assets are assumed to exist
    func htmlPageToString(&self, name : &std::string_view) : std::string {
        var str = std::string()
        str.reserve(pageHead.size() + pageHtml.size() + 128)
        str.append_view(std::string_view("<!DOCTYPE html><html><head>"))
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
        str.append_view(std::string_view("</head><body>"))
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
        fs::write_to_file(path.data(), completePage.data())
    }

    // given name -> {name}.css, {name}_head.js, {name}.js assets maybe generated
    func writeToDirectory(&self, path : &std::string_view, name : &std::string_view) {

        // TODO only if not exists
        fs::mkdir(path.data());

        // creating the route file at
        var htmlFile = std::string(path.data(), path.size())
        htmlFile.append('/');
        htmlFile.append_view(name);
        htmlFile.append_char_ptr(".html")

        // writing only html to route
        var htmlPage = htmlPageToString(name)
        fs::write_to_file(htmlFile.data(), htmlPage.data())

        // {name}.css
        if(!pageCss.empty()) {
            const cssFile = std::string(path.data(), path.size())
            cssFile.append('/');
            cssFile.append_view(name)
            cssFile.append_view(".css")
            fs::write_to_file(cssFile.data(), pageCss.data())
        }

        // {name}_head.js
        if(!pageHeadJs.empty()) {
            const jsHeadFile = std::string(path.data(), path.size())
            jsHeadFile.append('/');
            jsHeadFile.append_view(name)
            jsHeadFile.append_view("_head.js")
            fs::write_to_file(jsHeadFile.data(), pageHeadJs.data())
        }

        // {name}.js
        if(!pageJs.empty()) {
            const jsFile = std::string(path.data(), path.size())
            jsFile.append('/');
            jsFile.append_view(name)
            jsFile.append_view(".css")
            fs::write_to_file(jsFile.data(), pageJs.data())
        }

    }

}
