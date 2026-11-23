/**
 * `<NavLink href="..." routeId="...">` — a `Link` with active styling
 * (design D-6.7).
 *
 * `routeId` is only meaningful for id routes (where there is no URL); URL routes
 * derive active state from `$url` against the normalized `href`, so no `routeId`
 * prop is required. `aria-current="page"` mirrors the active class.
 *
 * URL active state is decided by `$__uni_link_active`, which only exists in the
 * browser runtime: the server would have to re-implement segment matching, and
 * a component body that reads `window` is rejected by the hydration-safety
 * analysis (§3.15b) because SSR and hydration would disagree. So it is computed
 * in an effect against the `$url` signal: the first client render matches the
 * server's (`false`), then the effect applies the active state — the same
 * post-mount pattern any client-only value needs.
 */
// Typed props (Phase 2), the same surface as `RouterLink` plus the implicit
// `class`/`className` merge below; see Link.ch for why `href` is required.
public struct NavLinkProps {
    // the destination, normalized against the router's base
    var href : std::string_view
    // the router instance (one per name; `main-router` when absent)
    @optional var router : std::string_view
    // id-route name: active state then comes from `$current`, not the URL
    @optional var routeId : std::string_view
    // preload the destination as soon as it is hovered or focused
    @optional var preload : bool
    // match the exact path (not its prefixes) for active state
    @optional var end : bool
    // anchor attributes forwarded by `{...props}` (see RouterLink)
    @optional var target : std::string_view
    @optional var rel : std::string_view
    @optional var download : std::string_view
}

public #universal NavLink(props : NavLinkProps) {
    const r = router(props.router || "main-router")
    const cur = r.$url.value
    const target = r.normPath(props.href)
    state urlActive = false
    useEffect(() => {
        urlActive = window.$__uni_link_active(cur, target, props.end)
    }, [cur, target, props.end])
    const active = props.routeId
        ? r.$current.value == props.routeId
        : urlActive
    const userClass = props.class || props.className
    const base = userClass ? ("chx-navlink " + userClass) : "chx-navlink"
    return <a {...props}
        router={null}
        routeId={null}
        preload={null}
        end={null}
        href={props.href}
        class={active ? (base + " is-active") : base}
        aria-current={active ? "page" : null}
        onMouseEnter={() => { if(props.preload) { if(props.routeId) { r.preload(props.routeId) } else { r.preloadByUrl(props.href) } } }}
        onFocus={() => { if(props.preload) { if(props.routeId) { r.preload(props.routeId) } else { r.preloadByUrl(props.href) } } }}
        onClick={(e) => {
            if(!window.$__uni_should_intercept(e, props.href)) { return }
            e.preventDefault()
            r.activateRouteByUrl(props.href)
        }}>
        {props.children}
    </a>
}
