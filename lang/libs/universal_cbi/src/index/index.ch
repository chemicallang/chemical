// Per-job ambient index for the universal component body checker (Phase 2).
//
// `JobBegin` clears everything and records which side of a `--web` build this
// job is. `IndexModule` appends every public function as an ambient entry. A
// zero-parameter function's entry carries its return type (`document()` ->
// `Document`), which the body checker uses for direct member lookup; a function
// with parameters (`fetch`) still gets an entry so the closed host-name list
// resolves, but its `type` is null and no member lookup is implied. The index
// lives for exactly one job: no entry survives into the next server/client pass.
//
// Output is gated by CHEM_DEBUG_INDEX so normal builds stay silent.

public enum AmbientKind {
    Function
}

public struct AmbientEntry {
    var kind : AmbientKind
    var name : std::string
    var provider : std::string_view
    var type : *mut BaseType
}

public struct AmbientConflict {
    var name : std::string
    var first_provider : std::string
    var second_provider : std::string
}

@no_drop
public struct Index {
    var ambient : std::unordered_map<std::string, AmbientEntry>
    var conflicts : std::vector<AmbientConflict>
}

@never_destructed
var universal_index : Index
var universal_index_ready : bool = false
var universal_is_client_job : bool = false

func universal_index_debug_enabled() : bool {
        unsafe {
        return getenv("CHEM_DEBUG_INDEX\0" as *char) != null
        }
    }

func index_string(view : &std::string_view) : std::string {
        unsafe {
        var out = std::string()
        out.append_view(view)
        return out

        }
}

// Null means "no indexed provider". `*mut AmbientEntry` points into the map
// node, which is stable until the next `JobBegin` clears the index.
public func universal_index_lookup(name : &std::string_view) : *mut AmbientEntry {
        unsafe {
        var key = index_string(name)
        return universal_index.ambient.get_ptr(&key)

        }
}

public func universal_index_conflict(name : &std::string_view) : *mut AmbientConflict {
        unsafe {
        for(var i : uint = 0; i < universal_index.conflicts.size(); i++) {
            const conflict = universal_index.conflicts.get_ptr(i)
            if(conflict.name.equals_view(name)) {
                return conflict
            }
        }
        return null

        }
}

func index_add_ambient(name : &std::string_view, provider : &std::string_view, type : *mut BaseType) {
        unsafe {
        var key = index_string(name)
        const existing = universal_index.ambient.get_ptr(&key)
        if(existing != null) {
            var conflict = AmbientConflict {
                name : std::string(),
                first_provider : std::string(),
                second_provider : std::string()
            }
            conflict.name.append_view(name)
            conflict.first_provider.append_view(&existing.provider)
            conflict.second_provider.append_view(provider)
            universal_index.conflicts.push(conflict)
            return
        }

        var entry = AmbientEntry {
            kind : AmbientKind.Function,
            name : std::string(),
            provider : *provider,
            type : type
        }
        entry.name.append_view(name)
        universal_index.ambient.insert(key, entry)

        }
    }

@no_mangle
public unsafe func universal_job_begin(job : *mut LabJob) {
        unsafe {
        if(job == null) { return }
        // A CBI job builds a plugin rather than user modules. It has no component
        // bodies to index or check, and resetting the plugin's own state while the
        // job is rebuilding that plugin is unsafe, so it is not a job boundary.
        if(job.getType() == LabJobType.CBI) { return }

        // Nothing survives across jobs. The maps are constructed once (globals
        // are zeroed) and then emptied, which releases the previous job's nodes
        // and owned strings without depending on an unload-time global destructor.
        if(!universal_index_ready) {
            universal_index = Index {
                ambient : std::unordered_map<std::string, AmbientEntry>(),
                conflicts : std::vector<AmbientConflict>()
            }
            universal_index_ready = true
        } else {
            universal_index.ambient.clear()
            universal_index.conflicts.clear()
        }

        var client : int = 0
        var js : int = 0
        const target = job.getTarget()
        if(target.client) { client = 1 }
        if(target.js) { js = 1 }
        universal_is_client_job = (client == 1) || (js == 1)

        if(universal_index_debug_enabled()) {
            const name = job.getName()
            printf("[index] universal_job_begin job=%.*s type=%d client=%d js=%d ambient=%d\n",
                name.size() as int, name.data(), job.getType() as int,
                client, js, universal_index.ambient.size() as int)
        }

        }
    }

@no_mangle
public unsafe func universal_index_module(module : *mut Module, declarations : *mut VecRef<ASTNode>) {
        unsafe {
        const debug = universal_index_debug_enabled()
        const name = module.getName()
        const count = declarations.size()
        const provider = name

        if(debug) {
            printf("[index] universal_index_module module=%.*s count=%d decls=", name.size() as int, name.data(), count as int)
        }

        var i : size_t = 0
        while(i < count) {
            const node = declarations.get(i as uint)
            switch(node.getKind()) {
                ASTNodeKind.StructDecl => {
                    if(debug) {
                        printf("StructDecl:")
                        const def = node as *mut StructDefinition
                        const decl_name = def.getName()
                        printf("%.*s", decl_name.size() as int, decl_name.data())
                    }
                }
                ASTNodeKind.VariantDecl => {
                    if(debug) {
                        printf("VariantDecl:")
                        const def = node as *mut VariantDefinition
                        const decl_name = def.getName()
                        printf("%.*s", decl_name.size() as int, decl_name.data())
                    }
                }
                ASTNodeKind.FunctionDecl => {
                    const def = node as *mut FunctionDeclaration
                    const decl_name = def.getName()
                    if(debug) {
                        printf("FunctionDecl:")
                        printf("%.*s", decl_name.size() as int, decl_name.data())
                    }

                    // Every public function is indexed. The participant has
                    // already filtered internal declarations and recursed public
                    // namespaces. A zero-parameter function is an ambient root
                    // whose return type drives member lookup; a function with
                    // parameters only makes the name resolvable (so e.g. `fetch`
                    // is not reported as an unknown host), with a null type.
                    const params = def.get_params()
                    var entry_type : *mut BaseType = null
                    if(params == null || params.size() == 0) {
                        entry_type = def.getReturnType()
                    }
                    index_add_ambient(&decl_name, &provider, entry_type)
                }
                default => {
                    if(debug) {
                        printf("Unknown")
                    }
                }
            }
            if(debug && i + 1 < count) {
                printf(",")
            }
            i = i + 1
        }
        if(debug) {
            printf(" ambient=%d\n", universal_index.ambient.size() as int)
        }

        }
    }
