// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include "std/chem_string_view.h"
#include "SubVisitor.h"
#include <vector>
#include "ast/base/ast_fwd.h"

class MembersContainer;
class ASTNode;
class FunctionDeclaration;

enum class DestructionJobType {
    Default,
    Array
};

struct DestructionJob {
    DestructionJobType type;
    chem::string_view self_name;
    std::string drop_flag_name;
    ASTNode* initializer;
    union {
        struct {
            MembersContainer* parent_node;
            FunctionDeclaration* destructor;
            bool is_pointer;
        } default_job;
        struct {
            int array_size;
            MembersContainer* linked;
            FunctionDeclaration* destructorFunc;
        } array_job;
    };
};

class CDestructionVisitor : public SubVisitor {
public:

    using SubVisitor::SubVisitor;

    int loop_job_begin_index = 0;

    bool destroy_current_scope = true;

    bool new_line_before = true;

    std::vector<DestructionJob> destruct_jobs;

    void destruct(
            const chem::string_view& self_name,
            MembersContainer* linked,
            FunctionDeclaration* destructor,
            bool is_pointer
    );

    void conditional_destruct(
            const chem::string_view& condition,
            const chem::string_view& self_name,
            MembersContainer* linked,
            FunctionDeclaration* destructor,
            bool is_pointer
    );

    void queue_destruct(
            const chem::string_view& self_name,
            ASTNode* initializer,
            MembersContainer* linked,
            bool is_pointer = false,
            bool has_drop_flag = true
    );

    std::string* get_drop_flag_name(ASTNode* initializer) {
        for(auto& d : destruct_jobs) {
            if(d.initializer == initializer) {
                return &d.drop_flag_name;
            }
        }
        return nullptr;
    }

    void queue_destruct(const chem::string_view& self_name, ASTNode* initializer, FunctionCall* call);

    void destruct_arr_ptr(const chem::string_view& self_name, Value* array_size, MembersContainer* linked, FunctionDeclaration* destructor);

    void destruct_arr(const chem::string_view& self_name, int array_size, MembersContainer* linked, FunctionDeclaration* destructor);

    void destruct(const DestructionJob& job, Value* current_return);

    bool queue_destruct_arr(const chem::string_view& self_name, ASTNode* initializer, BaseType* elem_type, int array_size);

    void queue_destruct_varInit_type(BaseType* type, ASTNode* initializer, const chem::string_view& self_name);

    void VisitVarInitStmt(VarInitStatement *init);

    void dispatch_jobs_from_no_clean(int begin);

    void dispatch_jobs_from(int begin);

    void queue_destruct_type(const chem::string_view& self_name, ASTNode* initializer, BaseType* type);

    void queue_destruct_decl_params(FunctionType* decl);

    void process_init_value(VarInitStatement *init, Value* value);

    void reset() final {
        destroy_current_scope = true;
        new_line_before = true;
        destruct_jobs.clear();
    }

};