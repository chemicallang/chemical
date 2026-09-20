// Copyright (c) Chemical Language Foundation 2025.

#include "GenericInstIdentifier.h"

GenericInstIdentifier* GenericInstIdentifier::copy(ASTAllocator &allocator) {
    std::vector<TypeLoc> args;
    args.reserve(generic_list.size());
    for(const auto& arg : generic_list) {
        args.emplace_back(arg.copy(allocator));
    }
    const auto view = allocator.allocate_str(value.data(), value.size());
    auto id = new (allocator.allocate<GenericInstIdentifier>()) GenericInstIdentifier(
        chem::string_view(view, value.size()),
        getType(),
        encoded_location(),
        is_ns,
        std::move(args)
    );
    id->linked = linked;
    id->is_moved = is_moved;
    return id;
}
