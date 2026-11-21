// Build a multi-page site into one artifact set: a shared JS/CSS bundle, one
// HTML page per route, and one script chunk per page.
//
// The hydration runtime, component definitions and global CSS are identical for
// every page. Sent inline, each page repeats them (measured on the components app:
// 78.7% of the second page's bytes were duplicates of the first). `StaticSite`
// routes those bytes into one `SharedAssets` sink, freezes it once, publishes it
// under a content-hashed name, and gives every page a `<script src>` / `<link>`
// that points at it — so N pages carry the runtime once and browsers cache it.
//
// Two ways to deliver the same build:
//
//   *disk* — `finish()` writes every artifact under `out_dir` (static hosting):
//
//     var site = StaticSite.make("out", "app", "assets")
//     var index = HtmlPage()
//     site.add(&mut index, "index.html")   // attaches the sink: render AFTER
//     index.defaultUniversalSetup()
//     #html { ... }
//     if(!site.finish()) { /* site.error_message() */ }
//
//   *memory* — `build()` produces the same artifacts with no filesystem access,
//   and `respond()` answers a request path directly (a server renders once and
//   then serves from RAM):
//
//     if(!site.build()) { ... }
//     var res = site.respond("/about/")     // status, headers, body view
//
// The artifact list is the build's manifest: `artifact_count()` / `artifact_path()`
// / `artifact_bytes()` enumerate exactly what would be written.
//
// Lifetime contract (a build, so it is simple): pages passed to `add` and paths
// passed to `add` must outlive `build()`. The site holds a pointer to each page
// and a view of each path, never a copy — so the pages are read from their own
// buffers, at the moment the shared bundle's hash is known. A `SiteResponse`'s
// body borrows the site's buffers, so it is valid until the site is changed.
@direct_init
public struct StaticSite {

    private var assets : SharedAssetsHandle

    private var pages : std::vector<*mut HtmlPage>
    // Views: a build passes literals (`"index.html"`), which outlive the site.
    private var paths : std::vector<std::string_view>

    private var out_dir : std::string
    // URL prefix the bundle is served under, relative to the page ("assets" ->
    // "assets/app.<hash>.js"). Also the directory it is written into.
    private var asset_prefix : std::string

    // The `<head>` tags for a page at the site root (empty before `build()`).
    private var head_tags : std::string
    // Site-relative URL of the frozen bundle ("" when that half is empty).
    private var bundle_js_path : std::string
    private var bundle_css_path : std::string

    // Everything the build produces, site-relative (`index.html`,
    // `assets/app.<hash>.js`, …). Filled by `build()`, written by `finish()`.
    private var artifacts : std::vector<SiteArtifact>

    private var built : bool
    /// True when `build()` failed. A failed *build* must not be served; a failed
    /// *write* must (the memory build is fine, only the files are missing).
    private var build_failed : bool
    private var written : bool
    private var failed : bool
    private var failure : std::string
    /// Why the last `render_dynamic` refused (empty on success). A per-request
    /// render must not poison the build, so it reports here instead of `failure`.
    private var dynamic_error : std::string

    /// Precompress text artifacts (`set_gzip`). Off by default; a compressed
    /// variant only costs build time and a second buffer in RAM.
    private var want_gzip : bool

    /// Absolute origin this build is published under (`https://example.com`),
    /// used by `export_sitemap`. Empty means "site-relative only".
    private var site_url : std::string

    // ── Unit graph (§3.13) ───────────────────────────────────────────────
    // Per page, the component definitions that only this route uses, in
    // definition order. Prepended to that page's chunk; removed from the bundle.
    private var chunk_prefixes : std::vector<std::string>
    private var chunked_units : ubigint
    private var retained_units : ubigint
    // Component definitions that more than one route uses: one `vendor` bundle
    // every page loads (still cached forever), so the runtime bundle stays the
    // size of the runtime.
    private var vendor_bytes : std::string
    private var vendor_js_path : std::string
    private var vendor_units : ubigint
    /// Why the build kept every unit in the bundle (empty when it split). A
    /// conservative reason is not a failure: the shared bundle is always correct.
    private var split_note_text : std::string

    // ── Profile (per-route bytes, §Phase 5) ──────────────────────────────
    // Recorded while building, because that is when a page's HTML and chunk bytes
    // are in hand; `profile_report()` and `export_profile()` read them back.
    private var page_html_sizes : std::vector<ubigint>
    private var page_chunk_sizes : std::vector<ubigint>
    private var page_chunk_units : std::vector<ubigint>

    @make
    public func make(out_dir : std::string_view, base_name : std::string_view = "app", asset_prefix : std::string_view = "assets") : StaticSite {
        var dir_s = std::string()
        dir_s.append_view(&out_dir)
        var base_s = std::string()
        base_s.append_view(&base_name)
        var prefix_s = std::string()
        prefix_s.append_view(&asset_prefix)
        return StaticSite {
            assets : SharedAssetsHandle.make(&base_s),
            pages : std::vector<*mut HtmlPage>(),
            paths : std::vector<std::string_view>(),
            out_dir : dir_s,
            asset_prefix : prefix_s,
            head_tags : std::string(),
            bundle_js_path : std::string(),
            bundle_css_path : std::string(),
            artifacts : std::vector<SiteArtifact>(),
            built : false,
            build_failed : false,
            written : false,
            failed : false,
            failure : std::string(),
            dynamic_error : std::string(),
            want_gzip : false,
            site_url : std::string(),
            chunk_prefixes : std::vector<std::string>(),
            chunked_units : 0,
            retained_units : 0,
            vendor_bytes : std::string(),
            vendor_js_path : std::string(),
            vendor_units : 0,
            split_note_text : std::string(),
            page_html_sizes : std::vector<ubigint>(),
            page_chunk_sizes : std::vector<ubigint>(),
            page_chunk_units : std::vector<ubigint>()
        }
    }

    /// The absolute origin URLs in `export_sitemap` are written against
    /// (`https://example.com`, no trailing slash). Call before `build()`.
    public func set_site_url(&mut self, url : std::string_view) {
        site_url.clear()
        site_url.append_view(&url)
    }

    /// Minify the frozen bundle's JS and CSS (`SharedAssets.set_minify`). Call
    /// before `build()`; a built site ignores it. Off by default — generated bytes
    /// stay readable while developing, and are only worth shrinking on the wire.
    public func set_minify(&mut self, on : bool) {
        if(!assets.is_valid()) {
            fail_with("shared asset allocation failed")
            return
        }
        assets.get_mut().set_minify(on)
    }

    public func is_minify_enabled(&self) : bool {
        if(!assets.is_valid()) { return false }
        return assets.get().is_minify_enabled()
    }

    /// Precompress every text artifact with gzip and negotiate it per request
    /// (`Accept-Encoding`). Call before `build()`; a built site ignores it.
    ///
    /// A compressed variant is only kept when it is smaller than the original, so
    /// an already-compressed asset (an image) never grows. `<path>.gz` sidecars are
    /// written by `finish()`, which is what a static host's `gzip_static` wants.
    public func set_gzip(&mut self, on : bool) {
        if(built) { return }
        want_gzip = on
    }

    public func is_gzip_enabled(&self) : bool {
        return want_gzip
    }

    /// True when some artifact carries a compressed variant, so a server knows the
    /// response's bytes depend on `Accept-Encoding` (`Vary`).
    public func has_gzip_variants(&self) : bool {
        var i : ubigint = 0
        while(i < artifacts.size()) {
            var a = unsafe(&*artifacts.get_ptr(i))
            if(!a.gzipped.empty()) { return true }
            i = i + 1
        }
        return false
    }

    /// Attach the shared sink to `page` and record it at `path` (relative to the
    /// output directory). Call this before rendering into the page: everything the
    /// page emits while attached that is page-independent goes to the bundle.
    public func add(&mut self, page : &mut HtmlPage, path : std::string_view) {
        if(!assets.is_valid()) {
            fail_with("shared asset allocation failed")
            return
        }
        var shared = assets.get_mut()
        page.attach_shared_ref(shared)
        // Route token for the unit graph: 1-based, so 0 keeps meaning "this page
        // is not part of a site". Set before the page renders — that is when its
        // component uses are recorded.
        page.unit_owner = pages.size() + 1
        pages.push_back(page as *mut HtmlPage)
        paths.push_back(path)
    }

    // ── Unit graph queries (build diagnostics) ────────────────────────────
    /// Component definitions moved out of the shared bundle into a page's own
    /// chunk. Zero when the build kept the whole bundle shared (see
    /// `split_note()`), or when every component is used by more than one route.
    public func chunked_unit_count(&self) : ubigint {
        return chunked_units
    }

    /// Component definitions that stayed in the runtime bundle because the graph
    /// could not place them (an unsafe range, or a component with no recorded
    /// range). Zero on a fully-described graph.
    public func retained_unit_count(&self) : ubigint {
        return retained_units
    }

    /// Component definitions in the shared `vendor` bundle: the ones more than one
    /// route — or a per-request route — uses.
    public func vendor_unit_count(&self) : ubigint {
        return vendor_units
    }

    /// Site-relative URL of the `vendor` bundle (empty when nothing is shared).
    public func bundle_vendor_url(&self) : std::string_view {
        return vendor_js_path.to_view()
    }

    /// Why the chunk plan was conservative (empty when every unit could be
    /// classified from its recorded definition). A note is never a failure: a
    /// unit left in the shared bundle is always served correctly, just larger.
    public func split_note(&self) : std::string_view {
        return split_note_text.to_view()
    }

    /// Components a route used that have no recorded byte range (SSR-only, or an
    /// inline definition the planner never moves). Zero on a fully-described graph.
    public func unbacked_unit_count(&self) : ubigint {
        return assets.get().unbacked_usage_count()
    }

    /// The recorded unit graph as `name:owners` entries, comma-separated (build
    /// diagnostics and CI: which route uses which component definition).
    public func unit_report(&self) : std::string {
        var out = std::string()
        var sink = assets.get()
        var i : ubigint = 0
        while(i < sink.unit_count()) {
            if(!out.empty()) { out.append(',') }
            var name = sink.unit_name(i)
            out.append_view(&name)
            out.append(':')
            out.append_uinteger(sink.owner_count(sink.unit_hash(i)))
            i = i + 1
        }
        return out
    }

    // ── Profile: per-route bytes, chunk composition, navigation cost ─────
    /// Bytes of the artifact at `path` (0 when there is none).
    func artifact_size_of(&self, path : &std::string_view) : ubigint {
        var index = artifact_index_of(path)
        if(index < 0) { return 0 }
        var a = unsafe(&*artifacts.get_ptr(index as ubigint))
        return a.bytes.size()
    }

    /// Bytes of the compressed representation of the artifact at `path` (0 when
    /// there is none: `set_gzip(false)`, a non-text type, or no win).
    func artifact_gzip_size_of(&self, path : &std::string_view) : ubigint {
        var index = artifact_index_of(path)
        if(index < 0) { return 0 }
        var a = unsafe(&*artifacts.get_ptr(index as ubigint))
        return a.gzipped.size()
    }

    /// Per-route byte accounting for a built site, one line per fact:
    ///
    ///   shared bundle=<B> gzip=<B> vendor=<B> vendor_gzip=<B> css=<B>
    ///   route path=<route> html=<B> chunk=<B> units=<N> first_load=<B> navigation=<B>
    ///   total routes=<N> first_load_max=<B> navigation_max=<B>
    ///
    /// `first_load` is what a cold client fetches for that route (its HTML, its
    /// chunk, and the shared bundle/vendor/CSS once); `navigation` is what an
    /// already-warm client fetches to move to it (its HTML plus its chunk — the
    /// shared assets are cache hits). `units` is how many component definitions the
    /// chunk carries, which is the chunk's composition in one number.
    ///
    /// Deterministic: routes are in registration order, so two builds can be
    /// diffed. Empty before `build()` (a profile of nothing is not a profile).
    public func profile_report(&self) : std::string {
        var out = std::string()
        if(!built || build_failed) {
            out.append_view("profile unavailable: site not built")
            return out
        }
        var bundle_view = bundle_js_path.to_view()
        var vendor_view = vendor_js_path.to_view()
        var css_view = bundle_css_path.to_view()
        var bundle_bytes = artifact_size_of(&bundle_view)
        var vendor_bytes = artifact_size_of(&vendor_view)
        var css_bytes = artifact_size_of(&css_view)
        var shared_bytes = bundle_bytes + vendor_bytes + css_bytes

        out.append_view("shared bundle=")
        out.append_uinteger(bundle_bytes)
        out.append_view(" gzip=")
        out.append_uinteger(artifact_gzip_size_of(&bundle_view))
        out.append_view(" vendor=")
        out.append_uinteger(vendor_bytes)
        out.append_view(" vendor_gzip=")
        out.append_uinteger(artifact_gzip_size_of(&vendor_view))
        out.append_view(" css=")
        out.append_uinteger(css_bytes)
        out.append_view("\n")

        var first_load_max : ubigint = 0
        var navigation_max : ubigint = 0
        var i : ubigint = 0
        while(i < page_html_sizes.size()) {
            var html = page_html_sizes.at(i)
            var chunk : ubigint = 0
            if(i < page_chunk_sizes.size()) { chunk = page_chunk_sizes.at(i) }
            var units : ubigint = 0
            if(i < page_chunk_units.size()) { units = page_chunk_units.at(i) }
            var first_load = html + chunk + shared_bytes
            var navigation = html + chunk
            if(first_load > first_load_max) { first_load_max = first_load }
            if(navigation > navigation_max) { navigation_max = navigation }
            var target = paths.at(i)
            out.append_view("route path=")
            out.append_view(&target)
            out.append_view(" html=")
            out.append_uinteger(html)
            out.append_view(" chunk=")
            out.append_uinteger(chunk)
            out.append_view(" units=")
            out.append_uinteger(units)
            out.append_view(" first_load=")
            out.append_uinteger(first_load)
            out.append_view(" navigation=")
            out.append_uinteger(navigation)
            out.append_view("\n")
            i = i + 1
        }

        out.append_view("total routes=")
        out.append_uinteger(page_html_sizes.size())
        out.append_view(" first_load_max=")
        out.append_uinteger(first_load_max)
        out.append_view(" navigation_max=")
        out.append_uinteger(navigation_max)
        out.append_view("\n")
        return out
    }

    /// Add `profile.json` to the artifact set, so a deploy carries the same
    /// per-route accounting `profile_report()` prints. Call after `build()` and
    /// before `finish()`.
    ///
    /// The numbers are the build's, computed from the bytes that are actually
    /// served; nothing here is measured at request time.
    public func export_profile(&mut self) : bool {
        if(!built || build_failed) {
            fail_with("export_profile needs a built site")
            return false
        }
        var bundle_view = bundle_js_path.to_view()
        var vendor_view = vendor_js_path.to_view()
        var css_view = bundle_css_path.to_view()
        var shared_bytes = artifact_size_of(&bundle_view) + artifact_size_of(&vendor_view) + artifact_size_of(&css_view)

        var json = std::string()
        json.append_view("{\n  \"shared\": {\"bundle\": ")
        json.append_uinteger(artifact_size_of(&bundle_view))
        json.append_view(", \"bundle_gzip\": ")
        json.append_uinteger(artifact_gzip_size_of(&bundle_view))
        json.append_view(", \"vendor\": ")
        json.append_uinteger(artifact_size_of(&vendor_view))
        json.append_view(", \"css\": ")
        json.append_uinteger(artifact_size_of(&css_view))
        json.append_view("},\n  \"routes\": [")
        var i : ubigint = 0
        while(i < page_html_sizes.size()) {
            if(i > 0) { json.append_view(", ") }
            var html = page_html_sizes.at(i)
            var chunk : ubigint = 0
            if(i < page_chunk_sizes.size()) { chunk = page_chunk_sizes.at(i) }
            var units : ubigint = 0
            if(i < page_chunk_units.size()) { units = page_chunk_units.at(i) }
            var target = paths.at(i)
            json.append_view("{\"path\": \"")
            append_chemical_json_string(&mut json, target)
            json.append_view("\", \"html\": ")
            json.append_uinteger(html)
            json.append_view(", \"chunk\": ")
            json.append_uinteger(chunk)
            json.append_view(", \"units\": ")
            json.append_uinteger(units)
            json.append_view(", \"first_load\": ")
            json.append_uinteger(html + chunk + shared_bytes)
            json.append_view(", \"navigation\": ")
            json.append_uinteger(html + chunk)
            json.append_view("}")
            i = i + 1
        }
        json.append_view("]\n}\n")
        var path = std::string()
        path.append_view("profile.json")
        push_artifact(path, json, std::string_view("application/json; charset=utf-8"), false)
        return true
    }

    public func page_count(&self) : ubigint {
        return pages.size()
    }

    /// The `<head>` tags a page at the site root needs (empty before `build()`).
    public func head_assets_html(&self) : std::string_view {
        return head_tags.to_view()
    }

    /// Site-relative URLs of the shared bundle, e.g. `assets/app.<hash>.js`.
    /// Empty when that half of the bundle is empty (or before `build()`).
    public func bundle_js_url(&self) : std::string_view {
        return bundle_js_path.to_view()
    }

    public func bundle_css_url(&self) : std::string_view {
        return bundle_css_path.to_view()
    }

    public func error_message(&self) : std::string_view {
        return failure.to_view()
    }

    // ── Manifest ─────────────────────────────────────────────────────────
    public func artifact_count(&self) : ubigint {
        return artifacts.size()
    }

    /// Site-relative path of artifact `i` (`index.html`, `assets/app.<hash>.js`).
    /// Out of range is an empty view, not a panic: this is a read-only query a
    /// caller may loop over.
    public func artifact_path(&self, i : ubigint) : std::string_view {
        if(i >= artifacts.size()) { return std::string_view("") }
        var a = unsafe(&*artifacts.get_ptr(i))
        return a.path.to_view()
    }

    public func artifact_bytes(&self, i : ubigint) : std::string_view {
        if(i >= artifacts.size()) { return std::string_view("") }
        var a = unsafe(&*artifacts.get_ptr(i))
        return a.bytes.to_view()
    }

    public func artifact_content_type(&self, i : ubigint) : std::string_view {
        if(i >= artifacts.size()) { return std::string_view("") }
        var a = unsafe(&*artifacts.get_ptr(i))
        return a.content_type
    }

    /// True when artifact `i`'s URL is content-hashed, so it may be cached forever.
    public func artifact_immutable(&self, i : ubigint) : bool {
        if(i >= artifacts.size()) { return false }
        var a = unsafe(&*artifacts.get_ptr(i))
        return a.immutable
    }

    // ── Two-phase build: generated source ────────────────────────────────
    /// Write a Chemical module carrying every artifact as string constants, so the
    /// next build compiles the site's bytes **into** the program and the server
    /// needs neither the source tree nor the output directory.
    ///
    /// The generated module exposes (with `module_name` as the prefix):
    ///
    ///   <module>.asset_count()          : ubigint
    ///   <module>.asset_path(i)          : std::string_view
    ///   <module>.asset_content_type(i)  : std::string_view
    ///   <module>.asset_immutable(i)     : bool
    ///   <module>.asset_bytes(i)         : std::string
    ///
    /// `asset_bytes` returns an owned string (a definition can be larger than one
    /// literal, so it is built from chunks), which `Bundle.add_owned` takes without
    /// copying.
    ///
    /// Call after `build()`: a build that has not run has no manifest to embed.
    public func write_assets_module(&self, path : std::string_view, module_name : std::string_view) : bool {
        if(!built || build_failed) { return false }
        var out = std::string()
        out.append_view("// Generated by page.StaticSite.write_assets_module. Do not edit.\n")
        out.append_view("//\n// The built site's bytes, compiled into the program so a deployed server can\n")
        out.append_view("// answer every request without the source tree or the output directory.\n")
        out.append_view("using std::string;\nusing std::string_view;\n\n")

        // Namespaced accessors: one module per site, so two sites cannot collide.
        var prefix = std::string()
        prefix.append_view(&module_name)
        var prefix_view = prefix.to_view()

        var count_s = std::string()
        count_s.append_uinteger(artifacts.size())
        var count_view = count_s.to_view()
        out.append_view("public func ")
        out.append_view(&prefix_view)
        out.append_view("_asset_count() : ubigint { return ")
        out.append_view(&count_view)
        out.append_view(" as ubigint }\n\n")

        var i : ubigint = 0
        while(i < artifacts.size()) {
            var a = unsafe(&*artifacts.get_ptr(i))
            var n_s = std::string()
            n_s.append_uinteger(i)
            var n = n_s.to_view()

            out.append_view("func ")
            out.append_view(&prefix_view)
            out.append_view("_asset_bytes_")
            out.append_view(&n)
            out.append_view("() : string {\n    var out = string()\n")
            append_string_chunks(&mut out, a.bytes.to_view())
            out.append_view("    return out\n}\n\n")
            i = i + 1
        }

        // The table: one accessor per property, dispatching on the index.
        out.append_view("public func ")
        out.append_view(&prefix_view)
        out.append_view("_asset_path(i : ubigint) : string_view {\n")
        append_index_cases(&mut out, &prefix_view, &artifacts, 0)
        out.append_view("    return string_view(\"\")\n}\n\n")

        out.append_view("public func ")
        out.append_view(&prefix_view)
        out.append_view("_asset_content_type(i : ubigint) : string_view {\n")
        append_index_cases(&mut out, &prefix_view, &artifacts, 1)
        out.append_view("    return string_view(\"text/plain; charset=utf-8\")\n}\n\n")

        out.append_view("public func ")
        out.append_view(&prefix_view)
        out.append_view("_asset_immutable(i : ubigint) : bool {\n")
        append_index_cases(&mut out, &prefix_view, &artifacts, 2)
        out.append_view("    return false\n}\n\n")

        out.append_view("public func ")
        out.append_view(&prefix_view)
        out.append_view("_asset_bytes(i : ubigint) : string {\n")
        var j : ubigint = 0
        while(j < artifacts.size()) {
            var n_s = std::string()
            n_s.append_uinteger(j)
            var n = n_s.to_view()
            out.append_view("    if(i == ")
            out.append_view(&n)
            out.append_view(" as ubigint) { return ")
            out.append_view(&prefix_view)
            out.append_view("_asset_bytes_")
            out.append_view(&n)
            out.append_view("() }\n")
            j = j + 1
        }
        out.append_view("    return string()\n}\n")

        var path_owned = std::string()
        path_owned.append_view(&path)
        // Create the module's directory: a build step passes a path under a
        // generated-source directory that need not exist yet.
        ensure_file_dir(path)
        var path_view = path_owned.to_view()
        var out_view = out.to_view()
        var result = unsafe(fs::raw::write_text_file(path_view.data(), out_view.data() as *u8, out_view.size()))
        if(result is std::Result.Err) {
            return false
        }
        return true
    }

    // One `if(i == n) { return <literal>/<bool> }` per artifact for column `column`
    // (0 = path, 1 = content type, 2 = immutable).
    func append_index_cases(&self, out : &mut std::string, prefix : &std::string_view, rows : &std::vector<SiteArtifact>, column : int) {
        var j : ubigint = 0
        while(j < rows.size()) {
            var a = unsafe(&*rows.get_ptr(j))
            var n_s = std::string()
            n_s.append_uinteger(j)
            var n = n_s.to_view()
            out.append_view("    if(i == ")
            out.append_view(&n)
            out.append_view(" as ubigint) { return ")
            if(column == 0) {
                append_chemical_string(out, a.path.to_view())
            } else if(column == 1) {
                append_chemical_string(out, a.content_type)
            } else {
                if(a.immutable) {
                    out.append_view("true")
                } else {
                    out.append_view("false")
                }
            }
            out.append_view(" }\n")
            j = j + 1
        }
        var _ = prefix
    }

    /// Build the whole site **in memory**: freeze the shared bundle, then render
    /// every page with a reference to it plus its own script chunk. Touches no
    /// files, so a server can build once and answer every request from RAM with
    /// `respond()`. `finish()` is `build()` plus writing the artifacts to disk.
    ///
    /// Render every page before calling this: a page rendered afterwards would
    /// change the bundle bytes after its name was fixed.
    public func build(&mut self) : bool {
        var ok = build_in_memory()
        if(!ok) { build_failed = true }
        return ok
    }

    func build_in_memory(&mut self) : bool {
        if(built) { return !failed }
        built = true

        if(!assets.is_valid()) {
            fail_with("shared asset allocation failed")
            return false
        }

        // Freeze before naming anything: the bundle is served from a long-lived
        // cache, so its URL may only be published once every page has rendered.
        var shared = assets.get_mut()

        // Give each route the component definitions only it uses, then hash the
        // bundle that is left (§3.13). Splitting must happen before `freeze()`: it
        // changes the bytes, and the published name has to describe them.
        var warm : ubigint = 0
        while(warm < pages.size()) {
            chunk_prefixes.push_back(std::string())
            warm = warm + 1
        }
        plan_chunks(shared)
        plan_vendor(shared)

        shared.freeze()
        if(shared.is_stale()) {
            fail_with("the shared bundle changed after it was frozen; render every page before build()")
            return false
        }

        var prefix_view = asset_prefix.to_view()
        var js_name = shared.js_name()
        var css_name = shared.css_name()
        // An empty bundle has nothing to serve: naming it would still publish a
        // zero-byte artifact and offer it to every page as a `<script>`/`<link>`.
        if(shared.js_size() == 0u) { js_name = std::string() }
        if(shared.css_size() == 0u) { css_name = std::string() }

        // The bundle is an artifact like any other, so `respond()` needs no special
        // case and `finish()` writes everything from one list.
        if(!js_name.empty()) {
            var name_view = js_name.to_view()
            bundle_js_path.append_view(&prefix_view)
            bundle_js_path.append('/')
            bundle_js_path.append_view(&name_view)

            var artifact_path = std::string()
            var bjp_view = bundle_js_path.to_view()
            artifact_path.append_view(&bjp_view)
            var bytes = std::string()
            var js_view = shared.js()
            bytes.append_view(&js_view)
            push_artifact(artifact_path, bytes, std::string_view("text/javascript; charset=utf-8"), true)
        }
        if(!css_name.empty()) {
            var name_view = css_name.to_view()
            bundle_css_path.append_view(&prefix_view)
            bundle_css_path.append('/')
            bundle_css_path.append_view(&name_view)

            var artifact_path = std::string()
            var bcp_view = bundle_css_path.to_view()
            artifact_path.append_view(&bcp_view)
            var bytes = std::string()
            var css_view = shared.css()
            bytes.append_view(&css_view)
            push_artifact(artifact_path, bytes, std::string_view("text/css; charset=utf-8"), true)
        }

        // The `vendor` bundle: every shared component definition, in one file the
        // browser caches forever. Written before the runtime bundle's tags are built
        // so both the pre-rendered pages and `render_dynamic` emit it.
        var vendor_name = std::string()
        if(!vendor_bytes.empty()) {
            var vendor_view = vendor_bytes.to_view()
            var vendor_hash = hex12(fnv1_hash_view(&vendor_view))
            var base = shared.base_name()
            vendor_name.append_view(&base)
            vendor_name.append_view(".vendor.")
            var vendor_hash_view = vendor_hash.to_view()
            vendor_name.append_view(&vendor_hash_view)
            vendor_name.append_view(".js")

            vendor_js_path.append_view(&prefix_view)
            vendor_js_path.append('/')
            var vendor_name_view = vendor_name.to_view()
            vendor_js_path.append_view(&vendor_name_view)

            var artifact_path = std::string()
            var vjp_view = vendor_js_path.to_view()
            artifact_path.append_view(&vjp_view)
            var bytes = std::string()
            bytes.append_view(&vendor_view)
            push_artifact(artifact_path, bytes, std::string_view("text/javascript; charset=utf-8"), true)
        }

        // Stylesheet first, then the scripts: a classic script in <head> has run by
        // the time the page's own script at the end of <body> executes. The root
        // page's tags do not depend on which page was registered first.
        var root_prefix = std::string()
        root_prefix.append_view(&prefix_view)
        root_prefix.append('/')
        var root_prefix_view = root_prefix.to_view()
        var js_name_view = js_name.to_view()
        var css_name_view = css_name.to_view()
        var vendor_name_view = vendor_name.to_view()
        var root_tags = head_tags_for(&root_prefix_view, &css_name_view, &vendor_name_view, &js_name_view)
        var root_tags_view = root_tags.to_view()
        head_tags.append_view(&root_tags_view)

        var i : ubigint = 0
        // A page that dispatches hydration calls (`$__uni_c(...)`) only works if
        // the bundle defines the runtime, and the runtime is written by
        // `page.defaultUniversalSetup()`. Forgetting that call is invisible until a
        // browser throws `$__uni_c is not a function`, so the build refuses it.
        var runtime_checked : bool = false
        var runtime_present : bool = false
        while(i < pages.size()) {
            var target = paths.at(i)
            var page_ptr = pages.at(i)
            // The site keeps raw pointers so it never copies a page (a page owns
            // several buffers and maps). Dereferencing one is the single audited
            // step: it came from a `&mut` the caller promised outlives `build()`.
            var page_ref = unsafe(&mut *page_ptr)

            // A page's asset URLs are relative to the *page*, not to the site root:
            // `about/index.html` must reach `../assets/…`, not `assets/…`. Getting
            // this wrong is invisible locally and breaks every nested route.
            var url_prefix_s = page_url_prefix(target, prefix_view)
            var url_prefix = url_prefix_s.to_view()

            // The page's own JS (its dispatch statements, preceded by any component
            // definitions only this route uses) becomes its own hashed chunk:
            // cacheable per route, and the HTML ends up with no inline <script> at
            // all (the CSP rule). A definition is emitted before the dispatch that
            // calls it, so the chunk is self-contained.
            var body_src = std::string()
            var page_js = page_ref.getFinalizedPageJs()
            var chunk_src = std::string()
            var own_prefix = unsafe(&*chunk_prefixes.get_ptr(i))
            if(!own_prefix.empty()) {
                chunk_src.append_string(own_prefix)
            }
            chunk_src.append_string(&page_js)
            if(!runtime_checked && !page_js.empty()) {
                var page_js_probe = page_js.to_view()
                const dispatch_call = std::string_view("$__uni_c(")
                if(page_js_probe.contains(&dispatch_call)) {
                    // Spacing-free on purpose: a minified bundle writes
                    // `window.$__uni_c=…`, and the check must not depend on the
                    // whitespace policy of whatever produced the bytes.
                    const runtime_definition = std::string_view("window.$__uni_c")
                    var shared_js = shared.js()
                    runtime_present = shared_js.contains(&runtime_definition)
                    runtime_checked = true
                }
            }
            if(!chunk_src.empty()) {
                // Named by content only, not by route: 40 routes whose chunk bytes
                // are identical are one file and one request (`push_artifact`
                // dedupes the identical path), instead of 40 copies of 28 bytes.
                const stem = std::string_view("chunk")
                var stem_view = stem
                var chunk_view = chunk_src.to_view()
                var page_hash = hex12(fnv1_hash_view(&chunk_view))
                var hash_view = page_hash.to_view()

                // The URL the page uses…
                body_src.append_view(&url_prefix)
                body_src.append_view(&stem_view)
                body_src.append('.')
                body_src.append_view(&hash_view)
                body_src.append_view(".js")

                // …and the same file, site-relative, for the artifact list.
                var chunk_path = std::string()
                chunk_path.append_view(&prefix_view)
                chunk_path.append('/')
                chunk_path.append_view(&stem_view)
                chunk_path.append('.')
                chunk_path.append_view(&hash_view)
                chunk_path.append_view(".js")
                var chunk_bytes = std::string()
                chunk_bytes.append_view(&chunk_view)
                push_artifact(chunk_path, chunk_bytes, std::string_view("text/javascript; charset=utf-8"), true)
            }

            // A page that emits no client JS of its own needs neither the runtime
            // bundle nor the vendor bundle: linking them would make an `#html`-only
            // marketing page download the hydration runtime it never runs. The
            // stylesheet stays — a page may use classes the shared CSS defines.
            var page_tags = head_tags_for(&url_prefix, &css_name_view, &vendor_name_view, &js_name_view)
            if(chunk_src.empty()) {
                var no_asset = std::string_view("")
                page_tags = head_tags_for(&url_prefix, &css_name_view, &no_asset, &no_asset)
            }
            var page_tags_view = page_tags.to_view()
            var body_src_view = body_src.to_view()
            var html = page_ref.toStringWithAssets(page_tags_view, body_src_view)

            var page_path = std::string()
            page_path.append_view(&target)
            var html_view = html.to_view()
            page_html_sizes.push_back(html_view.size())
            var chunk_size : ubigint = 0
            if(!chunk_src.empty()) {
                var chunk_size_view = chunk_src.to_view()
                chunk_size = chunk_size_view.size() as ubigint
            }
            page_chunk_sizes.push_back(chunk_size)
            var unit_count : ubigint = 0
            if(!own_prefix.empty()) {
                const unit_marker = std::string_view("function ")
                var own_prefix_view = own_prefix.to_view()
                unit_count = occurrences(&own_prefix_view, &unit_marker)
            }
            page_chunk_units.push_back(unit_count)
            push_artifact(page_path, html, std::string_view("text/html; charset=utf-8"), false)
            i = i + 1
        }

        if(runtime_checked && !runtime_present) {
            fail_with("a page hydrates components but the shared bundle has no hydration runtime; call defaultUniversalSetup() on every page before rendering it")
            return false
        }
        return !failed
    }

    /// `build()` plus writing every artifact under `out_dir`. Returns false and
    /// sets `error_message()` on failure — including a write that reports success
    /// but leaves no file, which is how a "successful" empty build hides.
    public func finish(&mut self) : bool {
        if(written) { return !failed }
        if(!build()) { return false }
        written = true

        var out_view = out_dir.to_view()
        // `mkdir` here is single-level (`_mkdir`/`mkdir(2)`), so the output root has
        // to exist before anything below it can be created.
        ensure_dir_path(out_view)

        var i : ubigint = 0
        while(i < artifacts.size()) {
            var a = unsafe(&*artifacts.get_ptr(i))
            var rel = a.path.to_view()
            ensure_parent_dirs(out_view, rel)

            var file = std::string()
            file.append_view(&out_view)
            file.append('/')
            file.append_view(&rel)
            var file_view = file.to_view()
            var result = unsafe(fs::raw::write_text_file(file_view.data(), a.bytes.data() as *u8, a.bytes.size()))
            if(result is std::Result.Err) {
                // A page that silently failed to write is a build that "succeeds"
                // with a missing page, so this is fatal and names the file.
                fail_with("could not write")
                fail_with(rel)
                return false
            }
            i = i + 1
        }

        // Compressed sidecars for a static host (`gzip_static`): `<path>.gz` next to
        // every artifact that has one. They are deliberately *not* artifacts, so the
        // manifest keeps describing the URLs a browser requests rather than the
        // files a server may pick from.
        var g : ubigint = 0
        while(g < artifacts.size()) {
            var ga = unsafe(&*artifacts.get_ptr(g))
            if(!ga.gzipped.empty()) {
                var gz_rel = ga.path.to_view()
                var gz_file = std::string()
                gz_file.append_view(&out_view)
                gz_file.append('/')
                gz_file.append_view(&gz_rel)
                gz_file.append_view(".gz")
                var gz_file_view = gz_file.to_view()
                var gz_result = unsafe(fs::raw::write_text_file(gz_file_view.data(), ga.gzipped.data() as *u8, ga.gzipped.size()))
                if(gz_result is std::Result.Err) {
                    fail_with("could not write")
                    fail_with(gz_file_view)
                    return false
                }
            }
            g = g + 1
        }

        if(artifacts.size() > 0) {
            var first = unsafe(&*artifacts.get_ptr(0))
            if(!artifact_exists(out_view, &first.path.to_view())) {
                fail_with("no artifact reached the output directory")
                return false
            }
        }
        return true
    }

    // ── Serving ──────────────────────────────────────────────────────────
    /// Answer `request_path` from the built artifacts. `/` and `/about/` resolve
    /// to their `index.html`; `/about` does too (a directory without its trailing
    /// slash is the common hand-typed URL). Content-hashed assets are marked
    /// immutable, HTML is not. Unknown paths get a 404 page; a request before
    /// `build()` (or after a failed build) gets a 503 rather than a half-built
    /// site. A failed *write* does not affect serving: the memory build is intact.
    ///
    /// The body borrows the site's buffers: serve it before mutating the site.
    public func respond(&self, request_path : std::string_view) : SiteResponse {
        return respond_if_none_match(request_path, std::string_view(""))
    }

    /// `respond`, but honouring a conditional request: when `if_none_match` is the
    /// artifact's ETag the response is a **304** with no body (the client keeps
    /// its cached copy). Pass the request's `If-None-Match` header here.
    ///
    /// Every artifact carries a strong ETag derived from its bytes, so a server
    /// answers revalidation without re-reading or re-hashing anything. 404/503
    /// responses carry no ETag.
    public func respond_if_none_match(&self, request_path : std::string_view, if_none_match : std::string_view) : SiteResponse {
        return respond_negotiated(request_path, if_none_match, std::string_view(""))
    }

    /// `respond_if_none_match`, plus content negotiation: when the artifact has a
    /// gzip variant (`set_gzip(true)`) and `accept_encoding` accepts gzip, the body
    /// is the compressed bytes with `Content-Encoding: gzip` and its own ETag, and
    /// the response carries `Vary: Accept-Encoding` so a cache never mixes the two
    /// representations of one URL.
    ///
    /// Pass the request's `Accept-Encoding` header here (empty means "identity").
    public func respond_negotiated(&self, request_path : std::string_view, if_none_match : std::string_view,
                                   accept_encoding : std::string_view) : SiteResponse {
        if(!built || build_failed) {
            return site_response(503, std::string_view("text/plain; charset=utf-8"),
                std::string_view("no-store"), std::string_view("site not built"), std::string_view(""))
        }
        var target_s = normalize_request_path(request_path)
        if(target_s.empty()) {
            return site_not_found()
        }
        var target = target_s.to_view()
        var index = artifact_index_of(&target)

        if(index < 0) {
            // `/about` -> `about/index.html`: the directory form of a route.
            var as_dir = std::string()
            as_dir.append_view(&target)
            as_dir.append_view("/index.html")
            var as_dir_view = as_dir.to_view()
            index = artifact_index_of(&as_dir_view)
        }
        if(index < 0) {
            return site_not_found()
        }

        var a = unsafe(&*artifacts.get_ptr(index as ubigint))
        var cache = std::string_view("no-cache")
        if(a.immutable) {
            cache = std::string_view("public, max-age=31536000, immutable")
        }
        var use_gzip = false
        if(!a.gzipped.empty() && accepts_gzip(accept_encoding)) { use_gzip = true }
        var etag = a.etag.to_view()
        var body = a.bytes.to_view()
        var encoding = std::string_view("")
        if(use_gzip) {
            var gz_etag_view = a.gz_etag.to_view()
            etag = gz_etag_view
            body = a.gzipped.to_view()
            encoding = std::string_view("gzip")
        }
        var vary = std::string_view("")
        if(!a.gzipped.empty()) { vary = std::string_view("Accept-Encoding") }
        if(!if_none_match.empty() && if_none_match.equals(&etag)) {
            return site_response_encoded(304, a.content_type, cache, std::string_view(""), etag, encoding, vary)
        }
        return site_response_encoded(200, a.content_type, cache, body, etag, encoding, vary)
    }

    // ── Export extras (static hosting) ───────────────────────────────────
    /// Add `sitemap.xml` for every **exported page** to the artifact set: one
    /// `<url>` per pre-rendered route, absolutised against `set_site_url`. A
    /// per-request route is not in it (it was never pre-rendered), so the sitemap
    /// can never advertise a URL this build does not serve from cache.
    ///
    /// Call before `build()`/`finish()`: the artifact list is what the build
    /// freezes, and adding to it afterwards would publish a manifest that
    /// disagrees with the bundle.
    public func export_sitemap(&mut self) : bool {
        if(built) {
            fail_with("export_sitemap must be called before build()")
            return false
        }
        if(site_url.empty()) {
            fail_with("export_sitemap needs set_site_url()")
            return false
        }
        var xml = std::string()
        xml.append_view("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
        xml.append_view("<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n")
        var i : ubigint = 0
        while(i < paths.size()) {
            var target = paths.at(i)
            var loc = sitemap_loc(&site_url, &target)
            var loc_view = loc.to_view()
            xml.append_view("  <url><loc>")
            xml.append_view(&loc_view)
            xml.append_view("</loc></url>\n")
            i = i + 1
        }
        xml.append_view("</urlset>\n")
        var path = std::string()
        path.append_view("sitemap.xml")
        push_artifact(path, xml, std::string_view("application/xml; charset=utf-8"), false)
        return true
    }

    /// Add `robots.txt` pointing at this build's sitemap. Same pre-build rule as
    /// `export_sitemap`.
    public func export_robots(&mut self, sitemap : bool = true) : bool {
        if(built) {
            fail_with("export_robots must be called before build()")
            return false
        }
        var body = std::string()
        body.append_view("User-agent: *\nAllow: /\n")
        if(sitemap) {
            if(site_url.empty()) {
                fail_with("export_robots needs set_site_url() for an absolute sitemap URL")
                return false
            }
            var loc = sitemap_loc(&site_url, &std::string_view("sitemap.xml"))
            var loc_view = loc.to_view()
            body.append_view("Sitemap: ")
            body.append_view(&loc_view)
            body.append('\n')
        }
        var path = std::string()
        path.append_view("robots.txt")
        push_artifact(path, body, std::string_view("text/plain; charset=utf-8"), false)
        return true
    }

    // ── Per-request (dynamic) routes ─────────────────────────────────────
    /// Attach `page` to the site's sink, so its page-independent bytes (the runtime,
    /// component definitions, global CSS) go to the shared bundle instead of into the
    /// page. Call this **before** rendering into the page.
    ///
    /// Before `build()` this is the **warm pass**: render every dynamic route once
    /// this way, discard the page, and the route's components become part of the
    /// frozen bundle. After `build()` the same call is how a request-time page is
    /// prepared; the render must not need anything the bundle lacks (see
    /// `render_dynamic`).
    public func attach_dynamic(&mut self, page : &mut HtmlPage) : bool {
        if(!assets.is_valid()) {
            dynamic_error.clear()
            dynamic_error.append_view("shared asset allocation failed")
            return false
        }
        var shared = assets.get_mut()
        page.attach_shared_ref(shared)
        // A per-request render dispatches its components from the shared bundle, so
        // whatever it uses must stay there (never moved into one static route's
        // chunk). The token says so from the moment the page renders.
        page.unit_owner = unit_owner_dynamic()
        return true
    }

    /// Render `page` for **this request** against the site's already-frozen bundle,
    /// and return the complete document.
    ///
    /// The page is attached to the site's sink before rendering, so everything the
    /// bundle already carries — the hydration runtime, every component definition,
    /// the global CSS — contributes **no bytes**: the returned HTML has the site's
    /// frozen `<script src>`/`<link>` in `<head>` and only this request's markup
    /// plus its dispatch tail. Call `page.defaultUniversalSetup()` for the page (as
    /// for a pre-rendered page) before writing its `#html`.
    ///
    /// Returns an empty string when the site is not built, or when the route would
    /// need bytes the frozen bundle does not have (a component that never rendered
    /// before `build()`). That is not a warning: appending them would make the
    /// published, content-hashed URL disagree with the bytes it names, so the render
    /// is refused and `dynamic_error_message()` says what is missing. **Warm every
    /// dynamic route once before `build()`** — render it into a throwaway page — and
    /// its components become part of the bundle.
    ///
    /// After that warm pass the sink is only read, so concurrent requests are safe
    /// while it stays frozen; do not rebuild the site while serving it.
    public func render_dynamic(&mut self, page : &mut HtmlPage) : std::string {
        var out = std::string()
        dynamic_error.clear()
        if(!assets.is_valid()) {
            dynamic_error.append_view("shared asset allocation failed")
            return out
        }
        if(!built || build_failed) {
            dynamic_error.append_view("site not built; dynamic routes render after build()")
            return out
        }
        var shared = assets.get_mut()
        if(!shared.is_frozen()) {
            dynamic_error.append_view("site not frozen; call build() first")
            return out
        }
        if(!attach_dynamic(page)) { return out }
        var head_view = head_tags.to_view()
        var empty_src = std::string_view("")
        var html = page.toStringWithAssets(head_view, empty_src)
        if(shared.is_stale()) {
            // The page needed a component the bundle does not contain. Say so, and
            // name the fix, instead of serving HTML whose hydration cannot run.
            dynamic_error.append_view("dynamic route needs components the frozen bundle does not contain; render the route once before build()")
            out.clear()
            return out
        }
        out.append_string(&html)
        return out
    }

    /// Answer a per-request render. `html` must outlive the response — the body is
    /// a view into it, never a copy.
    ///
    /// 200 `text/html` with `no-store` by default: the same URL now returns
    /// different bytes on every request, so a shared cache must not keep them. Pass
    /// `"private, no-cache"` only when the render is actually stable per URL, and
    /// add your own validator if you do. No `ETag` is set — there is nothing for a
    /// client to revalidate against.
    ///
    /// An empty `html` (a refused `render_dynamic`, or a route that chose to render
    /// nothing) is a **503**, not a 200 with an empty page: it means the site could
    /// not render, and `dynamic_error_message()` carries the reason.
    public func dynamic_response(&self, html : &std::string, cache_control : std::string_view = "no-store") : SiteResponse {
        var body = html.to_view()
        if(!built || build_failed || body.empty()) {
            return site_response(503, std::string_view("text/plain; charset=utf-8"),
                std::string_view("no-store"), std::string_view("dynamic route could not be rendered"), std::string_view(""))
        }
        return site_response(200, std::string_view("text/html; charset=utf-8"), cache_control, body, std::string_view(""))
    }

    /// Why the last `render_dynamic` returned nothing (empty on success).
    public func dynamic_error_message(&self) : std::string_view {
        return dynamic_error.to_view()
    }

    // ── Internals ────────────────────────────────────────────────────────
    /// Move the definitions the route split left behind (the ones more than one
    /// route, or a per-request route, uses) into the `vendor` bundle. They are
    /// still one hashed file the browser caches forever, but the runtime bundle is
    /// now only the runtime.
    ///
    /// Only called when the recorded ranges are provably sane: without that, the
    /// bytes are left exactly where they are.
    func plan_vendor(&mut self, shared : &mut SharedAssets) {
        if(shared.unit_count() == 0) { return }
        if(!shared.ranges_are_safe()) { return }
        var i : ubigint = 0
        while(i < shared.unit_count()) {
            if(shared.unit_length(i) == 0) {
                i = i + 1
                continue
            }
            var bytes = shared.take_unit(i)
            if(bytes.empty()) {
                i = i + 1
                continue
            }
            vendor_bytes.append_string(&bytes)
            vendor_units = vendor_units + 1
            i = i + 1
        }
    }

    /// Move every component definition that exactly one pre-rendered route uses
    /// out of `shared` and into that route's chunk prefix. Everything else stays:
    /// the runtime, `#globaljs`, any component two routes share, and any component
    /// a per-request (live/shell) route renders.
    ///
    /// Falls back to one shared bundle — never a broken split — when the recorded
    /// ranges are not provably disjoint, or when a route uses a component the sink
    /// has no definition for: either means the graph cannot be trusted.
    func plan_chunks(&mut self, shared : &mut SharedAssets) {
        var total = shared.unit_count()
        if(total == 0) {
            split_note_text.append_view("no component definitions were recorded")
            return
        }
        if(!shared.ranges_are_safe()) {
            split_note_text.append_view("component definitions overlap in the shared buffer; kept one bundle")
            return
        }

        var ui : ubigint = 0
        while(ui < total) {
            if(shared.unit_length(ui) == 0) {
                ui = ui + 1
                continue
            }
            var hash = shared.unit_hash(ui)
            var owners = shared.owner_count(hash)
            var single : ubigint = unit_owner_none()
            var pinned = false
            var k : ubigint = 0
            while(k < owners) {
                var o = shared.owner_at(hash, k)
                if(o == unit_owner_dynamic()) {
                    pinned = true
                } else if(o != unit_owner_none()) {
                    if(single == unit_owner_none()) { single = o } else { pinned = true }
                }
                k = k + 1
            }

            if(pinned || single == unit_owner_none() || single > pages.size()) {
                retained_units = retained_units + 1
                ui = ui + 1
                continue
            }
            // A range that is not one self-contained definition (something else was
            // emitted inside it) never moves: the bytes might belong to a different
            // route.
            if(!shared.unit_is_self_contained(ui)) {
                retained_units = retained_units + 1
                ui = ui + 1
                continue
            }

            var target = unsafe(chunk_prefixes.get_ptr(single - 1))
            if(target == null) {
                retained_units = retained_units + 1
                ui = ui + 1
                continue
            }
            var bytes = shared.take_unit(ui)
            if(bytes.empty()) {
                retained_units = retained_units + 1
                ui = ui + 1
                continue
            }
            unsafe { target.append_string(&bytes) }
            chunked_units = chunked_units + 1
            ui = ui + 1
        }
    }

    /// Index of the artifact at `path`, or -1. Content-hashed names make the path
    /// an identity, so this is the whole manifest lookup.
    func artifact_index_of(&self, path : &std::string_view) : int {
        var i : ubigint = 0
        while(i < artifacts.size()) {
            var a = unsafe(&*artifacts.get_ptr(i))
            if(a.path.equals_view(path)) {
                return i as int
            }
            i = i + 1
        }
        return -1
    }

    /// Record one artifact. Two artifacts at the same path with the same bytes are
    /// one file (two routes with identical output share a chunk); different bytes
    /// at the same path is a build bug, not a merge.
    func push_artifact(&mut self, path : std::string, bytes : std::string, content_type : std::string_view, immutable : bool) {
        var path_view = path.to_view()
        var existing = artifact_index_of(&path_view)
        if(existing >= 0) {
            var a = unsafe(&*artifacts.get_ptr(existing as ubigint))
            var bytes_view = bytes.to_view()
            if(!a.bytes.equals_view(&bytes_view)) {
                fail_with("two different artifacts at the same path")
                fail_with(path_view)
            }
            return
        }
        var gzipped = std::string()
        var gz_etag = std::string()
        if(want_gzip && is_compressible_type(content_type)) {
            var gz = compress_artifact(&bytes)
            if(gz.size() > 0 && gz.size() < bytes.size()) {
                gz_etag = quoted_etag_of(&gz)
                gzipped = gz
            }
        }
        artifacts.push_back(SiteArtifact {
            path : path,
            bytes : bytes,
            content_type : content_type,
            immutable : immutable,
            etag : quoted_etag_of(&bytes),
            gzipped : gzipped,
            gz_etag : gz_etag
        })
    }

    func fail_with(&mut self, message : std::string_view) {
        failed = true
        if(!failure.empty()) {
            failure.append_view("; ")
        }
        failure.append_view(&message)
    }
}

/// One built file, held in memory: what `finish()` writes and `respond()` serves.
@direct_init
public struct SiteArtifact {
    /// Site-relative path (`index.html`, `assets/app.<hash>.js`).
    public var path : std::string
    public var bytes : std::string
    public var content_type : std::string_view
    /// Content-hashed, so the URL changes whenever the bytes do: cache forever.
    public var immutable : bool
    /// Strong validator for `If-None-Match`, quoted for the header
    /// (`"3f2a91c0d4e5"`). Derived from the bytes once, at build time.
    public var etag : std::string
    /// gzip-compressed copy of `bytes`, empty when there is none (not a text
    /// artifact, compression disabled, or the result was not smaller).
    public var gzipped : std::string
    /// Validator for `gzipped`. Deliberately **not** `etag`: one URL has two
    /// representations now, and a client that cached the identity body must not be
    /// told "not modified" for the compressed one.
    public var gz_etag : std::string
}

/// A static response. `body` borrows the site's buffer (no copy per request), so
/// it is valid until the site is rebuilt or dropped.
@direct_init
public struct SiteResponse {
    public var status : int
    public var content_type : std::string_view
    public var cache_control : std::string_view
    public var body : std::string_view
    /// The `ETag` header value; empty for 404/503.
    public var etag : std::string_view
    /// `Content-Encoding` for `body` (`gzip`), empty when the body is the identity
    /// representation.
    public var content_encoding : std::string_view
    /// `Vary` value this response needs because a compressed variant exists
    /// (`Accept-Encoding`), empty when there is nothing to negotiate.
    public var vary : std::string_view

    public func ok(&self) : bool {
        return status == 200
    }

    public func not_found(&self) : bool {
        return status == 404
    }

    /// True when the client's copy is still valid (no body, `etag` set).
    public func not_modified(&self) : bool {
        return status == 304
    }
}

func site_response(status : int, content_type : std::string_view, cache_control : std::string_view, body : std::string_view, etag : std::string_view) : SiteResponse {
    return SiteResponse {
        status : status,
        content_type : content_type,
        cache_control : cache_control,
        body : body,
        etag : etag,
        content_encoding : std::string_view(""),
        vary : std::string_view("")
    }
}

// The compressed representation of an artifact, with the headers that describe it.
func site_response_encoded(status : int, content_type : std::string_view, cache_control : std::string_view,
                           body : std::string_view, etag : std::string_view,
                           content_encoding : std::string_view, vary : std::string_view) : SiteResponse {
    return SiteResponse {
        status : status,
        content_type : content_type,
        cache_control : cache_control,
        body : body,
        etag : etag,
        content_encoding : content_encoding,
        vary : vary
    }
}

func site_not_found() : SiteResponse {
    return site_response(404, std::string_view("text/html; charset=utf-8"), std::string_view("no-store"),
        std::string_view("<!DOCTYPE html><html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1></body></html>"),
        std::string_view(""))
}

/// A strong ETag for `bytes`: the same 12-hex content hash the bundle's file name
/// uses, in quotes as RFC 7232 requires (`"3f2a91c0d4e5"`).
public func quoted_etag(bytes : std::string_view) : std::string {
    var owned = std::string()
    owned.append_view(&bytes)
    return quoted_etag_of(&owned)
}

func quoted_etag_of(bytes : &std::string) : std::string {
    var view = bytes.to_view()
    var hash = hex12(fnv1_hash_view(&view))
    var out = std::string()
    out.append('"')
    var hash_view = hash.to_view()
    out.append_view(&hash_view)
    out.append('"')
    return out
}

// True when `content_type` is worth compressing. Text is where gzip wins; an
// image or a font is already compressed and would only grow.
//
// Parameter matching is a prefix/suffix test on the type, not a full media-type
// parser: the build controls every content type it emits.
func is_compressible_type(content_type : std::string_view) : bool {
    var t = content_type
    if(t.starts_with(&std::string_view("text/"))) { return true }
    if(t.starts_with(&std::string_view("application/javascript"))) { return true }
    if(t.starts_with(&std::string_view("application/json"))) { return true }
    if(t.starts_with(&std::string_view("application/xml"))) { return true }
    if(t.starts_with(&std::string_view("image/svg+xml"))) { return true }
    return false
}

/// gzip `bytes`, or an empty string when the codec refuses. The caller keeps the
/// result only when it is smaller.
func compress_artifact(bytes : &std::string) : std::string {
    var view = bytes.to_view()
    var result = compression::gzip(view.as_bytes())
    if(result is std::Result.Err) { return std::string() }
    var Ok(packed) = result else unsafe { unreachable }
    var out = std::string()
    unsafe { out.append_with_len(packed.data() as *char, packed.size()) }
    return out
}

// True when `header[start..end]` is exactly `lit`.
func accept_token_is(header : std::string_view, start : size_t, end : size_t, lit : std::string_view) : bool {
    if(end < start) { return false }
    if(end - start != lit.size()) { return false }
    var i : size_t = 0
    while(i < lit.size()) {
        if(header.at(start + i) != lit.at(i)) { return false }
        i = i + 1
    }
    return true
}

// The `q=` value in `header[start..end]` is zero (`q=0`, `q=0.0`, `q=0.00`).
// A missing `q` is not zero: it means 1.
func accept_q_is_zero(header : std::string_view, start : size_t, end : size_t) : bool {
    var i = start
    var eq : size_t = end
    while(i < end) {
        if(header.at(i) == '=') { eq = i + 1; break }
        i = i + 1
    }
    if(eq >= end) { return false }
    var saw_digit = false
    var non_zero = false
    var k = eq
    while(k < end) {
        var c = header.at(k)
        if(c == '.') {
            k = k + 1
            continue
        }
        if(c < '0' || c > '9') { return false }
        saw_digit = true
        if(c != '0') { non_zero = true }
        k = k + 1
    }
    if(!saw_digit) { return false }
    return !non_zero
}

// An `Accept-Encoding` header accepts gzip when it names `gzip` with a non-zero
// quality, or names `*` with a non-zero quality and does not exclude gzip.
// `gzip;q=0` is an explicit refusal, which outranks a `*` wildcard.
func accepts_gzip(header : std::string_view) : bool {
    if(header.size() == 0) { return false }
    var i : size_t = 0
    var wildcard_ok = false
    var gzip_seen = false
    while(i < header.size()) {
        // One comma-separated token, trimmed of surrounding spaces.
        var start = i
        while(i < header.size() && header.at(i) != ',') { i = i + 1 }
        var end = i
        while(start < end && header.at(start) == ' ') { start = start + 1 }
        while(end > start && header.at(end - 1) == ' ') { end = end - 1 }
        // `name` runs to the first `;`, the parameters follow it.
        var name_end = start
        while(name_end < end && header.at(name_end) != ';') { name_end = name_end + 1 }
        var params_start = name_end
        if(params_start < end && header.at(params_start) == ';') { params_start = params_start + 1 }
        var q_is_zero = accept_q_is_zero(header, params_start, end)
        if(accept_token_is(header, start, name_end, std::string_view("gzip"))) {
            gzip_seen = true
            if(!q_is_zero) { return true }
        } else if(accept_token_is(header, start, name_end, std::string_view("*"))) {
            if(!q_is_zero) { wildcard_ok = true }
        }
        i = i + 1
    }
    if(gzip_seen) { return false }
    return wildcard_ok
}

// `<link>`/`<script src>` tags for a page whose asset URLs start with
// `url_prefix`. Stylesheet first: the bundle must have run before the page's own
// script at the end of <body>.
func head_tags_for(url_prefix : &std::string_view, css_name : &std::string_view, vendor_name : &std::string_view, js_name : &std::string_view) : std::string {
    var out = std::string()
    if(!css_name.empty()) {
        out.append_view("<link rel=\"stylesheet\" href=\"")
        out.append_view(url_prefix)
        out.append_view(css_name)
        out.append_view("\">")
    }
    // The shared component definitions load before the runtime bundle: both are
    // classic scripts in <head>, and the page's own dispatch chunk (end of <body>)
    // calls into both.
    if(!vendor_name.empty()) {
        out.append_view("<script src=\"")
        out.append_view(url_prefix)
        out.append_view(vendor_name)
        out.append_view("\"></script>")
    }
    if(!js_name.empty()) {
        out.append_view("<script src=\"")
        out.append_view(url_prefix)
        out.append_view(js_name)
        out.append_view("\"></script>")
    }
    return out
}

// Append `value` as a JSON string body (no surrounding quotes). Route paths are
// built from segment names, so this only has to handle the characters that would
// break the document.
func append_chemical_json_string(out : &mut std::string, value : std::string_view) {
    var i : ubigint = 0
    while(i < value.size()) {
        const c = value.at(i)
        if(c == '"') {
            out.append_view("\\\"")
        } else if(c == '\\') {
            out.append_view("\\\\")
        } else if(c == '\n') {
            out.append_view("\\n")
        } else {
            out.append(c)
        }
        i = i + 1
    }
}

// Append `value` to `out` as a Chemical string literal, escaped. A build emits
// the site's bytes as source, so this is the only place bytes and syntax meet:
// everything that could end the literal is escaped, and nothing else is touched
// (the bytes must stay byte-identical, or the embedded site would differ from the
// built one).
func append_chemical_string(out : &mut std::string, value : std::string_view) {
    out.append('"')
    var i : ubigint = 0
    while(i < value.size()) {
        const c = value.at(i)
        if(c == '"') {
            out.append_view("\\\"")
        } else if(c == '\\') {
            out.append_view("\\\\")
        } else if(c == '\n') {
            out.append_view("\\n")
        } else if(c == '\r') {
            out.append_view("\\r")
        } else if(c == '\t') {
            out.append_view("\\t")
        } else {
            out.append(c)
        }
        i = i + 1
    }
    out.append('"')
}

// Append `value` to `out` as a series of `out.append_view(<literal>)` statements.
// One literal per 3 KB keeps a single line (and a single literal) from growing to
// the size of a whole bundle, which is slow to lex and awkward to diff.
func append_string_chunks(out : &mut std::string, value : std::string_view) {
    const chunk : ubigint = 3000
    var i : ubigint = 0
    while(i < value.size()) {
        var len = value.size() - i
        if(len > chunk) { len = chunk }
        var piece = unsafe(std::string_view(value.data() + i, len))
        out.append_view("    out.append_view(")
        append_chemical_string(out, piece)
        out.append_view(")\n")
        i = i + len
    }
}

// A request path -> the artifact path it names: strip the leading `/`, drop the
// query/fragment, map a directory to its `index.html`, and reject anything that
// could leave the site root. Returns "" for a path that must not be served.
/// A request path -> the artifact path it names (`/` -> `index.html`, `/about/` ->
/// `about/index.html`), with the query/fragment dropped. Empty means the path
/// must not be served (a traversal attempt or a backslash).
///
/// Public because every served build — `StaticSite.respond` and a
/// `web::Bundle` loaded from disk or generated source — must agree on this
/// mapping; two spellings of it would be two different sites.
public func normalize_request_path(path : std::string_view) : std::string {
    var out = std::string()
    var i : ubigint = 0
    while(i < path.size() && path.at(i) == '/') { i = i + 1 }
    while(i < path.size()) {
        const c = path.at(i)
        if(c == '?' || c == '#') { break }
        // A backslash is a path separator on some hosts and never a legal URL
        // character; refuse rather than guess.
        if(c == '\\') { return std::string() }
        out.append(c)
        i = i + 1
    }

    // Reject `..` as a whole segment. The site is served from memory, so nothing
    // can escape, but a caller wiring this to a disk-backed variant must not have
    // to re-check.
    var j : ubigint = 0
    while(j + 1 < out.size()) {
        if(out.at(j) == '.' && out.at(j + 1) == '.') {
            const at_start = j == 0 || out.at(j - 1) == '/'
            const at_end = j + 2 == out.size() || out.at(j + 2) == '/'
            if(at_start && at_end) { return std::string() }
        }
        j = j + 1
    }

    if(out.empty()) {
        out.append_view("index.html")
        return out
    }
    if(out.at(out.size() - 1) == '/') {
        out.append_view("index.html")
    }
    return out
}

// The URL prefix one page needs to reach the site's asset directory: a page at
// `about/index.html` needs `../assets/`, the root page needs `assets/`.
//
// An **absolute** prefix (`/app/assets`, a mount path) is used as-is for every
// page: a base path is already absolute, so prepending `../` to it would point
// outside the mount.
func page_url_prefix(target : std::string_view, asset_prefix : std::string_view) : std::string {
    var out = std::string()
    if(asset_prefix.size() > 0 && asset_prefix.at(0) == '/') {
        out.append_view(&asset_prefix)
        out.append('/')
        return out
    }
    var i : ubigint = 0
    while(i < target.size()) {
        if(target.at(i) == '/') {
            out.append_view("../")
        }
        i = i + 1
    }
    out.append_view(&asset_prefix)
    out.append('/')
    return out
}

// An absolute URL for one exported artifact: `index.html` -> `<site>/`,
// `about/index.html` -> `<site>/about/`, anything else -> `<site>/<path>`.
func sitemap_loc(site_url : &std::string, target : &std::string_view) : std::string {
    var out = std::string()
    var base = site_url.to_view()
    out.append_view(&base)

    const index_html = std::string_view("index.html")
    if(target.equals(&index_html)) {
        out.append('/')
        return out
    }
    const dir_index = std::string_view("/index.html")
    if(target.ends_with(&dir_index)) {
        var dir_len = target.size() - dir_index.size()
        var dir = unsafe(std::string_view(target.data(), dir_len))
        out.append('/')
        out.append_view(&dir)
        out.append('/')
        return out
    }
    out.append('/')
    out.append_view(target)
    return out
}

// `about/index.html` -> `about_index`: the per-page script's file stem.

// True when `<dir>/<name>` exists.
func artifact_exists(dir : std::string_view, name : &std::string_view) : bool {
    var path = std::string()
    path.append_view(&dir)
    path.append('/')
    path.append_view(name)
    var path_view = path.to_view()
    return unsafe(fs::raw::exists(path_view.data()))
}

// Create every directory leading to the file at `path` (not `path` itself: a
// caller passes a file path, and creating a directory with the file's name makes
// the write that follows fail).
func ensure_file_dir(path : std::string_view) {
    var i : ubigint = 0
    var last : ubigint = 0
    while(i < path.size()) {
        if(path.at(i) == '/') { last = i }
        i = i + 1
    }
    if(last == 0) { return }
    var j : ubigint = 1
    while(j <= last) {
        if(j == last || path.at(j) == '/') {
            var prefix = unsafe(std::string_view(path.data(), j))
            var dir = std::string()
            dir.append_view(&prefix)
            var dir_view = dir.to_view()
            unsafe(fs::raw::mkdir(dir_view.data()))
        }
        j = j + 1
    }
}

// Create `path` and every parent directory of it. The parents exist because
// `mkdir` is single-level: `<out>` before the bundle directory under it.
func ensure_dir_path(path : std::string_view) {
    var i : ubigint = 0
    while(i < path.size()) {
        if(path.at(i) == '/') {
            var dir = std::string()
            if(i > 0) {
                var prefix = unsafe(std::string_view(path.data(), i))
                dir.append_view(&prefix)
            }
            var dir_view = dir.to_view()
            unsafe(fs::raw::mkdir(dir_view.data()))
        }
        i = i + 1
    }
    var full = std::string()
    full.append_view(&path)
    var full_view = full.to_view()
    // An existing directory is not an error; a failed create surfaces as a failed
    // artifact write, which `finish` reports.
    unsafe(fs::raw::mkdir(full_view.data()))
}

// Create every parent directory of `path` (relative to `root`), so an artifact
// may live at `blog/post.html` or `assets/app.<hash>.js` without the caller
// preparing directories.
func ensure_parent_dirs(root : std::string_view, path : std::string_view) {
    var i : ubigint = 0
    while(i < path.size()) {
        if(path.at(i) == '/') {
            var dir = std::string()
            dir.append_view(&root)
            dir.append('/')
            if(i > 0) {
                var segment = unsafe(std::string_view(path.data(), i))
                dir.append_view(&segment)
            }
            var dir_view = dir.to_view()
            // An existing directory is not an error; a failed create shows up as a
            // failed artifact write.
            unsafe(fs::raw::mkdir(dir_view.data()))
        }
        i = i + 1
    }
}
