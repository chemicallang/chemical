// Copyright (c) Qinetik 2024.


#include "ast/base/ASTNode.h"

class SwitchStatement : ASTNode {
public:

    SwitchStatement();

    void accept(Visitor &visitor) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

    std::string representation() const override;

};