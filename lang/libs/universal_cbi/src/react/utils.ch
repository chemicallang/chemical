func is_event_attr_name(name : std::string_view) : bool {
    return name.size() > 2 && name.get(0) == 'o' && name.get(1) == 'n';
}

func event_attr_to_dom_event(builder : *mut ASTBuilder, name : std::string_view) : std::string_view {
    if(name.size() <= 2) return name;
    var eventName = std::string();
    for(var i : uint = 2; i < name.size(); i++) {
        var c = name.get(i);
        if(c >= 'A' && c <= 'Z') {
            c = c + 32;
        }
        eventName.append(c);
    }
    return builder.allocate_view(eventName.to_view());
}

func is_native_tag(tag : std::string_view) : bool {
    return !tag.empty() && tag.get(0) >= 'a' && tag.get(0) <= 'z';
}

func escape_html_append(out : &mut std::string, text : std::string_view) {
    for(var i : uint = 0; i < text.size(); i++) {
        const c = text.get(i);
        switch(c) {
            '&' => out.append_view("&amp;")
            '<' => out.append_view("&lt;")
            '>' => out.append_view("&gt;")
            '"' => out.append_view("&quot;")
            '\'' => out.append_view("&#39;")
            default => out.append(c)
        }
    }
}

func strip_js_string_quotes(value : std::string_view) : std::string_view {
    if(value.size() >= 2) {
        const first = value.get(0);
        const last = value.get(value.size() - 1);
        if((first == '"' || first == '\'' || first == '`') && first == last) {
            return value.subview(1, value.size() - 1);
        }
    }
    return value;
}

func append_css_key(out : &mut std::string, key : std::string_view) : bool {
    var k = key;
    if(k.size() >= 2) {
        const first = k.get(0);
        const last = k.get(k.size() - 1);
        if((first == '"' || first == '\'' || first == '`') && first == last) {
            k = k.subview(1, k.size() - 1);
        }
    }
    if(k.empty()) return false;
    if(k.size() >= 2 && k.get(0) == '-' && k.get(1) == '-') {
        out.append_view(k);
        return true;
    }
    for(var i : uint = 0; i < k.size(); i++) {
        var c = k.get(i);
        if(c >= 'A' && c <= 'Z') {
            out.append('-');
            c = c + 32;
        }
        out.append(c);
    }
    return true;
}

func append_escaped_single_quoted(out : &mut std::string, value : std::string_view) {
    for(var i : uint = 0; i < value.size(); i++) {
        const c = value.get(i);
        switch(c) {
            '\\' => out.append_view("\\\\")
            '\'' => out.append_view("\\'")
            '\n' => out.append_view("\\n")
            '\r' => out.append_view("\\r")
            '\t' => out.append_view("\\t")
            default => out.append(c)
        }
    }
}

func build_child_path(builder : *mut ASTBuilder, parentPath : std::string_view, childIndex : uint) : std::string_view {
    var p = std::string();
    if(parentPath.size() <= 2) {
        p.append_view("[");
        p.append_integer(childIndex as bigint);
        p.append_view("]");
    } else {
        p.append_view(parentPath.subview(0, parentPath.size() - 1));
        p.append_view(",");
        p.append_integer(childIndex as bigint);
        p.append_view("]");
    }
    return builder.allocate_view(p.to_view());
}
