// Copyright (c) Qinetik 2024.

#include "ast/base/ASTNode.h"
#include "Scope.h"
#include "ast/base/BaseType.h"
#include <optional>

using catch_var_type = std::optional<std::pair<std::string, std::unique_ptr<BaseType>>>;

class TryCatch : ASTNode {
public:

    Scope tryScope;
    catch_var_type catchVar;
    std::optional<Scope> catchScope;

    TryCatch(Scope tryScope, catch_var_type catchVar, std::optional<Scope> catchScope);

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

    void accept(Visitor &visitor) override;

    std::string representation() const override;

};