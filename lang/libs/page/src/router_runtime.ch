// Universal router client runtime — the normative source from
// lang/docs/universal-router-design.md §15.2.
//
// Emitted once per page that declares a router (via HtmlPage.ensure_router_runtime),
// never for pages that do not (INV-18). Keep this block pure ASCII: a byte >= 0x80
// inside an emitted JS blob crashes std::string::find (see the universal skill).
public func router_runtime_js() : std::string_view {
    return std::string_view("""
window.$__uni_router_version = 1;

window.$__uni_router_error = ((message, details = "") => {
    console.error("[router] " + message + (details ? ": " + details : ""));
});

window.$__uni_routers = window.$__uni_routers || Object.create(null);
window.$__uni_router_queue = window.$__uni_router_queue || [];
window.$__uni_router_busy = window.$__uni_router_busy || false;

window.$__uni_route_visible = ((route, visible) => {
    route.visible = visible;
    if(!route.el) {
        window.$__uni_router_error("route wrapper element missing", route.key);
        return;
    }
    if(!visible) {
        route.scrollY = window.scrollY;
        const a = document.activeElement;
        if(a && a !== document.body && route.el.contains(a)) route.focusEl = a;
    }
    route.el.setAttribute("data-uni-route-active", visible ? "true" : "false");
    if(visible) {
        if(!route.noscroll) window.scrollTo(0, route.scrollY || 0);
        const f = (route.focusEl && route.el.contains(route.focusEl))
            ? route.focusEl : route.el;
        try { f.focus({ preventScroll: true }); } catch(_) {}
    }
});

window.$__uni_decode_segment = ((s) => {
    try { return decodeURIComponent(s); } catch(_) { return s; }
});

window.$__uni_parse_query = ((s) => {
    const out = Object.create(null);
    const i = s ? s.indexOf("?") : -1;
    if(i < 0) return out;
    const parts = s.slice(i + 1).split("&");
    for(let k = 0; k < parts.length; k++) {
        const p = parts[k];
        if(!p) continue;
        const eq = p.indexOf("=");
        out[window.$__uni_decode_segment(eq < 0 ? p : p.slice(0, eq))] =
            eq < 0 ? "" : window.$__uni_decode_segment(p.slice(eq + 1));
    }
    return out;
});

window.$__uni_query_sig = ((q) => {
    if(!q) return "";
    let s = "";
    for(const k in q) s += k + "=" + q[k] + "&";
    return s;
});

window.$__uni_initial_url = (() => {
    let p = "/";
    try { p = location.pathname + location.search; } catch(_) {}
    if(!p || p.charCodeAt(0) !== 47) p = "/";
    return p;
});

window.$__uni_base_title = (window.$__uni_base_title === undefined)
    ? document.title : window.$__uni_base_title;

window.$__uni_route_register = ((routerName, routeId, spec) => {
    const r = window.$__uni_routers[routerName];
    if(!r) { window.$__uni_router_error("route registered for unknown router", routerName); return; }
    const el = spec.wrapperId ? document.getElementById(spec.wrapperId) : null;
    const host = window.$__uni_boundary(spec.hostId);
    if(el && host && el === host)
        window.$__uni_router_error("route boundary resolves to the wrapper", spec.key);
    r.routes[routeId] = {
        id: routeId, key: spec.key, el: el, host: host,
        comp: spec.comp, baseProps: spec.baseProps, params: null,
        hydrated: false, visible: false, failed: false,
        ssr: !spec.remote, remote: !!spec.remote, isUrl: !!spec.isUrl,
        url: null, rawUrl: null, inst: null, inFlight: null,
        fragment: null, pendingActivate: null, fetchUrl: spec.fetchUrl || null,
        scrollY: 0, focusEl: null, noscroll: !!spec.noscroll, title: spec.title || null,
        nested: spec.nested || null, nestedDefault: spec.nestedDefault || null,
        beforeActivate: spec.beforeActivate || null,
        onActivate: spec.onActivate || null,
        onDeactivate: spec.onDeactivate || null
    };
});

window.$__uni_route_props = ((route) => {
    if(!route.params) return route.baseProps;
    const out = Object.create(null);
    for(const k in route.baseProps) out[k] = route.baseProps[k];
    for(const k in route.params) out[k] = route.params[k];
    return out;
});

window.$__uni_HISTORY_NONE    = 0;
window.$__uni_HISTORY_PUSH    = 1;
window.$__uni_HISTORY_REPLACE = 2;

window.$__uni_activate = ((routerName, routeId, url, params, historyMode, rawUrl, chain) => {
    if(window.$__uni_router_busy) {
        window.$__uni_router_queue.push([routerName, routeId, url, params, historyMode, rawUrl, chain]);
        return true;
    }
    window.$__uni_router_busy = true;
    const ok = window.$__uni_activate_now(routerName, routeId, url, params, historyMode, rawUrl, chain);
    window.$__uni_router_busy = false;
    while(window.$__uni_router_queue.length) {
        const q = window.$__uni_router_queue.shift();
        window.$__uni_router_busy = true;
        window.$__uni_activate_now(q[0], q[1], q[2], q[3], q[4], q[5], q[6]);
        window.$__uni_router_busy = false;
    }
    return ok;
});

window.$__uni_activate_now = ((routerName, routeId, url, params, historyMode, raw, chain) => {
    const r = window.$__uni_routers[routerName];
    const route = r && r.routes[routeId];
    if(!route) {
        window.$__uni_router_error("activateRoute: unknown route", routerName + "#" + routeId);
        return false;
    }
    if(route.failed) {
        window.$__uni_router_error("activateRoute: route failed earlier", route.key);
        return false;
    }
    url = (url === undefined) ? route.url : url;
    raw = (raw === undefined)
        ? ((url === route.url) ? route.rawUrl : url)
        : raw;
    if(r.currentRoute === route && route.url === url && route.rawUrl === raw) return true;
    // Param-change remount (D-5.4). A layout that owns nested routes must NOT be
    // disposed: its DOM contains the nested wrappers, so remounting it would
    // destroy them. The nested activation chain re-derives the children from the
    // shared URL instead (§13.3.3).
    if(route.url !== url && (route.hydrated || route.inst) && !route.nested) {
        if(route.inst) window.$__uni_dispose(route.inst);
        route.inst = null;
        route.hydrated = false;
        route.ssr = false;
        route.fragment = null;
    }
    if(route.beforeActivate && route.beforeActivate(url) === false) {
        window.$__uni_router_error("navigation cancelled", route.key);
        return false;
    }
    if(route.remote) {
        if(!route.fragment) {
            route.pendingActivate = [url, params, historyMode, raw];
            if(!route.inFlight) window.$__uni_fetch_route(routerName, routeId, false);
            return true;
        }
        if(!route.host || !route.ssr || !route.host.firstChild) {
            if(!window.$__uni_mount_fragment(routerName, routeId)) return false;
        }
    }
    route.url = url;
    route.rawUrl = raw;
    route.params = params || null;
    if(!route.hydrated) {
        if(route.comp) {
            if(!route.host) {
                route.failed = true;
                window.$__uni_router_error("route has no mount host", route.key);
                return false;
            }
            if(!route.ssr) {
                while(route.host.firstChild) route.host.removeChild(route.host.firstChild);
            }
            try {
                window.$__uni_mount(route.host, route.comp,
                                    window.$__uni_route_props(route), "children");
                route.inst = route.host.$__uni_instance;
            } catch(err) {
                route.hydrated = true;
                window.$__uni_router_error("route mount failed", route.key + " " + err);
                return false;
            }
        }
        route.hydrated = true;
    }
    if(r.currentRoute) {
        if(r.currentRoute.onDeactivate) r.currentRoute.onDeactivate();
        window.$__uni_route_visible(r.currentRoute, false);
    }
    window.$__uni_route_visible(route, true);
    r.currentRoute = route;
    if(r.$current.value !== routeId) r.$current.value = routeId;
    if(r.$url && r.$url.value !== url) r.$url.value = url;
    if(route.isUrl && historyMode !== window.$__uni_HISTORY_NONE) {
        window.$__uni_set_url(raw, historyMode === window.$__uni_HISTORY_REPLACE);
    }
    if(r.$query && route.isUrl) {
        const q = window.$__uni_parse_query(raw);
        if(window.$__uni_query_sig(r.$query.value) !== window.$__uni_query_sig(q)) {
            r.$query.value = q;
        }
    }
    try { document.title = route.title || window.$__uni_base_title; } catch(_) {}
    // Nested routes (Phase 6): activating an outer route also activates its
    // nested router. When the URL selected a nested child (§6.4), the match
    // carries an activation chain: follow it (passing the remaining steps down)
    // instead of the declared nested default. The outer route's resolved params
    // are inherited, so a nested child's `props.id` sees the URL param.
    if(route.nested) {
        let childId = route.nestedDefault;
        let childChain = null;
        if(chain && chain.length && chain[0] && chain[0][0] === route.nested) {
            childId = chain[0][1];
            childChain = chain.slice(1);
        }
        if(childId) {
            window.$__uni_activate(route.nested, childId, url, route.params,
                                   window.$__uni_HISTORY_NONE, raw, childChain);
        }
    }
    if(route.onActivate) {
        try { route.onActivate(url); }
        catch(err) { window.$__uni_router_error("onActivate failed", route.key + " " + err); }
    }
    return true;
});

window.$__uni_preload = ((routerName, routeId) => {
    const r = window.$__uni_routers[routerName];
    const route = r && r.routes[routeId];
    if(!route) { window.$__uni_router_error("preload: unknown route", routerName + "#" + routeId); return false; }
    if(route.failed) return false;
    if(route.hydrated || route.inFlight) return true;
    if(route.remote) {
        if(!route.fragment) return window.$__uni_fetch_route(routerName, routeId, true);
        if(!route.host || !route.ssr || !route.host.firstChild) {
            if(!window.$__uni_mount_fragment(routerName, routeId)) return false;
        }
    }
    if(route.comp) {
        if(!route.host) {
            route.failed = true;
            window.$__uni_router_error("preload: route has no mount host", route.key);
            return false;
        }
        if(!route.ssr) { while(route.host.firstChild) route.host.removeChild(route.host.firstChild); }
        try {
            window.$__uni_mount(route.host, route.comp,
                                window.$__uni_route_props(route), "children");
            route.inst = route.host.$__uni_instance;
        } catch(err) {
            route.hydrated = true;
            window.$__uni_router_error("preload mount failed", route.key + " " + err);
            return false;
        }
    }
    route.hydrated = true;
    return true;
});

window.$__uni_release = ((routerName, routeId) => {
    const r = window.$__uni_routers[routerName];
    const route = r && r.routes[routeId];
    if(!route) { window.$__uni_router_error("release: unknown route", routerName + "#" + routeId); return false; }
    if(r.currentRoute === route) {
        window.$__uni_router_error("release: cannot release the active route", route.key);
        return false;
    }
    if(route.inFlight) {
        window.$__uni_router_error("release: route is loading", route.key);
        return false;
    }
    if(route.inst) window.$__uni_dispose(route.inst);
    route.inst = null;
    route.hydrated = false;
    route.failed = false;
    route.params = null;
    route.ssr = false;
    if(route.host) { while(route.host.firstChild) route.host.removeChild(route.host.firstChild); }
    return true;
});

window.$__uni_should_intercept = ((e, href) => {
    if(!e || e.defaultPrevented) return false;
    if(e.button !== 0 || e.metaKey || e.ctrlKey || e.shiftKey || e.altKey) return false;
    if(!href || href.charCodeAt(0) !== 47) return false;
    if(href.charCodeAt(1) === 47) return false;
    const t = e.currentTarget;
    if(t && (t.target || t.hasAttribute("download"))) return false;
    return true;
});

window.$__uni_router = ((name) => {
    const r = window.$__uni_routers[name];
    if(!r) {
        window.$__uni_router_error("router not present on this page", name);
        return window.$__uni_router_null(name);
    }
    return r;
});

window.$__uni_norm_path = ((p) => {
    if(!p) return "/";
    const q = p.indexOf("?"); if(q >= 0) p = p.slice(0, q);
    if(p.length > 1 && p.charCodeAt(p.length - 1) === 47) p = p.slice(0, -1);
    return p || "/";
});

window.$__uni_router_null = ((name) => ({
    name: name, currentRoute: null,
    $current: window.$_us(null), $url: window.$_us(null), $query: window.$_us(null),
    routes: Object.create(null), table: null,
    activateRoute: (() => false), activateRouteByUrl: (() => false),
    replaceRoute: (() => false), replaceRouteByUrl: (() => false),
    deactivate: (() => {}), preload: (() => false),
    release: (() => false), current: (() => null), currentUrl: (() => null),
    isActive: (() => false), buildPath: (() => null), normPath: window.$__uni_norm_path,
    query: (() => null), setQuery: (() => {})
}));

window.$__uni_router_methods = ((name) => ({
    activateRoute:     ((id, params) => window.$__uni_activate(name, id, undefined, params, window.$__uni_HISTORY_PUSH)),
    activateRouteByUrl:((path) => window.$__uni_activate_by_url(name, path, false)),
    replaceRoute:      ((id, params) => window.$__uni_activate(name, id, undefined, params, window.$__uni_HISTORY_REPLACE)),
    replaceRouteByUrl: ((path) => window.$__uni_activate_by_url(name, path, true)),
    deactivate:        (() => {
        const r = window.$__uni_routers[name];
        if(!r.currentRoute) return;
        if(r.currentRoute.onDeactivate) r.currentRoute.onDeactivate();
        window.$__uni_route_visible(r.currentRoute, false);
        r.currentRoute = null;
        if(r.$current.value !== null) r.$current.value = null;
        if(r.$url.value !== null) r.$url.value = null;
        if(r.$query.value !== null) r.$query.value = null;
        try { document.title = window.$__uni_base_title; } catch(_) {}
    }),
    preload:  ((id) => window.$__uni_preload(name, id)),
    release:  ((id) => window.$__uni_release(name, id)),
    current:  (() => { const r = window.$__uni_routers[name]; return r.currentRoute ? r.currentRoute.id : null; }),
    currentUrl:(() => { const r = window.$__uni_routers[name]; return r.currentRoute ? r.currentRoute.url : null; }),
    isActive: ((id) => window.$__uni_routers[name].$current.value === id),
    buildPath:((id, params) => window.$__uni_build_path(name, id, params)),
    normPath: ((p) => window.$__uni_norm_path(p)),
    query:    (() => window.$__uni_routers[name].$query.value),
    setQuery: ((obj) => window.$__uni_set_query(name, obj))
}));

window.$__uni_router_fragment_url = window.$__uni_router_fragment_url ||
    ((name, id) => "/__uni_fragment?router=" + encodeURIComponent(name) +
                   "&id=" + encodeURIComponent(id));

window.$__uni_set_table_base = ((name, base) => {
    const r = window.$__uni_routers[name];
    if(r && r.table) r.table.base = base || "";
});

window.$__uni_url_mem = window.$__uni_url_mem || { path: "/" };
if(window.$__uni_history_ok === undefined) {
    try { history.replaceState(history.state, "", location.href); window.$__uni_history_ok = true; }
    catch(_) { window.$__uni_history_ok = false; }
}

window.$__uni_set_url = ((path, replace) => {
    window.$__uni_url_mem.path = path;
    if(!window.$__uni_history_ok) return;
    try { replace ? history.replaceState(null, "", path) : history.pushState(null, "", path); }
    catch(_) { window.$__uni_history_ok = false; }
});

window.$__uni_match_url = ((name, path) => {
    const r = window.$__uni_routers[name];
    const t = r && r.table;
    if(!t) return null;
    let p = window.$__uni_norm_path(path);
    if(t.base && p.indexOf(t.base) === 0) p = p.slice(t.base.length) || "/";
    const segs = p.split("/");
    let n = 0;
    for(let i = 0; i < segs.length; i++) if(segs[i]) segs[n++] = segs[i];
    segs.length = n;
    for(let i = 0; i < t.routes.length; i++) {
        const e = t.routes[i];
        if(e.fallback) return { id: e.id, fallback: true, params: null, chain: null };
        if(e.pattern.length !== segs.length) continue;
        let params = null, ok = true;
        for(let j = 0; j < e.pattern.length; j++) {
            const s = e.pattern[j];
            if(s.charCodeAt(0) === 123) {
                if(!params) params = Object.create(null);
                params[s.slice(1, -1)] = window.$__uni_decode_segment(segs[j]);
            } else if(s !== segs[j]) { ok = false; break; }
        }
        if(ok) return { id: e.id, fallback: false, params: params, chain: e.chain || null };
    }
    return null;
});

window.$__uni_activate_by_url = ((name, path, replace) => {
    const m = window.$__uni_match_url(name, path);
    if(!m) {
        window.$__uni_router_error("no route matches", name + " " + path);
        return false;
    }
    return window.$__uni_activate(name, m.id, window.$__uni_norm_path(path), m.params,
        replace ? window.$__uni_HISTORY_REPLACE : window.$__uni_HISTORY_PUSH, path, m.chain);
});

window.$__uni_fetch_route = ((name, routeId, prefetchOnly) => {
    const r = window.$__uni_routers[name];
    const route = r && r.routes[routeId];
    if(!route) return false;
    if(route.failed) return false;
    if(route.hydrated || route.inFlight) return true;
    const url = route.fetchUrl || window.$__uni_router_fragment_url(name, routeId);
    route.inFlight = 1;
    fetch(url, { credentials: "same-origin" }).then((res) => {
        if(!res.ok) throw new Error("HTTP " + res.status);
        return res.text();
    }).then((html) => {
        route.inFlight = null;
        route.fragment = html;
        const pend = route.pendingActivate;
        if(!window.$__uni_mount_fragment(name, routeId)) {
            route.pendingActivate = null;
            return;
        }
        route.pendingActivate = null;
        if(pend) window.$__uni_activate(name, routeId, pend[0], pend[1], pend[2], pend[3]);
    }).catch((err) => {
        route.inFlight = null;
        route.pendingActivate = null;
        window.$__uni_router_error("fragment fetch failed", route.key + " " + err);
    });
    return true;
});

window.$__uni_mount_fragment = ((name, routeId) => {
    const route = window.$__uni_routers[name].routes[routeId];
    if(!route.fragment) return false;
    const tmp = document.createElement("div");
    tmp.innerHTML = route.fragment;
    const src = tmp.querySelector('[data-chx-i]');
    if(!src || !route.host) {
        route.failed = true;
        window.$__uni_router_error("fragment has no boundary", route.key);
        return false;
    }
    while(route.host.firstChild) route.host.removeChild(route.host.firstChild);
    while(src.firstChild) route.host.appendChild(src.firstChild);
    route.ssr = true;
    route.hydrated = false;
    return true;
});

window.$__uni_set_query = ((name, obj) => {
    const r = window.$__uni_routers[name];
    if(!r) { window.$__uni_router_error("setQuery: unknown router", name); return; }
    const parts = [];
    for(const k in obj) parts.push(encodeURIComponent(k) + "=" + encodeURIComponent(obj[k]));
    const q = parts.length ? "?" + parts.join("&") : "";
    const base = (r.currentRoute && r.currentRoute.rawUrl) || window.$__uni_url_mem.path;
    const full = window.$__uni_norm_path(base) + q;
    if(r.currentRoute) { r.currentRoute.rawUrl = full; r.currentRoute.url = window.$__uni_norm_path(base); }
    if(window.$__uni_query_sig(r.$query.value) !== window.$__uni_query_sig(obj)) r.$query.value = obj;
    window.$__uni_set_url(full, true);
});

window.$__uni_activate_initial = ((name, id, initialUrl) => {
    const r = window.$__uni_routers[name];
    const route = r && r.routes[id];
    if(!route) {
        window.$__uni_router_error("activateInitial: unknown route", name + "#" + id);
        return false;
    }
    const hasTable = !!(r.table && r.table.routes && r.table.routes.length);
    if(!hasTable) {
        return window.$__uni_activate(name, id, undefined, null, window.$__uni_HISTORY_NONE, undefined, null);
    }
    const m = window.$__uni_match_url(name, initialUrl);
    const matched = !!(m && m.id === id && !m.fallback);
    // An id default can sit in front of a URL table (a URL child nested under an
    // id layout); when the URL does not select it, activate it by id.
    if(!matched && !route.isUrl) {
        return window.$__uni_activate(name, id, undefined, null, window.$__uni_HISTORY_NONE, undefined, null);
    }
    return window.$__uni_activate(name, id,
        matched ? window.$__uni_norm_path(initialUrl) : undefined,
        (matched && m.params) ? m.params : null,
        window.$__uni_HISTORY_NONE,
        matched ? initialUrl : undefined,
        (matched && m.chain) ? m.chain : null);
});

window.$__uni_build_path = ((name, id, params) => {
    const r = window.$__uni_routers[name];
    const t = r && r.table;
    if(!t) return null;
    // A top-level entry's `id` is the root route; a nested URL entry's chain
    // ends with the nested route id, so both forms resolve here.
    const isTarget = (e) => {
        if(e.fallback) return false;
        if(!e.chain && e.id === id) return true;
        return !!(e.chain && e.chain.length && e.chain[e.chain.length - 1][1] === id);
    };
    for(let i = 0; i < t.routes.length; i++) {
        const e = t.routes[i];
        if(!isTarget(e)) continue;
        let out = "";
        for(let j = 0; j < e.pattern.length; j++) {
            const s = e.pattern[j];
            out += "/" + (s.charCodeAt(0) === 123
                ? encodeURIComponent((params && params[s.slice(1, -1)]) || "")
                : s);
        }
        return (t.base || "") + (out || "/");
    }
    window.$__uni_router_error("buildPath: unknown route", name + "#" + id);
    return null;
});

window.$__uni_sync_url = ((name) => {
    const rr = window.$__uni_routers[name];
    if(!rr || rr.$popstate) return;
    rr.$popstate = 1;
    window.addEventListener("popstate", () => {
        const raw = window.$__uni_history_ok
            ? (location.pathname + location.search)
            : window.$__uni_url_mem.path;
        const m = window.$__uni_match_url(name, raw);
        if(!m) {
            window.$__uni_router_error("no route matches on popstate", name + " " + raw);
            const cur0 = window.$__uni_routers[name].currentRoute;
            if(cur0) window.$__uni_set_url(cur0.rawUrl || cur0.url, true);
            return;
        }
        if(!window.$__uni_activate(name, m.id, window.$__uni_norm_path(raw), m.params,
                                   window.$__uni_HISTORY_NONE, raw, m.chain)) {
            const cur = window.$__uni_routers[name].currentRoute;
            if(cur) window.$__uni_set_url(cur.rawUrl || cur.url, true);
        }
    });
    if(window.$__uni_history_ok) history.scrollRestoration = "manual";
});
""")
}
