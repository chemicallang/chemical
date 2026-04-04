#pragma once

#include <unordered_map>
#include <string>
#include <string_view>
#include "chem_string_view.h"

namespace util {

    inline std::string_view to_sv(std::string_view v) noexcept { return v; }
    inline std::string_view to_sv(const std::string& v) noexcept { return v; }
    inline std::string_view to_sv(const chem::string_view& v) noexcept { return {v.data(), v.size()}; }

    // Transparent hash for string-like types
    struct string_hash {
        using is_transparent = void;

        size_t operator()(const std::string_view& v) const noexcept {
            return std::hash<std::string_view>{}(v);
        }

        size_t operator()(const chem::string_view& v) const noexcept {
            return std::hash<std::string_view>{}(to_sv(v));
        }
        size_t operator()(const std::string& v) const noexcept {
            return std::hash<std::string_view>{}(v);
        }
    };

    // Transparent equality
    struct string_equal {
        using is_transparent = void;
        template <class A, class B>
        bool operator()(const A& a, const B& b) const noexcept {
            return to_sv(a) == to_sv(b);
        }
    };

    // Drop-in unordered_map replacement
    template <
        class T,
        class Allocator = std::allocator<std::pair<const std::string, T>>
    >
    using unordered_string_map =
        std::unordered_map<std::string, T, string_hash, string_equal, Allocator>;

}