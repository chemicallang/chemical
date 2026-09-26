/**
 * `<RouterLink href="..." router="..." preload>` — an internal navigation link
 * (design §6.3, §6.6).
 *
 *   <RouterLink href="/projects" router="main-router">Projects</RouterLink>
 *
 * Only plain left-clicks on same-origin paths are intercepted; modified clicks,
 * `target`, `download` and cross-origin/mailto/tel/fragment hrefs fall through to
 * the browser (`$__uni_should_intercept`, emitted with the router runtime).
 *
 * Named `RouterLink` (not `Link`) because the `components` library already ships
 * a typography `Link`; two components named `Link` would collide for any app
 * importing both.
 *
 * Active state reads the router signals (§5.4): a URL route subscribes to `$url`
 * (param-aware), an id route to `$current`. Reading the signal — not
 * `r.currentRoute` — is what makes the link re-render on navigation.
 */
public #universal RouterLink(props) {
    const r = router(props.router || "main-router")
    const cur = r.$url.value
    const target = r.normPath(props.href)
    const active = props.routeId
        ? r.$current.value == props.routeId
        : window.$__uni_link_active(cur, target, props.end)
    return <a {...props}
        router={null}
        routeId={null}
        preload={null}
        end={null}
        href={props.href}
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
