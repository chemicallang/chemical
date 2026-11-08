// Server URL hand-off (lang/docs/universal-router-design.md §6.1, §6.3.1).
//
// `set_route_url` / `get_route_url` / `get_route_base` are raw page-stored
// views and live in `params.ch` under `router::raw` (Chemical cannot reopen a
// nested namespace across files). Extension functions keep the ergonomic
// `page.set_route_url(url)` syntax.
