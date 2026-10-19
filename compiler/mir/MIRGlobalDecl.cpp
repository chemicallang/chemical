// Copyright (c) Chemical Language Foundation 2025.
//
// Shared global-declaration spelling — see MIRGlobalDecl.h.

#include "MIRGlobalDecl.h"

#include "MIRModule.h"
#include "MIRTypeBuilder.h"

#include "ast/base/BaseType.h"
#include "ast/types/ArrayType.h"
#include "ast/statements/VarInit.h"
#include "compiler/mangler/NameMangler.h"
#include "preprocess/2c/BufferedWriter.h"

namespace mir {

void build_global_declaration_type(MIRTypeBuilder& types, VarInitStatement* init,
                                   MIRGlobalDecl& out) {
    out = MIRGlobalDecl{};
    BaseType* t = init->type ? init->type : (init->value ? init->value->getType() : nullptr);
    if (t) {
        if (t->kind() == BaseTypeKind::Array && !t->as_array_type()->has_array_size()) {
            // an unsized array (`char[]`) is declared `T name[]`; MIR maps it to
            // a pointer for decay, which is wrong for a declaration
            out.unsized_array = true;
            out.elem = types.map(t->as_array_type()->known_child_type());
        } else {
            out.type = types.map(t);
        }
    }
    if (init->is_thread_local()) out.flags |= MGD_THREAD_LOCAL;
    if (!init->is_linkage_public()) out.flags |= MGD_STATIC;
    if (init->is_volatile()) out.flags |= MGD_VOLATILE;
}

void build_global_declaration(NameMangler& mangler, MIRTypeBuilder& types, MIRModule& module,
                              VarInitStatement* init, MIRGlobalDecl& out) {
    (void)module;
    build_global_declaration_type(types, init, out);
    {
        BufferedWriter w(256);
        mangler.mangle(w, init);
        out.name.assign(w.data(), w.getPosition());
    }
}

} // namespace mir
