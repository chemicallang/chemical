#!/usr/bin/env bash
# Boundary audit for component body typing (workflow H12).
#
# The feature lives in the plugin (`lang/libs/universal_cbi`) and the provider
# (`lang/libs/dom`). The compiler only supplies generic hooks and metadata. This
# gate fails if that boundary is crossed:
#
#   * no component-typing vocabulary in `compiler/` (WEB-BODY-*, universal_index,
#     universal_job_begin, body_check);
#   * no `"dom"` / `"universal"` string literal in `compiler/` code (the
#     pre-existing JS host ABI table spells `chx_dom_*` symbols, not a module
#     name, and is reported informationally below);
#   * CBIFunctionType stays in sync in C++ and Chemical;
#   * the generic CBI hooks are declared, dispatched and registered;
#   * the code generators do not read the AST (only MIR);
#   * the CBI accessors this feature added are present.
#
#   ./scripts/component-typing-audit.sh
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

fail=0
check() {
    if [ "$2" = "1" ]; then
        echo "ok   $1"
    else
        echo "FAIL $1" >&2
        fail=1
    fi
}

# ---------------------------------------------------------------- compiler vocabulary
echo "== compiler boundary"
typing_vocab=$(grep -rnE 'WEB-BODY-|universal_index|universal_job_begin|component_body_check|body_check' \
    compiler --include='*.h' --include='*.cpp' 2>/dev/null || true)
if [ -n "$typing_vocab" ]; then
    echo "$typing_vocab" >&2
    check "no component-typing vocabulary in compiler/" 0
else
    check "no component-typing vocabulary in compiler/" 1
fi

# A module-name literal is what would special-case the provider. Existing JS
# host ABI strings are `chx_dom_*`; they do not match these exact literals.
module_literals=$(grep -rnE '"(dom|universal)"' compiler --include='*.h' --include='*.cpp' 2>/dev/null || true)
if [ -n "$module_literals" ]; then
    echo "$module_literals" >&2
    check "no \"dom\"/\"universal\" module-name literal in compiler/" 0
else
    check "no \"dom\"/\"universal\" module-name literal in compiler/" 1
fi

plugin_includes=$(grep -rnE '#include[[:space:]]+"[^"]*(lang/libs/(dom|universal_cbi)|(^|/)dom/src)' \
    compiler --include='*.h' --include='*.cpp' 2>/dev/null || true)
if [ -n "$plugin_includes" ]; then
    echo "$plugin_includes" >&2
    check "compiler does not include provider/plugin sources" 0
else
    check "compiler does not include provider/plugin sources" 1
fi

host_abi_refs=$(grep -rniE '\bdom\b|\buniversal\b' compiler --include='*.h' --include='*.cpp' 2>/dev/null | wc -l | tr -d ' ')
echo "info: $host_abi_refs pre-existing host-ABI/comment references remain (chx_dom_* mapping, client-only refusal, #universal macro comments)"

# ---------------------------------------------------------------- enum sync
echo
echo "== CBI contract"
if ./scripts/check-cbi-enum-sync.sh > /tmp/component_typing_enum_sync.log 2>&1; then
    check "CBIFunctionType is in sync (C++/Chemical)" 1
else
    cat /tmp/component_typing_enum_sync.log >&2
    check "CBIFunctionType is in sync (C++/Chemical)" 0
fi

check "IndexModule is in both enums" \
      "$(grep -qE '[[:space:]]IndexModule,' compiler/cbi/model/CBIFunctionType.h \
          && grep -qE '[[:space:]]IndexModule,' lang/libs/lab/src/lab.ch && echo 1 || echo 0)"
check "JobBegin is in both enums" \
      "$(grep -qE '[[:space:]]JobBegin' compiler/cbi/model/CBIFunctionType.h \
          && grep -qE '[[:space:]]JobBegin' lang/libs/lab/src/lab.ch && echo 1 || echo 0)"
check "JobBegin/IndexModule typedefs exist" \
      "$(grep -q 'JobBeginFunc' compiler/cbi/model/Model.h \
          && grep -q 'IndexModuleFunc' compiler/cbi/model/Model.h && echo 1 || echo 0)"
check "the hooks are dispatched by the host" \
      "$(grep -q 'CBIFunctionType::JobBegin' compiler/lab/LabBuildCompiler.cpp \
          && grep -q 'CBIFunctionType::IndexModule' compiler/ASTProcessor.cpp && echo 1 || echo 0)"
check "universal_cbi registers both hooks" \
      "$(grep -q 'CBIFunctionType.JobBegin' lang/libs/universal_cbi/build.lab \
          && grep -q 'CBIFunctionType.IndexModule' lang/libs/universal_cbi/build.lab && echo 1 || echo 0)"
check "the declaration-collection helper exists" \
      "$(grep -q 'collect_public_declarations' compiler/ASTProcessor.cpp && echo 1 || echo 0)"

# ---------------------------------------------------------------- generic CBI accessor
check "ASTNodegetNodeIdentifier is bound in C++" \
      "$(grep -q 'ASTNodegetNodeIdentifier' compiler/cbi/bindings/ASTCBI.h \
          && grep -q 'ASTNodegetNodeIdentifier' compiler/cbi/bindings/ASTBuilderCBI.cpp \
          && grep -q 'compiler_ASTNodegetNodeIdentifier' compiler/cbi/bindings/CBI.cpp && echo 1 || echo 0)"
check "ASTNode.getNodeIdentifier is available to plugins" \
      "$(grep -q 'getNodeIdentifier' lang/libs/compiler/src/ast/base/ASTNode.ch && echo 1 || echo 0)"

# ---------------------------------------------------------------- backends stay MIR-only
echo
echo "== backends work from MIR"
ast_reads=""
for backend in compiler/mir/MIREmitter.cpp compiler/mir/MIREmitterJs.cpp compiler/CodegenMir.cpp; do
    if [ ! -f "$backend" ]; then
        continue
    fi
    hit=$(grep -nE '#include[[:space:]]+"ast/|ASTNode|BaseType|StructDefinition|FunctionDeclaration|ASTAllocator|BaseDefMember' "$backend" 2>/dev/null || true)
    if [ -n "$hit" ]; then
        ast_reads="$ast_reads\n$backend:\n$hit"
    fi
done
if [ -n "$ast_reads" ]; then
    printf '%b\n' "$ast_reads" >&2
    check "no AST reads in the C/LLVM/JS code generators" 0
else
    check "no AST reads in the C/LLVM/JS code generators" 1
fi

if [ "$fail" != 0 ]; then
    echo "component-typing-audit: FAILED" >&2
    exit 1
fi
echo "component-typing-audit: compiler stays generic, hooks/bindings present, backends read MIR only"
