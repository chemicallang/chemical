// A built site as bytes a server can answer from: the deploy-time container.
//
// `page.StaticSite` builds and serves one site in one process. A deployed app is
// usually different: the build runs once (CI, or a build step) and the server
// must answer requests without rendering anything, and without the source tree.
// `Bundle` is that container. Its entries come from either
//
//   * a site that was just built — `bundle.from_site(&site)` — or
//   * generated source: `StaticSite.write_assets_module` writes a Chemical module
//     that carries every artifact's bytes as string constants, which a build
//     compiles into the binary, and the app registers them:
//
//       var bundle = Bundle.make()
//       var i : ubigint = 0
//       while(i < app_assets.count()) {
//           bundle.add_owned(app_assets.path_at(i), app_assets.bytes_at(i),
//                            app_assets.content_type_at(i), app_assets.immutable_at(i))
//           i = i + 1
//       }
//
// Serving is `respond` / `respond_if_none_match`, with exactly the contract
// `StaticSite` has: `/about` and `/about/` resolve to `about/index.html`, a
// content-hashed asset is immutable, every entry carries a strong ETag, a
// matching `If-None-Match` is a 304 with no body, and an unknown path is a 404.
// The Bundle owns every byte it serves, so it is independent of the site (or the
// generated module's buffers) and safe to keep for the process's lifetime.
@direct_init
public struct Bundle {

    private var entries : std::vector<BundleEntry>
    private var failure : std::string

    @make
    public func make() : Bundle {
        return Bundle {
            entries : std::vector<BundleEntry>(),
            failure : std::string()
        }
    }

    /// Register one artifact. `path` is site-relative (`index.html`,
    /// `assets/app.<hash>.js`). The bytes are copied, so the caller's buffer (a
    /// literal in generated code, or a site's artifact) may die immediately.
    ///
    /// Registering the same path twice with the same bytes is one artifact;
    /// different bytes at one path is an error, not an overwrite — two builds that
    /// disagree would otherwise silently serve one of them.
    public func add(&mut self, path : std::string_view, bytes : std::string_view, content_type : std::string_view, immutable : bool) : bool {
        var owned = std::string()
        owned.append_view(&bytes)
        return self.add_owned(path, owned, content_type, immutable)
    }

    /// `add`, taking bytes the caller already owns (no copy).
    public func add_owned(&mut self, path : std::string_view, bytes : std::string, content_type : std::string_view, immutable : bool) : bool {
        if(path.empty()) {
            fail_with("an artifact needs a path")
            return false
        }
        var existing = self.index_of(&path)
        if(existing >= 0) {
            var e = unsafe(&*entries.get_ptr(existing as ubigint))
            var have = e.bytes.to_view()
            var probe = bytes.to_view()
            if(!have.equals(&probe)) {
                fail_with("two different artifacts at the same path")
                fail_with(path)
                return false
            }
            return true
        }

        var path_owned = std::string()
        path_owned.append_view(&path)
        var type_owned = std::string()
        type_owned.append_view(&content_type)
        var etag = quoted_etag(bytes.to_view())
        unsafe {
            entries.push_back(BundleEntry {
                path : path_owned,
                bytes : bytes,
                content_type : type_owned,
                etag : etag,
                immutable : immutable
            })
        }
        return true
    }

    /// Take every artifact from a site that has been built (`build()`/`finish()`).
    /// A site that was not built, or that failed its build, is refused: serving a
    /// half-built site is the failure this container exists to prevent.
    public func from_site(&mut self, site : &StaticSite) : bool {
        var i : ubigint = 0
        while(i < site.artifact_count()) {
            var path = site.artifact_path(i)
            var bytes = site.artifact_bytes(i)
            var ty = site.artifact_content_type(i)
            if(!add(path, bytes, ty, site.artifact_immutable(i))) { return false }
            i = i + 1
        }
        return true
    }

    public func count(&self) : ubigint {
        return entries.size()
    }

    public func path_at(&self, i : ubigint) : std::string_view {
        if(i >= entries.size()) { return std::string_view("") }
        var e = unsafe(&*entries.get_ptr(i))
        return e.path.to_view()
    }

    public func bytes_at(&self, i : ubigint) : std::string_view {
        if(i >= entries.size()) { return std::string_view("") }
        var e = unsafe(&*entries.get_ptr(i))
        return e.bytes.to_view()
    }

    public func content_type_at(&self, i : ubigint) : std::string_view {
        if(i >= entries.size()) { return std::string_view("") }
        var e = unsafe(&*entries.get_ptr(i))
        return e.content_type.to_view()
    }

    public func etag_at(&self, i : ubigint) : std::string_view {
        if(i >= entries.size()) { return std::string_view("") }
        var e = unsafe(&*entries.get_ptr(i))
        return e.etag.to_view()
    }

    public func error_message(&self) : std::string_view {
        return failure.to_view()
    }

    // ── Serving ──────────────────────────────────────────────────────────
    /// Answer `request_path` from the registered bytes (same contract as
    /// `StaticSite.respond`).
    public func respond(&self, request_path : std::string_view) : SiteResponse {
        return respond_if_none_match(request_path, std::string_view(""))
    }

    /// `respond`, honouring a conditional request: a matching `If-None-Match` is a
    /// 304 with no body and the entry's cache policy.
    public func respond_if_none_match(&self, request_path : std::string_view, if_none_match : std::string_view) : SiteResponse {
        var target_s = normalize_request_path(request_path)
        if(target_s.empty()) { return site_not_found() }
        var target = target_s.to_view()
        var index = self.index_of(&target)
        if(index < 0) {
            // `/about` -> `about/index.html`: the directory form of a route.
            var as_dir = std::string()
            as_dir.append_view(&target)
            as_dir.append_view("/index.html")
            var as_dir_view = as_dir.to_view()
            index = self.index_of(&as_dir_view)
        }
        if(index < 0) { return site_not_found() }

        var e = unsafe(&*entries.get_ptr(index as ubigint))
        var etag = e.etag.to_view()
        var cache = std::string_view("no-cache")
        if(e.immutable) {
            cache = std::string_view("public, max-age=31536000, immutable")
        }
        if(page_etag_matches(if_none_match, etag)) {
            return site_response(304, e.content_type.to_view(), cache, std::string_view(""), etag)
        }
        return site_response(200, e.content_type.to_view(), cache, e.bytes.to_view(), etag)
    }

    func index_of(&self, path : &std::string_view) : int {
        var i : ubigint = 0
        while(i < entries.size()) {
            var e = unsafe(&*entries.get_ptr(i))
            if(e.path.equals_view(path)) { return i as int }
            i = i + 1
        }
        return -1
    }

    func fail_with(&mut self, message : std::string_view) {
        if(!failure.empty()) {
            failure.append_view("; ")
        }
        failure.append_view(&message)
    }
}

/// One servable file: its site-relative path, the bytes, the `Content-Type`, a
/// strong `ETag` and whether the URL is content-hashed (cache forever).
@direct_init
public struct BundleEntry {
    public var path : std::string
    public var bytes : std::string
    public var content_type : std::string
    public var etag : std::string
    public var immutable : bool
}
