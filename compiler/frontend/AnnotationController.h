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
#include <mutex>
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
     * when an annotation must be present on a single node in the entire executable
     * the single marker annotation is used, when multiple annotations are used
     * they override
     */
    SingleMarker,
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

enum class SingleMarkerMultiplePolicy {
    Override,
    Ignore,
    Error
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
        void(*handler)(Parser* parser, ASTNode* node, std::vector<Value*>& args);

        // collector annotation have collection reference
        std::size_t collection_id;

        // policy only available on single marked annotations
        SingleMarkerMultiplePolicy policy;

    };

    chem::string_view name;

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

    /**
     * single marked nodes are stored here
     */
    std::unordered_map<chem::string_view, CollectedAnnotation> single_marked;

public:

    /**
     * this mutex is used to handle detected annotations
     * because we parse multiple files concurrently
     */
    std::mutex single_marker_mutex;

    /**
     * this mutex is used to handle detected annotations
     * because we parse multiple files concurrently
     */
    std::mutex marker_mutex;

    /**
     * this mutex is used to handle detected annotations
     * because we parse multiple files concurrently
     */
    std::mutex collector_mutex;

    /**
     * constructor
     * @param is_env_testing required, during testing environment the testing collector
     * annotation is initialized with a large container so that annotations are collected quickly
     */
    explicit AnnotationController();

private:

    /**
     * create a collection with expected range and get an id
     */
    std::size_t create_collection(unsigned int expected_usage) {
        const auto index = collections.size();
        collections.emplace_back();
        if(expected_usage > 2) {
            collections.back().nodes.reserve(expected_usage);
        }
        return index;
    }

    void create_collector_annotation(const chem::string_view& name, AnnotationDefType type, unsigned int expected_usage) {
        definitions.emplace(name, AnnotationDefinition {
            .collection_id = create_collection(expected_usage),
            .name = name,
            .type = type
        });
    }

public:

    /**
     * this disposes any marked + collected nodes
     * this is done after a single executable been processed
     * definitions of annotations remain
     */
    void dispose_stored_nodes() {
        marked.clear();
        for(auto& coll : collections) {
            coll.nodes.clear();
        }
    }

    inline AnnotationCollection& get_collection(std::size_t collection_id) {
        return collections[collection_id];
    }

    inline void create_collector_annotation(const chem::string_view& name, unsigned int expected_usage) {
        create_collector_annotation(name, AnnotationDefType::Collector, expected_usage);
    }

    inline void create_marker_and_collector_annotation(const chem::string_view& name, unsigned int expected_usage) {
        create_collector_annotation(name, AnnotationDefType::MarkerAndCollector, expected_usage);
    }

    inline void create_single_marker_annotation(const chem::string_view& name, SingleMarkerMultiplePolicy policy = SingleMarkerMultiplePolicy::Override) {
        definitions.emplace(name, AnnotationDefinition {
                .policy = policy,
                .name = name,
                .type = AnnotationDefType::SingleMarker
        });
    }

    inline void create_marker_annotation(const chem::string_view& name) {
        definitions.emplace(name, AnnotationDefinition {
                .name = name,
                .type = AnnotationDefType::Marker
        });
    }

    inline void mark_single_emplace(ASTNode* node, AnnotationDefinition& definition, std::vector<Value*>& arguments) {
        single_marked.emplace(definition.name, CollectedAnnotation{node, std::move(arguments)});
    }

    // returns true on error
    bool mark_single(ASTNode* node, AnnotationDefinition& definition, std::vector<Value*>& arguments);

    void mark(ASTNode* node, AnnotationDefinition& definition, std::vector<Value*>& arguments) {
        std::lock_guard lock(marker_mutex);
        marked.emplace(MarkedAnnotatedNode{node, definition.name}, std::move(arguments));
    }

    void collect(ASTNode* node, AnnotationDefinition& definition, std::vector<Value*>& arguments) {
        std::lock_guard lock(collector_mutex);
        auto& coll = collections[definition.collection_id];
        coll.nodes.emplace_back(node, std::move(arguments));
    }

    void mark_and_collect(ASTNode* node, AnnotationDefinition& definition, std::vector<Value*>& arguments) {
        mark(node, definition, arguments);
        collect(node, definition, arguments);
    }

    bool is_marked(ASTNode* node, const chem::string_view& name) {
        return marked.find(MarkedAnnotatedNode{node, name}) != marked.end();
    }

    CollectedAnnotation* get_single_marked(const chem::string_view& name) {
        auto found = single_marked.find(name);
        return found == single_marked.end() ? nullptr : &found->second;
    }

    std::vector<Value*>* get_args(ASTNode* node, const chem::string_view& name) {
        auto found = marked.find(MarkedAnnotatedNode{node, name});
        return found == marked.end() ? nullptr : &found->second;
    }

    bool handle_annotation(AnnotationDefinition& definition, Parser* parser, ASTNode* node, std::vector<Value*>& arguments);

    AnnotationDefinition* get_definition(const chem::string_view& name) {
        auto found = definitions.find(name);
        return found == definitions.end() ? nullptr : &found->second;
    }

    bool handle_annotation(Parser* parser, ASTNode* node, const chem::string_view& name, std::vector<Value*>& arguments) {
        auto found = definitions.find(name);
        return found == definitions.end() ? false : handle_annotation(found->second, parser, node, arguments);
    }

    // Other Helper Functions

    void ensure_test_resources();

};
