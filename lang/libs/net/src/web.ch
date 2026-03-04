// ===== Router + middleware =====
public namespace web {
    public type Handler = std.function<(req: http.Request, res: http.ResponseWriter) => void>
    public type Middleware = std.function<(next: Handler) => Handler>

    public struct Route {
        var method: std::string;
        var pattern: std::string; // e.g. /users/:id
        var handler: Handler;
    }

    public struct Router {
        var routes: std::vector<Route>;
        var middlewares: std::vector<Middleware>;

        @constructor func constructor() {
            return Router {
                routes = std::vector<Route>(),
                middlewares = std::vector<Middleware>()
            }
        }

        func add(&mut self, method: *char, pattern: *char, h: Handler) {
            routes.push_back(Route{ method: std::string::make_no_len(method), pattern: std::string::make_no_len(pattern), handler: h });
        }

        func use_middleware(&mut self, m: Middleware) { middlewares.push_back(m) }

        func match_route(&self, method: &std::string, path: &std::string, params_out: *mut std::vector<std::pair<std::string,std::string>>) : *Route {
            var i = 0u;
            while(i < routes.size()) {
                var r = routes.get_ptr(i);
                if(r.method.equals_with_len(method.data(), method.size())) {
                    // match pattern vs path
                    if(match_pattern(r.pattern, path, params_out)) {
                        return r
                    } else {
                        // printf("route path doesn't match, want %s, has pattern %s\n", path.data(), r.pattern.data());
                    }
                } else {
                    // printf("route method doesn't match, want %s, has %s\n", method.data(), r.method.data());
                }
                i = i + 1u;
            }
            // printf("no route matched, return\n");
            return null
        }

        // apply middlewares to a handler and return final handler
        func apply_middlewares(&self, base: Handler) : Handler {
            var h = base;
            var i = (middlewares.size());
            while(i > 0u) {
                i = i - 1u;
                var m = middlewares.get_ref(i);
                h = m(h);
            }
            return h;
        }

    }

    // match pattern like /users/:id against path and fill params
    public func match_pattern(pattern: &std::string, path: &std::string, params_out: *mut std::vector<std::pair<std::string,std::string>>) : bool {
        // split into segments by '/'
        var pseg = split_segments(pattern);
        var tseg = split_segments(path);
        if(pseg.size() != tseg.size()) { return false }
        var i = 0u;
        while(i < pseg.size()) {
            var ps = pseg.get_ptr(i);
            var ts = tseg.get_ptr(i);
            if(ps.size() > 0 && ps.get(0) == ':' ) {
                // param
                var key = ps.substring(1, ps.size());
                params_out.push_back(std::pair<std::string,std::string>{ first : key, second : ts.copy() });
            } else {
                if(!ps.equals_with_len(ts.data(), ts.size())) { return false }
            }
            i = i + 1u;
        }
        return true;
    }

    public func split_segments(s: &std::string) : std::vector<std::string> {
        var out = std::vector<std::string>();
        var i = 0u; var start = 0u;
        // skip leading '/'
        if(i < s.size() && s.get(i) == '/') { i++; start = i }
        while(i <= s.size()) {
            if(i == s.size() || s.get(i) == '/') {
                out.push_back(s.substring(start, i));
                start = i + 1u;
            }
            i = i + 1u;
        }
        return out;
    }
}