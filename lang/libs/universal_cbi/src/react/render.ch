func render_universal_jsx(
    builder : *mut ASTBuilder,
    node : *mut JsNode,
    path : std::string_view,
    states : &std::vector<UniversalStateDecl>,
    textBindings : &mut std::vector<UniversalTextBinding>,
    eventBindings : &mut std::vector<UniversalEventBinding>,
    propBindings : &mut std::vector<UniversalPropTextBinding>,
    attrBindings : &mut std::vector<UniversalAttrBinding>,
    nestedBindings : &mut std::vector<UniversalNestedBinding>,
    out : &mut std::vector<TemplateToken>,
    propsName : std::string_view,
    converter : &mut JsConverter
) : bool {
    if(node == null) return false;

    switch(node.kind) {
        JsNodeKind.JSXText => {
            const text = node as *mut JsJSXText;
            escape_html_append(converter.str, text.value);
            return true;
        }
        JsNodeKind.JSXExpressionContainer => {
            const container = node as *mut JsJSXExpressionContainer;
            if(container.expression != null) {
                if(container.expression.kind == JsNodeKind.Identifier) {
                    const id = container.expression as *mut JsIdentifier;
                    if(has_state(states, id.value)) {
                        const initial = find_state_init_text(states, id.value);
                        converter.str.append_view("<span>");
                        escape_html_append(converter.str, initial);
                        converter.str.append_view("</span>");
                        textBindings.push(UniversalTextBinding {
                            stateName : id.value,
                            path : path
                        });
                        return true;
                    }
                } else if(container.expression.kind == JsNodeKind.Literal) {
                    const lit = container.expression as *mut JsLiteral;
                    escape_html_append(converter.str, strip_js_string_quotes(lit.value));
                    return true;
                } else if(container.expression.kind == JsNodeKind.ObjectLiteral) {
                    const obj = container.expression as *mut JsObjectLiteral;
                    if(obj.properties.size() == 1) {
                        const prop = obj.properties.get(0);
                        if(prop.value != null && prop.value.kind == JsNodeKind.Identifier) {
                            const id = prop.value as *mut JsIdentifier;
                            if(has_state(states, id.value)) {
                                const initial = find_state_init_text(states, id.value);
                                converter.str.append_view("<span>");
                                escape_html_append(converter.str, initial);
                                converter.str.append_view("</span>");
                                textBindings.push(UniversalTextBinding {
                                    stateName : id.value,
                                    path : path
                                });
                                return true;
                            }
                        }
                    }
                }

                // Check for prop access
                const propPath = get_prop_access_path(builder, container.expression, propsName);
                if(!propPath.empty()) {
                    converter.flush_text(out);
                    if(propPath.equals("children")) {
                        out.push(TemplateToken { kind : TemplateTokenKind.Children });
                    } else {
                        out.push(TemplateToken { kind : TemplateTokenKind.PropAccess, value : propPath });
                        propBindings.push(UniversalPropTextBinding {
                            propPath : propPath,
                            path : path
                        });
                    }
                    return true;
                }

                // Check for chemical value
                if(container.expression.kind == JsNodeKind.ChemicalValue) {
                    converter.flush_text(out);
                    const cv = container.expression as *mut JsChemicalValue;
                    out.push(TemplateToken { kind : TemplateTokenKind.ChemicalValue, chemicalValue : cv.value });
                    return true;
                }
            }
            // Any unsupported JSX expression should fall back to runtime rendering.
            return false;
        }
        JsNodeKind.JSXFragment => {
            const frag = node as *mut JsJSXFragment;
            var childElementIndex : uint = 0;
            for(var i : uint = 0; i < frag.children.size(); i++) {
                const child = frag.children.get(i);
                if(child == null) continue;
                if(child.kind == JsNodeKind.JSXElement) {
                    const childPath = build_child_path(builder, path, childElementIndex);
                    if(!render_universal_jsx(builder, child, childPath, states, textBindings, eventBindings, propBindings, attrBindings, nestedBindings, out, propsName, converter)) {
                        return false;
                    }
                    childElementIndex++;
                } else if(child.kind == JsNodeKind.JSXExpressionContainer) {
                    const childPath = build_child_path(builder, path, childElementIndex);
                    if(!render_universal_jsx(builder, child, childPath, states, textBindings, eventBindings, propBindings, attrBindings, nestedBindings, out, propsName, converter)) {
                        return false;
                    }
                    const cont = child as *mut JsJSXExpressionContainer;
                    if(cont.expression != null && cont.expression.kind == JsNodeKind.Identifier) {
                        const id = cont.expression as *mut JsIdentifier;
                        if(has_state(states, id.value)) childElementIndex++;
                    }
                } else {
                    if(!render_universal_jsx(builder, child, path, states, textBindings, eventBindings, propBindings, attrBindings, nestedBindings, out, propsName, converter)) {
                        return false;
                    }
                }
            }
            return true;
        }
        JsNodeKind.JSXElement => {
            const element = node as *mut JsJSXElement;
            if(element.opening.tagName == null || element.opening.tagName.kind != JsNodeKind.Identifier) return false;
            const tagNode = element.opening.tagName as *mut JsIdentifier;
            const tagName = tagNode.value;
            if(!is_native_tag(tagName)) {
                if(element.componentSignature != null && element.componentSignature.mountStrategy == MountStrategy.Universal && !element.componentSignature.universalTemplate.empty()) {
                    converter.flush_text(out);
                    var compName = std::string();
                    get_module_scoped_name(element.componentSignature.functionNode as *mut ASTNode, tagName, compName);
                    const propsExpr = build_nested_props_expr(builder, element, states, converter.support, converter.tokens);
                    nestedBindings.push(UniversalNestedBinding {
                        componentName : builder.allocate_view(compName.to_view()),
                        path : path,
                        propsExpr : propsExpr
                    });

                    // Inline the universal template of the nested component
                    for(var i : uint = 0; i < element.componentSignature.universalTemplate.size(); i++) {
                        const tok = element.componentSignature.universalTemplate.get(i);
                        if(tok.kind == TemplateTokenKind.Text) {
                            converter.str.append_view(tok.value);
                        } else if(tok.kind == TemplateTokenKind.PropAccess) {
                            if(tok.value.equals("children")) {
                                var childElementIndex : uint = 0;
                                for(var ci : uint = 0; ci < element.children.size(); ci++) {
                                    const ch = element.children.get(ci);
                                    if(ch == null) continue;
                                    if(ch.kind == JsNodeKind.JSXElement) {
                                        const childPath = build_child_path(builder, path, childElementIndex);
                                        if(!render_universal_jsx(builder, ch, childPath, states, textBindings, eventBindings, propBindings, attrBindings, nestedBindings, out, propsName, converter)) return false;
                                        childElementIndex++;
                                    } else if(ch.kind == JsNodeKind.JSXExpressionContainer) {
                                        const childPath = build_child_path(builder, path, childElementIndex);
                                        if(!render_universal_jsx(builder, ch, childPath, states, textBindings, eventBindings, propBindings, attrBindings, nestedBindings, out, propsName, converter)) return false;
                                        const cont = ch as *mut JsJSXExpressionContainer;
                                        if(cont.expression != null && cont.expression.kind == JsNodeKind.Identifier) {
                                            const id = cont.expression as *mut JsIdentifier;
                                            if(has_state(states, id.value)) childElementIndex++;
                                        }
                                    } else {
                                        if(!render_universal_jsx(builder, ch, path, states, textBindings, eventBindings, propBindings, attrBindings, nestedBindings, out, propsName, converter)) return false;
                                    }
                                }
                            } else {
                                var txt = view("");
                                var propPath = view("");
                                var chem : *mut Value = null;
                                const kind = resolve_nested_prop_as_text(builder, element, tok.value, propsName, states, txt, propPath, chem);
                                if(kind == 1 && !txt.empty()) {
                                    converter.str.append_view(txt);
                                } else if(kind == 2 && !propPath.empty()) {
                                    converter.flush_text(out);
                                    out.push(TemplateToken { kind : TemplateTokenKind.PropAccess, value : propPath });
                                } else if(kind == 3 && chem != null) {
                                    converter.flush_text(out);
                                    out.push(TemplateToken { kind : TemplateTokenKind.ChemicalValue, chemicalValue : chem });
                                }
                            }
                        } else if(tok.kind == TemplateTokenKind.MergedAttribute) {
                            if(tok.mergedAttr == null) continue;
                            var merged = builder.allocate<MergedAttribute>();
                            new (merged) MergedAttribute { name : tok.mergedAttr.name, segments : std::vector<MergedAttrSegment>() };
                            for(var si : uint = 0; si < tok.mergedAttr.segments.size(); si++) {
                                const seg = tok.mergedAttr.segments.get(si);
                                if(seg.kind == MergedAttrSegmentKind.Text) {
                                    merged.segments.push(seg);
                                } else if(seg.kind == MergedAttrSegmentKind.PropAccess) {
                                    var txt2 = view("");
                                    var propPath2 = view("");
                                    var chem2 : *mut Value = null;
                                    const kind2 = resolve_nested_prop_as_text(builder, element, seg.value, propsName, states, txt2, propPath2, chem2);
                                    if(kind2 == 1 && !txt2.empty()) {
                                        merged.segments.push(MergedAttrSegment { kind : MergedAttrSegmentKind.Text, value : txt2, chemicalValue : null });
                                    } else if(kind2 == 2 && !propPath2.empty()) {
                                        merged.segments.push(MergedAttrSegment { kind : MergedAttrSegmentKind.PropAccess, value : propPath2, chemicalValue : null });
                                    } else if(kind2 == 3 && chem2 != null) {
                                        merged.segments.push(MergedAttrSegment { kind : MergedAttrSegmentKind.ChemicalValue, value : view(""), chemicalValue : chem2 });
                                    }
                                } else if(seg.kind == MergedAttrSegmentKind.ChemicalValue) {
                                    merged.segments.push(seg);
                                }
                            }
                            converter.flush_text(out);
                            out.push(TemplateToken { kind : TemplateTokenKind.MergedAttribute, mergedAttr : merged });
                        } else if(tok.kind == TemplateTokenKind.ChemicalValue) {
                            converter.flush_text(out);
                            out.push(TemplateToken { kind : TemplateTokenKind.ChemicalValue, chemicalValue : tok.chemicalValue });
                        }
                    }
                    return true;
                }
                converter.flush_text(out);
                out.push(TemplateToken { kind : TemplateTokenKind.NestedComponent, jsxElement : element as *mut void });
                return true;
            }

            converter.str.append('<');
            converter.str.append_view(tagName);

            var classSegments = std::vector<MergedAttrSegment>();
            var styleSegments = std::vector<MergedAttrSegment>();
            var hasSpread = false;

            for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
                const attrNode = element.opening.attributes.get(i);
                if(attrNode == null) continue;
                if(attrNode.kind == JsNodeKind.JSXSpreadAttribute) {
                    hasSpread = true;
                    classSegments.push(MergedAttrSegment { kind : MergedAttrSegmentKind.PropAccess, value : view("class"), chemicalValue : null });
                    classSegments.push(MergedAttrSegment { kind : MergedAttrSegmentKind.PropAccess, value : view("className"), chemicalValue : null });
                    styleSegments.push(MergedAttrSegment { kind : MergedAttrSegmentKind.PropAccess, value : view("style"), chemicalValue : null });
                    converter.flush_text(out);
                    out.push(TemplateToken { kind : TemplateTokenKind.Spread });
                    continue;
                }
                if(attrNode.kind != JsNodeKind.JSXAttribute) continue;
                const attr = attrNode as *mut JsJSXAttribute;

                if(is_event_attr_name(attr.name)) {
                    if(attr.value != null && attr.value.kind == JsNodeKind.JSXExpressionContainer) {
                        const container = attr.value as *mut JsJSXExpressionContainer;
                        const handler = js_node_to_source(builder, container.expression, states, converter.support, converter.tokens);
                        eventBindings.push(UniversalEventBinding {
                            eventName : event_attr_to_dom_event(builder, attr.name),
                            path : path,
                            handlerExpr : handler
                        });
                    }
                    continue;
                }

                const isClass = attr.name.equals("class") || attr.name.equals("className");
                const isStyle = attr.name.equals("style");

                if(isClass || isStyle) {
                    if(attr.value == null) continue;
                    var target = if(isClass) &mut classSegments else &mut styleSegments;
                    if(attr.value.kind == JsNodeKind.Literal) {
                        const lit = attr.value as *mut JsLiteral;
                        const txt = strip_js_string_quotes(lit.value);
                        if(!txt.empty()) {
                            target.push(MergedAttrSegment { kind : MergedAttrSegmentKind.Text, value : txt, chemicalValue : null });
                        }
                    } else if(attr.value.kind == JsNodeKind.JSXExpressionContainer) {
                        const container = attr.value as *mut JsJSXExpressionContainer;
                        if(container.expression != null && container.expression.kind == JsNodeKind.Literal) {
                            const lit = container.expression as *mut JsLiteral;
                            const txt = strip_js_string_quotes(lit.value);
                            if(!txt.empty()) {
                                target.push(MergedAttrSegment { kind : MergedAttrSegmentKind.Text, value : txt, chemicalValue : null });
                            }
                        } else if(isStyle && container.expression != null && container.expression.kind == JsNodeKind.ObjectLiteral) {
                            const obj = container.expression as *mut JsObjectLiteral;
                            var cssText = view("");
                            if(!try_build_style_object_text(builder, obj, cssText)) return false;
                            if(!cssText.empty()) {
                                target.push(MergedAttrSegment { kind : MergedAttrSegmentKind.Text, value : cssText, chemicalValue : null });
                            }
                        } else if(container.expression != null && container.expression.kind == JsNodeKind.Identifier) {
                            const id = container.expression as *mut JsIdentifier;
                            if(has_state(states, id.value)) {
                                const initText = find_state_init_text(states, id.value);
                                if(!initText.empty()) target.push(MergedAttrSegment { kind : MergedAttrSegmentKind.Text, value : initText, chemicalValue : null });
                            } else {
                                const propPath = get_prop_access_path(builder, container.expression, propsName);
                                if(!propPath.empty() && !propPath.equals("children")) {
                                    target.push(MergedAttrSegment { kind : MergedAttrSegmentKind.PropAccess, value : propPath, chemicalValue : null });
                                } else { return false; }
                            }
                        } else if(container.expression != null && container.expression.kind == JsNodeKind.MemberAccess) {
                            const propPath = get_prop_access_path(builder, container.expression, propsName);
                            if(!propPath.empty()) {
                                target.push(MergedAttrSegment { kind : MergedAttrSegmentKind.PropAccess, value : propPath, chemicalValue : null });
                            } else { return false; }
                        } else if(container.expression != null && container.expression.kind == JsNodeKind.ChemicalValue) {
                            const cv = container.expression as *mut JsChemicalValue;
                            target.push(MergedAttrSegment { kind : MergedAttrSegmentKind.ChemicalValue, value : view(""), chemicalValue : cv.value });
                        } else { return false; }
                    } else { return false; }
                    continue;
                }

                converter.str.append(' ');
                converter.str.append_view(attr.name);
                if(attr.value != null) {
                    converter.str.append_view("=\"");
                    if(attr.value.kind == JsNodeKind.Literal) {
                        const lit = attr.value as *mut JsLiteral;
                        escape_html_append(converter.str, strip_js_string_quotes(lit.value));
                    } else if(attr.value.kind == JsNodeKind.JSXExpressionContainer) {
                        const container = attr.value as *mut JsJSXExpressionContainer;
                        if(container.expression != null && container.expression.kind == JsNodeKind.Identifier) {
                             const id = container.expression as *mut JsIdentifier;
                             if(has_state(states, id.value)) {
                                 escape_html_append(converter.str, find_state_init_text(states, id.value));
                             } else {
                                 const propPath = get_prop_access_path(builder, container.expression, propsName);
                                 if(!propPath.empty()) {
                                     if(!propPath.equals("children")) {
                                         converter.flush_text(out);
                                         out.push(TemplateToken { kind : TemplateTokenKind.PropAccess, value : propPath });
                                     }
                                 } else { return false; }
                             }
                        } else if(container.expression != null && container.expression.kind == JsNodeKind.Literal) {
                             const lit = container.expression as *mut JsLiteral;
                             escape_html_append(converter.str, strip_js_string_quotes(lit.value));
                        } else if(container.expression != null && container.expression.kind == JsNodeKind.MemberAccess) {
                             const propPath = get_prop_access_path(builder, container.expression, propsName);
                             if(!propPath.empty()) {
                                 converter.flush_text(out);
                                 out.push(TemplateToken { kind : TemplateTokenKind.PropAccess, value : propPath });
                             } else { return false; }
                        } else if(container.expression != null && container.expression.kind == JsNodeKind.ChemicalValue) {
                                converter.flush_text(out);
                                const cv = container.expression as *mut JsChemicalValue;
                                out.push(TemplateToken { kind : TemplateTokenKind.ChemicalValue, chemicalValue : cv.value });
                        } else { return false; }
                    }
                    converter.str.append('"');
                }
            }

            if(!classSegments.empty() || !styleSegments.empty() || hasSpread) {
                converter.flush_text(out);
                if(!classSegments.empty() || hasSpread) {
                    var mergedClass = builder.allocate<MergedAttribute>();
                    new (mergedClass) MergedAttribute { name : view("class"), segments : std::vector<MergedAttrSegment>() };
                    for(var i : uint = 0; i < classSegments.size(); i++) mergedClass.segments.push(classSegments.get(i));
                    out.push(TemplateToken { kind : TemplateTokenKind.MergedAttribute, mergedAttr : mergedClass });
                    attrBindings.push(UniversalAttrBinding { attr : mergedClass, path : path });
                }
                if(!styleSegments.empty() || hasSpread) {
                    var mergedStyle = builder.allocate<MergedAttribute>();
                    new (mergedStyle) MergedAttribute { name : view("style"), segments : std::vector<MergedAttrSegment>() };
                    for(var i : uint = 0; i < styleSegments.size(); i++) mergedStyle.segments.push(styleSegments.get(i));
                    out.push(TemplateToken { kind : TemplateTokenKind.MergedAttribute, mergedAttr : mergedStyle });
                    attrBindings.push(UniversalAttrBinding { attr : mergedStyle, path : path });
                }
            }
            converter.str.append('>');

            if (hasSpread) {
                converter.flush_text(out);
                out.push(TemplateToken { kind : TemplateTokenKind.Children });
            }

            var childElementIndex : uint = 0;
            for(var i : uint = 0; i < element.children.size(); i++) {
                const child = element.children.get(i);
                if(child == null) continue;
                if(child.kind == JsNodeKind.JSXElement) {
                    const childPath = build_child_path(builder, path, childElementIndex);
                    if(!render_universal_jsx(builder, child, childPath, states, textBindings, eventBindings, propBindings, attrBindings, nestedBindings, out, propsName, converter)) return false;
                    childElementIndex++;
                } else if(child.kind == JsNodeKind.JSXExpressionContainer) {
                    const childPath = build_child_path(builder, path, childElementIndex);
                    if(!render_universal_jsx(builder, child, childPath, states, textBindings, eventBindings, propBindings, attrBindings, nestedBindings, out, propsName, converter)) return false;
                    const cont = child as *mut JsJSXExpressionContainer;
                    if(cont.expression != null && cont.expression.kind == JsNodeKind.Identifier) {
                        const id = cont.expression as *mut JsIdentifier;
                        if(has_state(states, id.value)) childElementIndex++;
                    }
                } else {
                    if(!render_universal_jsx(builder, child, path, states, textBindings, eventBindings, propBindings, attrBindings, nestedBindings, out, propsName, converter)) return false;
                }
            }

            converter.str.append_view("</");
            converter.str.append_view(tagName);
            converter.str.append('>');
            return true;
        }
        default => {
            return false;
        }
    }
    return false;
}
