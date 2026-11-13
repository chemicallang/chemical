// ===== Router =====
public namespace web {
    // A stored request handler. The witness type proves the capture environment
    // is `Send + Sync + 'static` at registration, so a router can be shared by a
    // worker pool and the handler invoked concurrently. Handlers receive the
    // request and the response writer; the framework owns the socket.
    public type Handler = std.send_sync_function<(req: &mut http.Request, res: &mut http.ResponseWriter) => void>

    public struct Route {
        var method: std::string;
        var pattern: std::string;
        var handler: Handler;
    }

    public struct Router {
        var routes: std::vector<Route>;

        @constructor func constructor() {
            return Router {
                routes = std::vector<Route>()
            }
        }

        func add(&mut self, method: *char, pattern: *char, h: Handler) {
            unsafe {

            routes.push_back(Route{ method: std::string::make_no_len(method), pattern: std::string::make_no_len(pattern), handler: h });

        }
}

        func match_route(&self, method: &std::string, path: &std::string, params_out: *mut std::vector<std::pair<std::string,std::string>>) : *Route {
            unsafe {

            var i = 0u;
            while(i < routes.size()) {
                var r = routes.get_ptr(i);
                if(r.method.equals_with_len(method.data(), method.size())) {
                    if(match_pattern(&r.pattern, path, params_out)) {
                        return r
                    }
                }
                i = i + 1u;
            }
            return null

        }
        }
    }

    public func match_pattern(pattern: &std::string, path: &std::string, params_out: *mut std::vector<std::pair<std::string,std::string>>) : bool {
        unsafe {

        var pseg = split_segments(pattern);
        var tseg = split_segments(path);

        var i = 0u;
        while(i < pseg.size()) {
            var ps = pseg.get_ptr(i);

            if(ps.size() > 0 && ps.at(0) == ':' ) {
                if (ps.at(ps.size() - 1u) == '*') {
                    var key = ps.substring(1, ps.size() - 1u);
                    var val = std::string();
                    var j = i;
                    while (j < tseg.size()) {
                        if (j > i) val.append('/');
                        val.append_string(tseg.at_ref(j));
                        j++;
                    }
                    params_out.push_back(std::pair<std::string,std::string>{ first : key, second : val });
                    return true;
                }

                if (i >= tseg.size()) return false;
                var ts = tseg.get_ptr(i);
                var key = ps.substring(1, ps.size());
                params_out.push_back(std::pair<std::string,std::string>{ first : key, second : ts.copy() });
            } else {
                if (i >= tseg.size()) return false;
                var ts = tseg.get_ptr(i);
                if(!ps.equals_with_len(ts.data(), ts.size())) return false;
            }
            i = i + 1u;
        }

        return i == tseg.size();

    }
}

    public func split_segments(s: &std::string) : std::vector<std::string> {
        var out = std::vector<std::string>();
        var i = 0u; var start = 0u;
        if(i < s.size() && s.at(i) == '/') { i++; start = i }
        while(i <= s.size()) {
            if(i == s.size() || s.at(i) == '/') {
                out.push_back(s.substring(start, i));
                start = i + 1u;
            }
            i = i + 1u;
        }
        return out;
    }
}
