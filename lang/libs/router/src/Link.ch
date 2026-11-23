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
 *
 * Typed props (Phase 2). `href` is required — it drives `normPath`, the rendered
 * `href` and the click predicate. The rest are `@optional`: the body already
 * substitutes `"main-router"` and treats `preload`/`end` as falsy when absent, and
 * the tag nulls those router-only attributes out again after `{...props}`, so a
 * declared default would only move where the default is written.
 *
 * URL active state comes from `$__uni_link_active`, a browser-only runtime
 * helper, so it is applied in an effect (§3.15b): reading `window` during render
 * cannot be reproduced by SSR, and the hydration-safety analysis rejects it. The
 * first client render therefore matches the server (`aria-current` absent), then
 * the effect sets it — the post-mount pattern for client-only values.
 */
public struct RouterLinkProps {
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
    // anchor attributes forwarded by `{...props}`: a link that opens a new tab or
    // downloads is not intercepted by the router (it falls through to the browser),
    // but the attribute still has to be declarable to reach the anchor
    @optional var target : std::string_view
    @optional var rel : std::string_view
    // the string form (`download="name"`); a bare boolean `download` is not a
    // Chemical value and cannot be expressed here
    @optional var download : std::string_view
}

public #universal RouterLink(props : RouterLinkProps) {
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
