// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/values/VariableIdentifier.h"
#include "ast/base/TypeLoc.h"
#include <vector>

/**
 * a bare generic function reference used as a value, with explicit generic
 * arguments. It is written `ident<int>` or `ns::ident<int>` and appears as the
 * last value of an access chain (or as a lone value when the chain is singlified).
 *
 * It IS a VariableIdentifier: it only adds the generic argument list on top of the
 * identifier state, so isIdentifier() reports true, as_identifier() /
 * as_identifier_unsafe() return it, and every identifier method (linked_node,
 * byte_size, llvm_value, evaluated_value, ...) applies to it unchanged. Only the
 * arguments are stored on this node, so a plain identifier does not pay for them.
 *
 * Symbol resolution links this identifier to the generic declaration, then
 * instantiates the function with these arguments and relinks it to the resulting
 * concrete FunctionDeclaration. The arguments are dead after that.
 */
class GenericInstIdentifier : public VariableIdentifier {
public:

    /**
     * generic arguments the referenced function is instantiated with
     */
    std::vector<TypeLoc> generic_list;

    GenericInstIdentifier(
        chem::string_view value,
        BaseType* type,
        SourceLocation location,
        bool is_ns,
        std::vector<TypeLoc> generic_list
    ) : VariableIdentifier(
            ValueKind::GenericInstIdentifier,
            value,
            type,
            location,
            is_ns
        ),
        generic_list(std::move(generic_list)) {

    }

    GenericInstIdentifier* copy(ASTAllocator& allocator) override;

};
