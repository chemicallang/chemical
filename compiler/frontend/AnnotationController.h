// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <cstdint>
#include <cassert>
#include "std/chem_string_view.h"

class ASTNode;
class Value;
class Parser;
class AnnotationController;

enum class AnnotationDefType {
    /**
     * the annotation has a handler that is invoked
     */
    Handler,
    /**
     * marks a node, so you can quickly check whether a node has been marked
     * with a given annotation
     */
    Marker,
    /**
     * collects node into a known collection
     */
    Collector,
    /**
     * marks the node and collects into a known collection
     */
    MarkerAndCollector,
};

struct CollectedAnnotation {

    /**
     * collected node
     */
    ASTNode* node;

    /**
     * the arguments given to the annotation
     */
    std::vector<Value*> args;

};

struct AnnotationCollection {

    std::vector<CollectedAnnotation> nodes;

};

struct AnnotationDefinition {

    union {

        // intrinsic annotations have this handler
        void(*handler)(Parser* parser, ASTNode* node);

        // collector annotation have collection reference
        std::size_t collection_id;

    };

    AnnotationDefType type;

};

// Combined key
struct MarkedAnnotatedNode {
    ASTNode* ptr;
    chem::string_view sv;
};

// Hash functor (use with unordered_map)
struct MarkedAnnotatedNodeHash {
    std::size_t operator()(MarkedAnnotatedNode const& k) const noexcept {
        // Hash pointer and string_view using std::hash
        std::size_t h1 = std::hash<void*>()(k.ptr);
        std::size_t h2 = std::hash<chem::string_view>()(k.sv);
        // combine: boost::hash_combine style
        // magic constant from boost
        constexpr std::size_t magic = 0x9e3779b97f4a7c15ULL;
        h1 ^= h2 + magic + (h1 << 6) + (h1 >> 2);
        return h1;
    }
};

// Equality comparator
struct MarkedAnnotatedNodeEq {
    bool operator()(MarkedAnnotatedNode const& a, MarkedAnnotatedNode const& b) const noexcept {
        return a.ptr == b.ptr && a.sv == b.sv;
    }
};

class AnnotationController {
private:

    /**
     * annotation definitions
     */
    std::unordered_map<chem::string_view, AnnotationDefinition> definitions;

    /**
     * annotation collections
     */
    std::vector<AnnotationCollection> collections;

    /**
     * nodes that have been marked
     * the map value is the arguments of annotation
     */
    std::unordered_map<MarkedAnnotatedNode, std::vector<Value*>, MarkedAnnotatedNodeHash, MarkedAnnotatedNodeEq> marked;

public:

    std::size_t create_collection(const chem::string_view& name, unsigned int expected_usage) {
        const auto index = collections.size();
        collections.emplace_back();
        if(expected_usage > 2) {
            collections.back().nodes.reserve(expected_usage);
        }
        return index;
    }

private:

    void create_collector_annotation(const chem::string_view& name, AnnotationDefType type, unsigned int expected_usage) {
        definitions.emplace(name, AnnotationDefinition {
            .collection_id = create_collection(name, expected_usage),
            .type = type
        });
    }

public:

    inline void create_collector_annotation(const chem::string_view& name, unsigned int expected_usage) {
        create_collector_annotation(name, AnnotationDefType::Collector, expected_usage);
    }

    inline void create_marker_and_collector_annotation(const chem::string_view& name, unsigned int expected_usage) {
        create_collector_annotation(name, AnnotationDefType::MarkerAndCollector, expected_usage);
    }

    void create_marker_annotation(const chem::string_view& name) {
        definitions.emplace(name, AnnotationDefinition {
                .collection_id = 0,
                .type = AnnotationDefType::Marker
        });
    }

    void mark(ASTNode* node, const chem::string_view& name, AnnotationDefinition& definition, std::vector<Value*>* arguments) {
        marked.emplace(MarkedAnnotatedNode{node, name}, arguments ? std::move(*arguments) : std::vector<Value*> {});
    }

    void collect(ASTNode* node, const chem::string_view& name, AnnotationDefinition& definition, std::vector<Value*>* arguments) {
        auto& coll = collections[definition.collection_id];
        coll.nodes.emplace_back(node, arguments ? std::move(*arguments) : std::vector<Value*> {});
    }

    void mark_and_collect(ASTNode* node, const chem::string_view& name, AnnotationDefinition& definition, std::vector<Value*>* arguments) {
        mark(node, name, definition, arguments);
        collect(node, name, definition, nullptr);
    }

    bool is_marked(ASTNode* node, const chem::string_view& name) {
        return marked.find(MarkedAnnotatedNode{node, name}) != marked.end();
    }

    std::vector<Value*>* get_args(ASTNode* node, const chem::string_view& name) {
        auto found = marked.find(MarkedAnnotatedNode{node, name});
        return found == marked.end() ? nullptr : &found->second;
    }

    bool handle_annotation(Parser* parser, ASTNode* node, const chem::string_view& name, std::vector<Value*>* arguments) {
        auto found = definitions.find(name);
        if(found == definitions.end()) {
            return false;
        } else {
            auto& definition = found->second;
            switch(definition.type) {
                case AnnotationDefType::Handler:
                    definition.handler(parser, node);
                    return true;
                case AnnotationDefType::Marker:
                    mark(node, name, definition, arguments);
                    return true;
                case AnnotationDefType::Collector:
                    collect(node, name, definition, arguments);
                    return true;
                case AnnotationDefType::MarkerAndCollector:
                    mark_and_collect(node, name, definition, arguments);
                    return true;
            }
        }
    }

    /**
     * constructor
     * @param is_env_testing required, during testing environment the testing collector
     * annotation is initialized with a large container so that annotations are collected quickly
     */
    explicit AnnotationController(bool is_env_testing);

};
