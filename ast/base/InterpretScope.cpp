// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "InterpretScope.h"
#include "GlobalInterpretScope.h"

InterpretScope::InterpretScope(GlobalInterpretScope* global) : global(global) {

}

void InterpretScope::error(const std::string& err) {
    global->add_error(err);
}