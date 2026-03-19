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
        if(d.name.equals_text(name)) {
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

func writePrimitiveAttrValue(output : &mut std::string, attrVal : &SsrAttributeValue, space_multiple : bool = false) {
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
                writePrimitiveAttrValue(output, *curr)
                curr++
                if(space_multiple && curr != end) { output.append(' '); }
            }
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
}

func (page : &mut HtmlPage) renderHtmlAttrsInternal(list : &SsrAttributeList, special : &mut SpecialAttrs) {
    var output = &mut page.pageHtml
    var d = list.data
    const end = d + list.size

    while(d != end) {
        switch(d.value) {
            Spread(value) => {
                // Recursively pass the accumulator down the spread tree
                page.renderHtmlAttrsInternal(value, special)
            }
            default => {
                // Accumulate special attributes; stream the rest immediately
                if (d.name.equals("class")) {
                    special.classes[special.class_count] = &d.value
                    special.class_count++
                } else if (d.name.equals("style")) {
                    special.styles[special.style_count] = &d.value
                    special.style_count++
                } else {
                    output.append(' ');
                    output.append_with_len(d.name.data, d.name.size)
                    output.append_view("=\"")
                    writePrimitiveAttrValue(*output, d.value)
                    output.append_view("\"")
                }
            }
        }
        d++;
    }
}

public func renderHtmlAttrs(page : &mut HtmlPage, list : &SsrAttributeList) {
    var special = zeroed<SpecialAttrs>()
    page.renderHtmlAttrsInternal(list, special)

    var output = &mut page.pageHtml

    // 1. Render merged classes
    if (special.class_count > 0) {
        output.append_view(" class=\"")
        for (var i = 0; i < special.class_count; i++) {
            if (i > 0) output.append(' ') // Space-separated classes
            writePrimitiveAttrValue(*output, *special.classes[i])
        }
        output.append_view("\"")
    }

    // 2. Render merged styles
    if (special.style_count > 0) {
        output.append_view(" style=\"")
        for (var i = 0; i < special.style_count; i++) {
            if (i > 0) output.append(';') // Semicolon-separated styles
            writePrimitiveAttrValue(*output, *special.styles[i])
        }
        output.append_view("\"")
    }
}

func (page : &mut HtmlPage) renderJsAttrsInternal(list : &SsrAttributeList, special : &mut SpecialAttrs, is_first : &mut bool) {
    var output = &mut page.pageJs
    var d = list.data
    const end = d + list.size

    while(d != end) {
        switch(d.value) {
            Spread(value) => {
                page.renderJsAttrsInternal(value, special, is_first)
            }
            default => {
                if (d.name.equals("class")) {
                    special.classes[special.class_count] = &d.value
                    special.class_count++
                } else if (d.name.equals("style")) {
                    special.styles[special.style_count] = &d.value
                    special.style_count++
                } else {
                    // Safe comma placement
                    if (!*is_first) output.append_view(", ")
                    *is_first = false

                    output.append_with_len(d.name.data, d.name.size)
                    output.append_view(":\"")
                    writePrimitiveAttrValue(*output, d.value)
                    output.append('"')
                }
            }
        }
        d++;
    }
}

public func renderJsAttrs(page : &mut HtmlPage, list : &SsrAttributeList) {
    var special = zeroed<SpecialAttrs>()
    var is_first = true
    page.renderJsAttrsInternal(list, special, is_first)

    var output = &mut page.pageJs

    if (special.class_count > 0) {
        if (!is_first) output.append_view(", ")
        output.append_view("class:\"")
        for (var i = 0; i < special.class_count; i++) {
            if (i > 0) output.append(' ')
            writePrimitiveAttrValue(*output, *special.classes[i])
        }
        output.append('"')
        is_first = false
    }

    if (special.style_count > 0) {
        if (!is_first) output.append_view(", ")
        output.append_view("style:\"")
        for (var i = 0; i < special.style_count; i++) {
            if (i > 0) output.append(';')
            writePrimitiveAttrValue(*output, *special.styles[i])
        }
        output.append('"')
    }
}

public func renderHtmlAttrValue(page : &mut HtmlPage, attrVal : &SsrAttributeValue) {
    writePrimitiveAttrValue(page.pageHtml, attrVal)
}

public func renderJsAttrValue(page : &mut HtmlPage, attrVal : &SsrAttributeValue) {
    writePrimitiveAttrValue(page.pageJs, attrVal)
}

// this function allows libraries like preact, solid and react to move rendered html to js buffer
// then they send the html to a function that creates an element out of it
// then they hydrate that element, this is usually done for a universal component that renders html for ssr
public func move_html_to_js_with_lambda_start(page : &mut HtmlPage, index : size_t) {
    page.pageHeadJs.append_view("\n(() => { const html = `");
    page.pageHeadJs.append_with_len(page.pageHtml.data() + index, page.pageHtml.size() - index)
    page.pageHtml.resize_unsafe(index)
    page.pageHeadJs.append_view("`;")
    // after this the caller would write something like
    // create_and_hydrate(html, ComponentFunction, MissingAttributesObject)
}