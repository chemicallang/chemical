// Copyright (c) Chemical Language Foundation 2025.

#include "InterfaceDefinition.h"
#include "StructDefinition.h"
#include "ImplDefinition.h"
#include "StructMember.h"
#include "compiler/mangler/NameMangler.h"
#include "ast/types/LinkedType.h"
#include <sstream>
#include "std/except.h"


uint64_t InterfaceDefinition::byte_size(const TargetData& target) {
    // a single pointer
    // this is required, because well user can write types like std::Result<ArrayDecoder, SerializationError>
    // here, ArrayDecoder is an interface, so we (pretend to) store it as a pointer
    return target.is64Bit ? 8 : 4;
}

