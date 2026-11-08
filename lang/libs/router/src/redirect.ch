// Server-side redirects (lang/docs/universal-router-design.md §6.7, §6.7-F).
//
// `redirect_to` / `get_route_redirect` are raw page-stored path hand-offs and
// live in `params.ch` under `router::raw` (Chemical cannot reopen a nested
// namespace across files). They remain extension methods, so
// `page.redirect_to(...)` keeps working.
