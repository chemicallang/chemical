// Copyright (c) Qinetik 2024.

#include "ast/base/ASTNode.h"

class TryCatch : ASTNode {
public:

    TryCatch();

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

    void accept(Visitor &visitor) override;

    std::string representation() const override;

};