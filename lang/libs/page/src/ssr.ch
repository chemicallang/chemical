public struct SsrText {
    var data : *char
    var size : u64
    func equals(&self, other : &std::string_view) : bool {
        return size == other.size() && strncmp(data, other.data(), size) == 0;
    }
    func equals_text(&self, other : &SsrText) : bool {
        return size == other.size && strncmp(data, other.data, size) == 0;
    }
}

public struct MultipleAttributeValues {
    var data : *SsrAttributeValue
    var size : u64
}

public struct SsrCallable {
    var fn_ptr : (object : *void, page : &mut HtmlPage, buffer : &mut std::string) => void
    var object : *void
}

public variant SsrAttributeValue {
    None()
	Boolean(value : bool)
	Char(value : char)
    UInteger(value : ubigint)
	Integer(value : bigint)
	Double(value : double, precision : int = 2)
    Text(value : SsrText)
    PtrChar(value : *char)
    Multiple(value : MultipleAttributeValues)
	Spread(value : SsrAttributeList)
	Callable(value : SsrCallable)
}

public struct SsrAttribute {
    var name : SsrText
    var value : SsrAttributeValue
}

public struct SsrAttributeList {
	var data : *SsrAttribute
	var size : u64
}

public func getSsrAttributeValue(list : SsrAttributeList, name : SsrText) : SsrAttributeValue {
    var d = list.data
    const end = d + list.size
    while(d != end) {
        if(d.name.equals_text(&name)) {
            // gets copied
            return d.value;
        } else if(d.value is SsrAttributeValue.Spread) {
            var Spread(value) = d.value else unreachable;
            const found = getSsrAttributeValue(value, name)
            if(found !is SsrAttributeValue.None) {
                return found;
            }
        }
        d++;
    }
    return SsrAttributeValue.None()
}

public func isSsrAttributeValueTruthy(val : &SsrAttributeValue) : bool {
    switch(val) {
        None() => return false
        Boolean(v) => return v
        Char(v) => return v != '\0'
        UInteger(v) => return v != 0
        Integer(v) => return v != 0
        Double(v, _) => return v != 0.0
        Text(v) => return v.size > 0
        PtrChar(v) => return v != null && v[0] != '\0'
        Multiple(v) => return v.size > 0
        Spread(_) => return true
        Callable(_) => return true
        default => return false
    }
}

func writePrimitiveAttrValue(page : &mut HtmlPage, output : &mut std::string, attrVal : &SsrAttributeValue) {
    switch(attrVal) {
        None() => {
            output.append_view("null")   
        }
        Boolean(value) => {
            if(value) output.append_view("true") else output.append_view("false")
        }
        Char(value) => output.append(value)
        UInteger(value) => output.append_uinteger(value)
        Integer(value) => output.append_integer(value)
        Double(value, precision) => output.append_double(value, precision)
        Text(value) => output.append_with_len(value.data, value.size)
        PtrChar(value) => output.append_char_ptr(value)
        Multiple(value) => {
            var curr = value.data
            const end = curr + value.size
            while(curr != end) {
                writePrimitiveAttrValue(page, output, &*curr)
                curr++
            }
        }
        Callable(value) => {
            value.fn_ptr(value.object, page, output)
        }
        Spread(value) => {} // Unreachable for primitive values
    }
}

// Stack-allocated accumulator for maximum speed.
struct SpecialAttrs {
    var classes : [32]*SsrAttributeValue
    var class_count : u32 = 0
    var styles : [32]*SsrAttributeValue
    var style_count : u32 = 0
    // Deferred non-special attributes with last-wins dedup
    var others_names : [64]SsrText
    var others_values : [64]*SsrAttributeValue
    var others_count : u32 = 0
}

func (page : &mut HtmlPage) renderHtmlAttrsInternal(list : &SsrAttributeList, special : &mut SpecialAttrs) {
    var d = list.data
    const end = d + list.size

    while(d != end) {
        switch(d.value) {
            Spread(value) => {
                // Recursively pass the accumulator down the spread tree
                page.renderHtmlAttrsInternal(&value, special)
            }
            default => {
                // Accumulate special attributes; defer the rest for dedup
                if (d.name.equals("class")) {
                    special.classes[special.class_count] = &raw d.value
                    special.class_count++
                } else if (d.name.equals("style")) {
                    special.styles[special.style_count] = &raw d.value
                    special.style_count++
                } else {
                    if(d.value is SsrAttributeValue.None) {
                        d++;
                        continue;
                    }
                    if(d.value is SsrAttributeValue.Boolean) {
                        var Boolean(value) = d.value else unreachable;
                        if(!value) {
                            d++;
                            continue;
                        }
                    }
                    // Last-wins dedup: replace existing or append
                    var found = false
                    for(var j = 0; j < special.others_count; j++) {
                        if(special.others_names[j].equals_text(&d.name)) {
                            special.others_values[j] = &raw d.value
                            found = true
                            break
                        }
                    }
                    if(!found) {
                        special.others_names[special.others_count] = d.name
                        special.others_values[special.others_count] = &raw d.value
                        special.others_count++
                    }
                }
            }
        }
        d++;
    }
}

public func renderHtmlAttrs(page : &mut HtmlPage, list : &SsrAttributeList) {
    var special = zeroed<SpecialAttrs>()
    page.renderHtmlAttrsInternal(list, &mut special)

    var output = &mut page.pageHtml

    // 1. Render merged classes
    if (special.class_count > 0) {
        output.append_view(" class=\"")
        for (var i = 0; i < special.class_count; i++) {
            if (i > 0) output.append(' ') // Space-separated classes
            writePrimitiveAttrValue(page, output, &*special.classes[i])
        }
        output.append_view("\"")
    }

    // 2. Render merged styles
    if (special.style_count > 0) {
        output.append_view(" style=\"")
        for (var i = 0; i < special.style_count; i++) {
            if (i > 0) output.append(';') // Semicolon-separated styles
            writePrimitiveAttrValue(page, &mut *output, &*special.styles[i])
        }
        output.append_view("\"")
    }

    // 3. Render deferred non-special attributes (already dedup'd, last-wins)
    for (var i = 0; i < special.others_count; i++) {
        output.append(' ')
        output.append_with_len(special.others_names[i].data, special.others_names[i].size)
        output.append_view("=\"")
        writePrimitiveAttrValue(page, &mut *output, &mut *special.others_values[i])
        output.append_view("\"")
    }
}

func writeJsPrimitiveAttrValue(page : &mut HtmlPage, output : &mut std::string, attrVal : &SsrAttributeValue) {
    switch(attrVal) {
        None() => {
            output.append_view("null")
        }
        Boolean(value) => {
            if(value) output.append_view("true") else output.append_view("false")
        }
        Char(value) => {
            output.append('\'');
            output.append(value)
            output.append('\'');
        }
        UInteger(value) => output.append_uinteger(value)
        Integer(value) => output.append_integer(value)
        Double(value, precision) => output.append_double(value, precision)
        Text(value) => {
            output.append('"');
            output.append_with_len(value.data, value.size)
            output.append('"');
        }
        PtrChar(value) => {
            output.append('"');
            output.append_char_ptr(value)
            output.append('"');
        }
        Multiple(value) => {
            var curr = value.data
            const end = curr + value.size
            while(curr != end) {
                writeJsPrimitiveAttrValue(page, output, &*curr)
                curr++
            }
        }
        Callable(value) => {
            value.fn_ptr(value.object, page, output)
        }
        Spread(value) => {} // Unreachable for primitive values
    }
}

func (page : &mut HtmlPage) renderJsAttrsInternal(list : &SsrAttributeList, special : &mut SpecialAttrs, is_first : &mut bool) {
    var d = list.data
    const end = d + list.size

    while(d != end) {
        switch(d.value) {
            Spread(value) => {
                page.renderJsAttrsInternal(&value, special, is_first)
            }
            default => {
                if (d.name.equals("class")) {
                    special.classes[special.class_count] = &raw d.value
                    special.class_count++
                } else if (d.name.equals("style")) {
                    special.styles[special.style_count] = &raw d.value
                    special.style_count++
                } else {
                    if(d.value is SsrAttributeValue.None) {
                        d++;
                        continue;
                    }
                    if(d.value is SsrAttributeValue.Boolean) {
                        var Boolean(value) = d.value else unreachable;
                        if(!value) {
                            d++;
                            continue;
                        }
                    }
                    // Last-wins dedup: replace existing or append
                    var found = false
                    for(var j = 0; j < special.others_count; j++) {
                        if(special.others_names[j].equals_text(&d.name)) {
                            special.others_values[j] = &raw d.value
                            found = true
                            break
                        }
                    }
                    if(!found) {
                        special.others_names[special.others_count] = d.name
                        special.others_values[special.others_count] = &raw d.value
                        special.others_count++
                    }
                }
            }
        }
        d++;
    }
}

public func renderJsAttrs(page : &mut HtmlPage, list : &SsrAttributeList) {
    var special = zeroed<SpecialAttrs>()
    var is_first = true
    page.renderJsAttrsInternal(list, &mut special, &mut is_first)

    var output = &mut page.pageJs

    // 1. Render merged classes
    if (special.class_count > 0) {
        if (!is_first) output.append_view(", ")
        output.append_view("class:\"")
        for (var i = 0; i < special.class_count; i++) {
            if (i > 0) output.append(' ')
            writeJsPrimitiveAttrValue(page, &mut *output, &mut *special.classes[i])
        }
        output.append('"')
        is_first = false
    }

    // 2. Render merged styles
    if (special.style_count > 0) {
        if (!is_first) output.append_view(", ")
        output.append_view("style:\"")
        for (var i = 0; i < special.style_count; i++) {
            if (i > 0) output.append(';')
            writeJsPrimitiveAttrValue(page, &mut *output, &*special.styles[i])
        }
        output.append('"')
        is_first = false
    }

    // 3. Render deferred non-special attributes (already dedup'd, last-wins)
    for (var i = 0; i < special.others_count; i++) {
        if (!is_first) output.append_view(", ")
        is_first = false
        output.append_with_len(special.others_names[i].data, special.others_names[i].size)
        output.append(':')
        writeJsPrimitiveAttrValue(page, &mut *output, &*special.others_values[i])
    }
}

public func renderHtmlAttrValue(page : &mut HtmlPage, attrVal : &SsrAttributeValue) {
    writePrimitiveAttrValue(page, &mut page.pageHtml, attrVal)
}

public func renderHtmlChildValue(page : &mut HtmlPage, attrVal : &SsrAttributeValue) {
    switch(attrVal) {
        None() => {}
        Boolean(_) => {}
        default => writePrimitiveAttrValue(page, &mut page.pageHtml, attrVal)
    }
}

public func renderJsAttrValue(page : &mut HtmlPage, attrVal : &SsrAttributeValue) {
    writeJsPrimitiveAttrValue(page, &mut page.pageJs, attrVal)
}

public func renderCssAttrValue(page : &mut HtmlPage, attrVal : &SsrAttributeValue) {
    writeJsPrimitiveAttrValue(page, &mut page.pageCss, attrVal)
}

// this function allows libraries like preact, solid and react to move rendered html to js buffer
// then they send the html to a function that creates an element out of it
// then they hydrate that element, this is usually done for a universal component that renders html for ssr
public func move_html_to_js_with_lambda_start(page : &mut HtmlPage, index : size_t) {
    page.pageHeadJs.append_view("\n(() => { const html = `");
    const delta_size = page.pageHtml.size() - index;
    const delta = page.pageHtml.data() + index;
    for(var i = 0u; i < delta_size; i++) {
        const c = delta[i];
        if(c == '`') page.pageHeadJs.append_view("\\`")
        else if(c == '$' && i + 1 < delta_size && delta[i+1] == '{') page.pageHeadJs.append_view("\\$")
        else if(c == '\\') page.pageHeadJs.append_view("\\\\")
        else page.pageHeadJs.append(c)
    }
    page.pageHtml.resize(index)
    page.pageHeadJs.append_view("`;")
    // after this the caller would write something like
    // create_and_hydrate(html, ComponentFunction, MissingAttributesObject)
}
