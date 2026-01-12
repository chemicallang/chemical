// Copyright (c) Chemical Language Foundation 2026.

#include "AnnotationControllerCBI.h"
#include "compiler/frontend/AnnotationController.h"

AnnotationDefinition* AnnotationControllergetDefinition(AnnotationController* controller, chem::string_view* name) {
    return controller->get_definition(*name);
}

void AnnotationControllercreateSingleMarkerAnnotation(AnnotationController* controller, chem::string_view* name, int policy) {
    controller->create_single_marker_annotation(*name, (SingleMarkerMultiplePolicy) policy);
}

void AnnotationControllercreateMarkerAnnotation(AnnotationController* controller, chem::string_view* name) {
    controller->create_marker_annotation(*name);
}

void AnnotationControllercreateCollectorAnnotation(AnnotationController* controller, chem::string_view* name, unsigned int expected_usage) {
    controller->create_collector_annotation(*name, expected_usage);
}

void AnnotationControllercreateMarkerAndCollectorAnnotation(AnnotationController* controller, chem::string_view* name, unsigned int expected_usage) {
    controller->create_marker_and_collector_annotation(*name, expected_usage);
}

bool AnnotationControllermarkSingle(AnnotationController* controller, ASTNode* node, AnnotationDefinition* def, ValueSpan* args) {
    std::vector<Value*> argsVec;
    take_chemical_values(argsVec, args);
    return controller->mark_single(node, *def, argsVec);
}

void AnnotationControllermark(AnnotationController* controller, ASTNode* node, AnnotationDefinition* def, ValueSpan* args) {
    std::vector<Value*> argsVec;
    take_chemical_values(argsVec, args);
    return controller->mark(node, *def, argsVec);
}

void AnnotationControllercollect(AnnotationController* controller, ASTNode* node, AnnotationDefinition* def, ValueSpan* args) {
    std::vector<Value*> argsVec;
    take_chemical_values(argsVec, args);
    return controller->collect(node, *def, argsVec);
}

void AnnotationControllermarkAndCollect(AnnotationController* controller, ASTNode* node, AnnotationDefinition* def, ValueSpan* args) {
    std::vector<Value*> argsVec;
    take_chemical_values(argsVec, args);
    return controller->mark_and_collect(node, *def, argsVec);
}

bool AnnotationControllerhandleAnnotation(AnnotationController* controller, Parser* parser, ASTNode* node, AnnotationDefinition* def, ValueSpan* args) {
    std::vector<Value*> argsVec;
    take_chemical_values(argsVec, args);
    return controller->handle_annotation(*def, parser, node, argsVec);
}

bool AnnotationControllerisMarked(AnnotationController* controller, ASTNode* node, chem::string_view* name) {
    return controller->is_marked(node, *name);
}