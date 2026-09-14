// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include <vector>
#include <unordered_map>
#include <cstddef>
#include <utility>

#include "std/chem_string_view.h"

class Value;

/**
 * Storage for the variable bindings of a single InterpretScope.
 *
 * The historical implementation used a std::unordered_map, which allocates a
 * heap node for every declared variable. Interpreted loops create a fresh scope
 * (and therefore a fresh map) on every iteration, so a simple `var a = ...` in
 * a hot loop turned into a malloc/free pair per iteration.
 *
 * This container keeps the entries in a flat, insertion-ordered vector. Small
 * scopes (the overwhelming majority) are scanned linearly with no heap traffic.
 * Once a scope grows past a threshold, a hash index (name -> vector position) is
 * built lazily so that large scopes (globals, functions with many locals) still
 * get O(1) lookups without per-entry node allocations.
 *
 * The public surface intentionally mirrors the subset of std::unordered_map the
 * interpreter relied on: operator[], find, end, erase(key/iterator), clear,
 * begin/end iteration, and a value_type compatible with structured bindings.
 */
class ScopeValueMap {
public:

    using value_type = std::pair<chem::string_view, Value*>;
    using iterator = value_type*;
    using const_iterator = const value_type*;

    static constexpr std::size_t INDEX_THRESHOLD = 12;

    ScopeValueMap() = default;
    ScopeValueMap(ScopeValueMap&&) noexcept = default;
    ScopeValueMap& operator=(ScopeValueMap&&) noexcept = default;
    ScopeValueMap(const ScopeValueMap&) = delete;
    ScopeValueMap& operator=(const ScopeValueMap&) = delete;

    iterator begin() noexcept { return entries.data(); }
    iterator end() noexcept { return entries.data() + entries.size(); }
    const_iterator begin() const noexcept { return entries.data(); }
    const_iterator end() const noexcept { return entries.data() + entries.size(); }
    const_iterator cbegin() const noexcept { return entries.data(); }
    const_iterator cend() const noexcept { return entries.data() + entries.size(); }

    bool empty() const noexcept { return entries.empty(); }
    std::size_t size() const noexcept { return entries.size(); }

    Value*& operator[](const chem::string_view& key) {
        if(index.empty()) {
            for(auto& entry : entries) {
                if(entry.first == key) return entry.second;
            }
        } else {
            auto found = index.find(key);
            if(found != index.end()) return entries[found->second].second;
        }
        auto& slot = entries.emplace_back(key, nullptr).second;
        if(index.empty() && entries.size() > INDEX_THRESHOLD) {
            build_index();
        } else if(!index.empty()) {
            index[key] = entries.size() - 1;
        }
        return slot;
    }

    iterator find(const chem::string_view& key) {
        if(!index.empty()) {
            auto found = index.find(key);
            if(found == index.end()) return end();
            return entries.data() + found->second;
        }
        for(auto it = entries.begin(); it != entries.end(); ++it) {
            if(it->first == key) return &*it;
        }
        return end();
    }

    std::size_t erase(const chem::string_view& key) {
        if(!index.empty()) {
            auto found = index.find(key);
            if(found == index.end()) return 0;
            entries.erase(entries.begin() + found->second);
            rebuild_index();
            return 1;
        }
        for(auto it = entries.begin(); it != entries.end(); ++it) {
            if(it->first == key) {
                entries.erase(it);
                return 1;
            }
        }
        return 0;
    }

    iterator erase(iterator position) {
        const auto offset = static_cast<std::size_t>(position - entries.data());
        auto result = entries.erase(entries.begin() + offset);
        if(!index.empty()) {
            rebuild_index();
        }
        return entries.data() + (result - entries.begin());
    }

    void clear() {
        entries.clear();
        index.clear();
    }

private:

    void build_index() {
        index.reserve(entries.size() * 2);
        for(std::size_t i = 0; i < entries.size(); ++i) {
            index[entries[i].first] = i;
        }
    }

    void rebuild_index() {
        index.clear();
        if(entries.size() > INDEX_THRESHOLD) {
            build_index();
        }
    }

    std::vector<value_type> entries;
    std::unordered_map<chem::string_view, std::size_t> index;

};
