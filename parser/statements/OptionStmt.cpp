// Copyright (c) Chemical Language Foundation 2025.

#include "parser/Parser.h"
#include "compiler/ModuleOptionRegistry.h"
#include <cstdlib>

/**
 * Parses: option <key> = <value> ;
 *
 * <key>    can be a dotted path: "checks.bounds"
 * <value>  can be a boolean (true/false), integer (42), or string ("enforce")
 *
 * Validates against the global option registry during parsing.
 */
bool BasicParser::parseOptionStmt(ASTAllocator& allocator, ModuleFileData& data) {
    // "option" has already been consumed by the caller

    // ---- Parse the key path (identifiers separated by dots) ----
    auto first_id = consumeIdentifierOrKeyword();
    if (!first_id) {
        error() << "expected an option key after 'option'";
        return false;
    }

    // Build the key as a single string joining dot-separated identifiers
    // We use a small stack buffer + potential allocation
    chem::string key_buf;
    key_buf.append(first_id->value);

    while (consumeToken(TokenType::DotSym)) {
        key_buf.append('.');
        auto next_id = consumeIdentifierOrKeyword();
        if (!next_id) {
            error() << "expected identifier after '.' in option key";
            return false;
        }
        key_buf.append(next_id->value);
    }

    // Store the key on the allocator
    auto key = allocate_view(allocator, key_buf.to_chem_view());

    if (!consumeToken(TokenType::EqualSym)) {
        error() << "expected '=' after option key '" << key << "'";
        return false;
    }

    // ---- Parse the typed value ----
    ModOptionValue parsed_value;
    SourceLocation value_location = 0;

    switch (token->type) {
        case TokenType::TrueKw: {
            parsed_value = ModOptionValue::make_bool(true);
            value_location = loc_single(token);
            token++;
            break;
        }
        case TokenType::FalseKw: {
            parsed_value = ModOptionValue::make_bool(false);
            value_location = loc_single(token);
            token++;
            break;
        }
        case TokenType::Number: {
            // Parse as 64-bit integer
            char* end = nullptr;
            int64_t val = std::strtoll(token->value.data(), &end, 0);
            if (end == token->value.data() || *end != '\0') {
                // Could be a float, try double
                char* end_f = nullptr;
                double fval = std::strtod(token->value.data(), &end_f);
                if (end_f == token->value.data()) {
                    error() << "invalid numeric value for option";
                    return false;
                }
                // It parsed as a float
                parsed_value = ModOptionValue::make_float(fval);
            } else {
                parsed_value = ModOptionValue::make_int(val);
            }
            value_location = loc_single(token);
            token++;
            break;
        }
        case TokenType::String:
        case TokenType::MultilineString: {
            auto str = parseString(allocator);
            if (!str.has_value()) {
                error() << "expected a string value for option";
                return false;
            }
            parsed_value = ModOptionValue::make_string(str.value());
            value_location = loc_single(token);
            break;
        }
        default:
            error() << "unexpected value type for option (expected boolean, integer, float, or string)";
            return false;
    }

    // ---- Validate against the option registry ----
    const auto& registry = get_option_registry();
    auto it = registry.find(key);
    if (it == registry.end()) {
        error() << "unknown option '" << key << "'";
        return false;
    }

    const auto& desc = it->second;

    // Check value kind matches
    if (!value_kind_matches(desc.value_kind, parsed_value.kind)) {
        error() << "option '" << key << "' expects a " << option_value_kind_name(desc.value_kind)
                << " value, got a different type";
        return false;
    }

    // Check allowed values
    if (!validate_allowed(desc, parsed_value)) {
        error() << "value is not valid for option '" << key << "'";
        return false;
    }

    // ---- Store the validated option ----
    data.options.push_back(ModFileOption {
        .key = key,
        .value = parsed_value,
        .location = value_location
    });

    return true;
}
