/**
 * `<NavLink href="..." routeId="...">` — a `Link` with active styling
 * (design D-6.7).
 *
 * `routeId` is only meaningful for id routes (where there is no URL); URL routes
 * derive active state from `$url` against the normalized `href`, so no `routeId`
 * prop is required. `aria-current="page"` mirrors the active class.
 */
public #universal NavLink(props) {
    const r = router(props.router || "main-router")
    const cur = r.$url.value
    const target = r.normPath(props.href)
    const active = props.routeId
        ? r.$current.value == props.routeId
        : window.$__uni_link_active(cur, target, props.end)
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
