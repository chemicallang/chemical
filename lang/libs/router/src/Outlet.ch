/**
 * `<Outlet />` — marks where a nested router's child routes render (Phase 6).
 *
 * Inside a route body that declares nested `route` declarations, the router
 * converter expands this element in place into the nested router's wrappers.
 *
 * Inside a *separate layout component's own body* (no route context) it renders
 * a slot (`data-uni-outlet`); the runtime relocates the SSR'd nested wrappers
 * into the nearest slot when the layout activates, so a layout can host the
 * nested content from its own body.
 */
public #universal Outlet(props) {
    return <div data-uni-outlet="true"></div>
}
