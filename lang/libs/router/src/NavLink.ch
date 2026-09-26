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
    const active = props.routeId
        ? r.$current.value == props.routeId
        : r.$url.value == r.normPath(props.href)
    return <a href={props.href}
        class={active ? "chx-navlink is-active" : "chx-navlink"}
        aria-current={active ? "page" : null}
        onMouseEnter={() => { if(props.preload) { r.preload(props.routeId) } }}
        onFocus={() => { if(props.preload) { r.preload(props.routeId) } }}
        onClick={(e) => {
            if(!window.$__uni_should_intercept(e, props.href)) { return }
            e.preventDefault()
            r.activateRouteByUrl(props.href)
        }}>
        {props.children}
    </a>
}
