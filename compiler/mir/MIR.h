// Copyright (c) Chemical Language Foundation 2025.
//
// Umbrella include for the MIR core. Pulls in the data structures, module
// container, dump and verifier. Does NOT pull in the builder, lowerer, or any
// backend, and never includes LLVM or AST headers.

#pragma once

#include "MIRTypes.h"
#include "MIRArena.h"
#include "MIRArray.h"
#include "MIRInstruction.h"
#include "MIRMovePath.h"
#include "MIRFunction.h"
#include "MIRTypeTable.h"
#include "MIRConstantPool.h"
#include "MIRSymbolTable.h"
#include "MIRModule.h"
#include "MIRDump.h"
#include "MIRVerifier.h"
