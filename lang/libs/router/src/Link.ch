/**
 * `<Link href="...">` — an internal navigation link (design §6.3, §6.6).
 *
 * Only plain left-clicks on same-origin paths are intercepted; modified clicks,
 * `target`, `download` and cross-origin/mailto/tel/fragment hrefs fall through to
 * the browser (`$__uni_should_intercept`, emitted with the router runtime).
 *
 * Active state reads the router signals (§5.4): a URL route subscribes to `$url`
 * (param-aware), an id route to `$current`. Reading the signal — not
 * `r.currentRoute` — is what makes the link re-render on navigation.
 */
public #universal Link(props) {
    const r = router(props.router || "main-router")
    const active = props.routeId
        ? r.$current.value == props.routeId
        : r.$url.value == r.normPath(props.href)
    return <a href={props.href}
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
