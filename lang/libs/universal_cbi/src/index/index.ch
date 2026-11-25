// Phase 1 probe for the generic IndexModule / JobBegin CBI hooks.
//
// The real browser-surface index lands in Phase 2. This file only proves the
// compiler dispatch is wired: JobBegin runs once per job and IndexModule runs
// once per marked module, with the module's public top-level declarations.
// Output is gated by CHEM_DEBUG_INDEX so normal builds stay silent.

func universal_index_debug_enabled() : bool {
        unsafe {
        return getenv("CHEM_DEBUG_INDEX\0" as *char) != null
        }
    }

@no_mangle
public unsafe func universal_job_begin(job : *mut LabJob) {
        unsafe {
        if(!universal_index_debug_enabled()) {
            return
        }
        const name = job.getName()
        printf("[index] universal_job_begin job=%.*s type=%d\n", name.size() as int, name.data(), job.getType() as int)
        }
    }

@no_mangle
public unsafe func universal_index_module(module : *mut Module, declarations : *mut VecRef<ASTNode>) {
        unsafe {
        if(!universal_index_debug_enabled()) {
            return
        }
        const name = module.getName()
        const count = declarations.size()
        printf("[index] universal_index_module module=%.*s count=%d decls=", name.size() as int, name.data(), count as int)
        var i : size_t = 0
        while(i < count) {
            if(i != 0) {
                printf(",")
            }
            const node = declarations.get(i as uint)
            switch(node.getKind()) {
                ASTNodeKind.StructDecl => {
                    printf("StructDecl:")
                    const def = node as *mut StructDefinition
                    const decl_name = def.getName()
                    printf("%.*s", decl_name.size() as int, decl_name.data())
                }
                ASTNodeKind.VariantDecl => {
                    printf("VariantDecl:")
                    const def = node as *mut VariantDefinition
                    const decl_name = def.getName()
                    printf("%.*s", decl_name.size() as int, decl_name.data())
                }
                ASTNodeKind.FunctionDecl => {
                    printf("FunctionDecl:")
                    const def = node as *mut FunctionDeclaration
                    const decl_name = def.getName()
                    printf("%.*s", decl_name.size() as int, decl_name.data())
                }
                default => {
                    printf("Unknown")
                }
            }
            i = i + 1
        }
        printf("\n")
        }
    }
