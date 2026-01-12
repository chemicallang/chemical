// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include "std/chem_string_view.h"
#include "CBIUtils.h"

struct AnnotationDefinition;

class AnnotationController;

class ASTNode;

class Parser;

extern "C" {

    AnnotationDefinition* AnnotationControllergetDefinition(AnnotationController* controller, chem::string_view* name);

    void AnnotationControllercreateSingleMarkerAnnotation(AnnotationController* controller, chem::string_view* name, int policy);

    void AnnotationControllercreateMarkerAnnotation(AnnotationController* controller, chem::string_view* name);

    void AnnotationControllercreateCollectorAnnotation(AnnotationController* controller, chem::string_view* name, unsigned int expected_usage);

    void AnnotationControllercreateMarkerAndCollectorAnnotation(AnnotationController* controller, chem::string_view* name, unsigned int expected_usage);

    bool AnnotationControllermarkSingle(AnnotationController* controller, ASTNode* node, AnnotationDefinition* def, ValueSpan* args);

    void AnnotationControllermark(AnnotationController* controller, ASTNode* node, AnnotationDefinition* def, ValueSpan* args);

    void AnnotationControllercollect(AnnotationController* controller, ASTNode* node, AnnotationDefinition* def, ValueSpan* args);

    void AnnotationControllermarkAndCollect(AnnotationController* controller, ASTNode* node, AnnotationDefinition* def, ValueSpan* args);

    bool AnnotationControllerhandleAnnotation(AnnotationController* controller, Parser* parser, ASTNode* node, AnnotationDefinition* def, ValueSpan* args);

    bool AnnotationControllerisMarked(AnnotationController* controller, ASTNode* node, chem::string_view* name);



}