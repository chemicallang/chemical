// Copyright (c) Chemical Language Foundation 2025.

#include "GenericInstIdentifier.h"

GenericInstIdentifier* GenericInstIdentifier::copy(ASTAllocator &allocator) {
    std::vector<TypeLoc> args;
    args.reserve(generic_list.size());
    for(const auto& arg : generic_list) {
        args.emplace_back(arg.copy(allocator));
    }
    return new (allocator.allocate<GenericInstIdentifier>()) GenericInstIdentifier(
        identifier->copy(allocator),
        std::move(args)
    );
}
