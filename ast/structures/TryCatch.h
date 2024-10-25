// Copyright (c) Qinetik 2024.

#include "ast/base/ASTNode.h"
#include "Scope.h"
#include "ast/base/BaseType.h"
#include <optional>

using catch_var_type = std::optional<std::pair<std::string, std::unique_ptr<BaseType>>>;

class TryCatch : public ASTNode {
public:

    std::unique_ptr<FunctionCall> tryCall;
    catch_var_type catchVar;
    std::optional<Scope> catchScope;
    ASTNode* parent_node;
    CSTToken* token;

    TryCatch(
            std::unique_ptr<FunctionCall> tryCall,
            catch_var_type catchVar,
            std::optional<Scope> catchScope,
            ASTNode* parent_node,
            CSTToken* token
    );

    ASTNodeKind kind() final {
        return ASTNodeKind::TryStmt;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    CSTToken *cst_token() final {
        return token;
    }

    ASTNode *parent() final {
        return parent_node;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

    void declare_and_link(SymbolResolver &linker) final;

    void accept(Visitor *visitor) final;

};