/**
 * `<Outlet />` — marks where a nested router's child routes render (Phase 6).
 *
 * The router converter expands this element in place into the nested router's
 * wrappers when it appears inside a route that declares nested `route`
 * declarations. Used outside such a context it renders a harmless empty
 * placeholder.
 */
public #universal Outlet(props) {
    return <div data-uni-outlet="true" hidden></div>
}
