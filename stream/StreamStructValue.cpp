// Copyright (c) Qinetik 2024.

#include "StreamStructValue.h"
#include "ast/utils/InterpretValues.h"
#include "ast/values/IntValue.h"
#include "ast/values/CharValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/StringValue.h"

void define_source_stream_fns(GlobalInterpretScope &global) {
    auto globalVec = global.global_vals.find("stream");
    if (globalVec == global.global_vals.end()) {
        // defining vector member function values
        std::unordered_map<std::string, MemValueFn> member_fns;
        member_fns["currentPosition"] = [&](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            return new IntValue(static_cast<StreamStructValue *>(value)->provider.currentPosition());
        };
        member_fns["readCharacter"] = [&](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            return new CharValue(static_cast<StreamStructValue *>(value)->provider.readCharacter());
        };
        member_fns["eof"] = [&](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            return new BoolValue(static_cast<StreamStructValue *>(value)->provider.eof());
        };
        member_fns["peek"] = [&](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            if (params.size() == 0) {
                return new CharValue(static_cast<StreamStructValue *>(value)->provider.peek());
            }
            if (params.size() == 1 && params[0]->value_type() == ValueType::Int) {
                return new CharValue(static_cast<StreamStructValue *>(value)->provider.peek(params[0]->as_int()));
            }
            scope.error("peek can take a single optional parameter of type integer");
            return nullptr;
        };
        member_fns["readUntil"] = [&](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            if (params.size() == 1 && params[0]->value_type() == ValueType::Char) {
                return new StringValue(
                        static_cast<StreamStructValue *>(value)->provider.readUntil(params[0]->as_char()));
            }
            scope.error("readUntil requires a single parameter of type character");
            return nullptr;
        };
        member_fns["increment"] = [&](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            if (params.size() == 1 && params[0]->value_type() == ValueType::Char) {
                static_cast<StreamStructValue *>(value)->provider.increment(params[0]->as_char());
                return nullptr;
            }
            if (params.size() >= 1 && params[0]->value_type() == ValueType::String) {
                static_cast<StreamStructValue *>(value)->provider.increment(
                        params[0]->as_string(),
                        (params.size() > 1 && params[1]->value_type() == ValueType::Bool) && (params[1]->as_bool())
                );
                return nullptr;
            }
            scope.error("increment requires a single parameter of type character / string");
            return nullptr;
        };
        member_fns["getLineNumber"] = [&](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            return new IntValue(static_cast<StreamStructValue *>(value)->provider.getLineNumber());
        };
        member_fns["getLineCharNumber"] = [&](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            return new IntValue(static_cast<StreamStructValue *>(value)->provider.getLineCharNumber());
        };
        member_fns["readAlpha"] = [&](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            return new StringValue(static_cast<StreamStructValue *>(value)->provider.readAlpha());
        };
        member_fns["readNumber"] = [&](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            return new StringValue(static_cast<StreamStructValue *>(value)->provider.readNumber());
        };
        member_fns["readAlphaNum"] = [&](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            return new StringValue(static_cast<StreamStructValue *>(value)->provider.readAlphaNum());
        };
        member_fns["readIdentifier"] = [&](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            return new StringValue(static_cast<StreamStructValue *>(value)->provider.readIdentifier());
        };
        member_fns["readWhitespaces"] = [&](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            return new IntValue(static_cast<StreamStructValue *>(value)->provider.readWhitespaces());
        };
        member_fns["hasNewLine"] = [&](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            return new BoolValue(static_cast<StreamStructValue *>(value)->provider.hasNewLine());
        };
        member_fns["readNewLineChars"] = [&](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            return new BoolValue(static_cast<StreamStructValue *>(value)->provider.readNewLineChars());
        };
        global.global_vals["stream"] = std::make_unique<MemberFnsValue>(std::move(member_fns));
    }
}