// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/MacroToken.h"
#include "lexer/model/tokens/RawToken.h"
#include "ast/values/StructValue.h"
#include "stream/StreamStructValue.h"
#include "lexer/model/tokens/UserToken.h"

Value* extract_child(InterpretScope& scope, StructValue* value, const std::string& name, ValueType type, Lexer& lexer) {
    auto child = value->child(scope, name);
    if(child == nullptr) {
        lexer.error("required value \"" + name + "\" not found in the struct");
    } else if(child->value_type() != type) {
        lexer.error("value \"" + name + "\" is a different type, expected " + to_string(type) + ", got " + to_string(child->value_type()));
        return nullptr;
    }
    return child;
}

LexUserToken* extract_token(InterpretScope& scope, StructValue* value, Lexer& lexer) {
    auto line = extract_child(scope, value, "line", ValueType::Int, lexer);
    auto character = extract_child(scope, value, "character", ValueType::Int, lexer);
    auto length = extract_child(scope, value, "length", ValueType::Int, lexer);
    if(line == nullptr || character == nullptr || length == nullptr) {
        return nullptr;
    }
    return new LexUserToken(UserToken(line->as_int(), character->as_int(), length->as_int()));
}

void extract_user_tokens(InterpretVectorValue* list, Lexer& lexer) {
    for(auto& value : list->values) {
        if(value->value_type() == ValueType::Struct) {
            auto token = extract_token(lexer.interpret_scope, value->as_struct(), lexer);
            if(token != nullptr) { lexer.tokens.emplace_back(token); }
        } else {
            lexer.error("given vector contains a value that is not a struct, with representation " + value->representation());
        }
    }
}

bool Lexer::lexAnnotationMacro() {
    if(provider.peek() == '@' || provider.peek() == '#') {
        auto isAnnotation = provider.peek() == '@';
        provider.readCharacter();
        auto macro = lexIdentifier();
#ifdef LSP_BUILD
        SemanticTokenModifier modifier;
        if(macro == "deprecated") {
            modifier = SemanticTokenModifier::ls_deprecated;
        } else if(macro == "readonly") {
            modifier = SemanticTokenModifier::ls_readonly;
        } else {
            modifier = SemanticTokenModifier::LastModifier;
        }
        tokens.emplace_back(std::make_unique<MacroToken>(backPosition(macro.size() + 1), std::move(macro), isAnnotation, modifier));
#else
        tokens.emplace_back(std::make_unique<MacroToken>(backPosition(macro.size() + 1), macro, isAnnotation));
#endif
        if(isAnnotation) {
            auto found = annotation_modifiers.find(macro);
            if(found != annotation_modifiers.end()) {
                found->second(this);
            } else {
                info("unknown annotation at lexer level : " + macro + " next char :" + provider.peek());
            }
        } else {

            if(macro.starts_with("end")) {
                error("processing ending macro as starting point, macro cannot be named starting with 'end' like " + macro);
                return true;
            }

            std::string ending = "#end" + macro;

            // check if this macro has a lexer fined
            auto lex_struct = this->lexer_structs.find(macro);
            if(lex_struct != this->lexer_structs.end()) {

                // interpret the struct in the interpret scope, so struct value can find it when initializing
                lex_struct->second->interpret(interpret_scope);

                // get the function
                auto fn = lex_struct->second->member("lex");
                if(fn == nullptr) {

                    error("struct doesn't contain a function named lex");

                } else {

                    // set the declaration scope for the function to be interpreted in
                    fn->declarationScope = &interpret_scope;

                    // create the instance of the struct
                    auto lex_struct_value = std::make_unique<StructValue>(macro,
                                                                          std::vector<std::pair<std::string, std::unique_ptr<Value>>>());
                    lex_struct_value->definition = lex_struct->second.get();

                    // defining function params, containing the stream source
                    std::vector<std::unique_ptr<Value>> params(1);
                    auto &members = static_cast<MemberFnsValue *>(interpret_scope.global_vals["stream"].get())->members;
                    params[0] = std::make_unique<StreamStructValue>(provider, members);

                    // calling the member function lex and getting the tokens struct
                    auto tokens_value = lex_struct_value->call_member(interpret_scope, "lex", params);
                    for(const auto& err  : interpret_scope.errors) {
                        error("lex call error " + err);
                    }
                    if (tokens_value == nullptr) {
                        error("received no tokens struct value from lex member function of " + macro);
                    } else if (tokens_value->value_type() != ValueType::Vector) {
                        error("received a value that is not a vector from lex member function of " + macro + ", value : " + tokens_value->representation());
                        delete tokens_value;
                    } else {
                        auto vec = tokens_value->as_vector();
                        extract_user_tokens(vec, *this);
                        delete vec;
                    }
                    interpret_scope.clean();

                }

            }

            lexWhitespaceAndNewLines();

            auto before_ending = position();
            if(provider.increment(ending)) {
                tokens.emplace_back(std::make_unique<MacroToken>(before_ending, "end" + macro, false, true));
            } else {
                auto current = position();
                auto content = provider.readUntil(ending, false);
                tokens.emplace_back(std::make_unique<RawToken>(current, std::move(content)));
                if(!provider.increment(ending)) {
                    error("expected ending macro with " + ending);
                } else {
                    tokens.emplace_back(std::make_unique<MacroToken>(before_ending, "end" + macro, false, true));
                }
            }
        }
        return true;
    } else {
        return false;
    }
}