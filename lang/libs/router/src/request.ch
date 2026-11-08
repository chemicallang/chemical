// Request-context extension (lang/docs/universal-router-design.md §3.3).
//
// The raw `RouteRequest` / `set_request` / `get_request` surface now lives in
// `params.ch` under `router::raw` (Chemical cannot reopen a nested namespace
// across files). The canonical owned surface is `router::safe::RequestInfo`.
