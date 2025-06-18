// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <span>
#include <unordered_map>
#include <vector>

#include <cassert>

class BaseType;

/**
 * each type inside this span is written for a generic instantiation
 * for example map<int, string> < -- here int, string types will be stored in the span
 */
using InstantiationType = std::span<BaseType*>;

struct DeclInstantiations {
    // the types that caused the instantiation are stored in this
    // types vector
    std::vector<InstantiationType>       types;
    // this is a reference to the instantiations vector stored in generic declaration
    // present in the ast, since every generic declaration owns the vector for instantiations
    // which can contain different pointers we use reference to vec to void*
    std::vector<void*>&                  implData;
    // same length as 'types' and 'implData';
    // each entry tells you which slot in fileIdRegistry[fileId] owns that inst.
    std::vector<unsigned int>            registryPositions;
};

struct RegistryEntry {
    void*           key;
    unsigned int    typeIndex;  // index within DeclInstantiations.types
};

/**
 * why is instantiations container required ?
 * well, in IDE, files change, which means AST units pointing to those files
 * become invalid after every edit, instantiations caused by the file changes
 * also become invalid, we need to track every instantiation with the file id
 * to remove them so it doesn't cause freed pointers present in AST and types
 */
class InstantiationsContainer {
private:

    // key → its list of instantiations
    std::unordered_map<void*, DeclInstantiations> instantiations;
    // fileId → vector of “who to delete” records
    std::unordered_map<unsigned int, std::vector<RegistryEntry>> fileIdRegistry;

public:

    /**
     * this is set by symbol resolver, when linking signature or body of each file
     * this allows us to know which file generic instantiations are coming from
     */
    unsigned int current_file_id = 0;

    /**
     * constructor
     */
    InstantiationsContainer(
            size_t expectedKeys = 1024,
            size_t expectedFiles = 16
    ) {
        instantiations.reserve(expectedKeys);
        fileIdRegistry.reserve(expectedFiles);
    }

    /**
     * Gets all the instantiations for the given key
     */
    std::span<InstantiationType> getInstantiationTypesFor(void* key) {
        auto it = instantiations.find(key);
        if (it == instantiations.end())
            return {};
        return it->second.types;
    }

    /**
     * Register a new instantiation under `key` coming from `fileId`.
     * - `types` is your span of BaseType*
     * - `instVec` is the external vector<void*>& you maintain for implData.
     */
    size_t registerInstantiation(
            void*                        key,
            InstantiationType            types,
            std::vector<void*>&          instVec
    ) {
        // 1) Grab-or-create our DeclInstantiations
        auto [it, inserted] = instantiations.try_emplace(
                key,
                DeclInstantiations{ {}, instVec, {} }
        );
        auto& decl = it->second;

        // 2) Where in this file's registry will we land?
        auto& registry = fileIdRegistry[current_file_id];
        auto regPos = static_cast<unsigned int>(registry.size());

        // 3) Append new instantiation
        auto instIdx = static_cast<unsigned int>(decl.types.size());
        decl.types          .push_back(types);
        decl.registryPositions.push_back(regPos);

        // 4) Remember to delete it later
        registry.push_back({ key, instIdx });
        return instIdx;
    }

    /**
     * this will remove instantiations for the given key
     */
    void removeInstantiationsFor(void* key) {
        auto found = instantiations.find(key);
        if(found != instantiations.end()) {
            instantiations.erase(found);
        }
    }

    /**
     * Remove all instantiations that were registered from `fileId`.
     * This runs in O(1) per instantiation (no middle‑of‑vector erases).
     */
    void removeInstantiationsFor(unsigned int fileId) {
        auto fit = fileIdRegistry.find(fileId);
        if (fit == fileIdRegistry.end()) return;

        auto& registry = fit->second;
        // Process in reverse so that swap‑and‑pop on registry itself is O(1).
        for (int i = static_cast<int>(registry.size()) - 1; i >= 0; --i) {
            auto rec = registry[i];
            auto  kit = instantiations.find(rec.key);
            if(kit == instantiations.end()) {
                // instantiation doesn't exist, probably because file changed and
                // its declarations were removed instantiations of (using key)
                registry.pop_back();
                continue;
            }
            auto& decl = kit->second;

            unsigned int removeIdx = rec.typeIndex;
            unsigned int lastIdx   = static_cast<unsigned int>(decl.types.size()) - 1;

            if (removeIdx != lastIdx) {
                // 1) swap our vectors’ entries
                std::swap(decl.types[removeIdx],       decl.types[lastIdx]);
                std::swap(decl.implData[removeIdx],    decl.implData[lastIdx]);
                std::swap(decl.registryPositions[removeIdx], decl.registryPositions[lastIdx]);

                // 2) fix up the RegistryEntry that now points to 'lastIdx'
                unsigned int otherRegPos = decl.registryPositions[removeIdx];
                auto& otherRec = registry[otherRegPos];
                otherRec.typeIndex = removeIdx;
            }

            // pop our key’s data
            decl.types.pop_back();
            decl.implData.pop_back();
            decl.registryPositions.pop_back();

            // and pop our registry entry
            registry.pop_back();
        }

        // finally, drop the empty registry vector
        fileIdRegistry.erase(fit);
    }

};
