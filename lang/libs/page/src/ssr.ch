public variant SsrAttributeValue {
	Boolean(value : bool)
	Char(value : char)
    UInteger(value : ubigint)
	Integer(value : bigint)
	Double(value : double, precision : int)
    Text(value : std::string_view)
	Spread(value : SsrAttributeList)
}

public struct SsrAttribute {
    var name : std::string_view
    var value : SsrAttributeValue
}

public struct SsrAttributeList {
	var data : *SsrAttribute
	var size : u64
}

func writePrimitiveAttrValue(output : &mut std::string, attrVal : &SsrAttributeValue) {
    switch(attrVal) {
        Boolean(value) => {
            if(value) output.append_view("true") else output.append_view("false")
        }
        Char(value) => output.append(value)
        UInteger(value) => output.append_uinteger(value)
        Integer(value) => output.append_integer(value)
        Double(value, precision) => output.append_double(value, precision)
        Text(value) => output.append_view(value)
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
                    output.append_view(d.name)
                    output.append_view("=\"")
                    writePrimitiveAttrValue(page.pageHtml, d.value)
                    output.append_view("\" ")
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
        output.append_view("class=\"")
        for (var i = 0; i < special.class_count; i++) {
            if (i > 0) output.append(' ') // Space-separated classes
            writePrimitiveAttrValue(page.pageHtml, *special.classes[i])
        }
        output.append_view("\" ")
    }

    // 2. Render merged styles
    if (special.style_count > 0) {
        output.append_view("style=\"")
        for (var i = 0; i < special.style_count; i++) {
            if (i > 0) output.append(';') // Semicolon-separated styles
            writePrimitiveAttrValue(page.pageJs, *special.styles[i])
        }
        output.append_view("\" ")
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

                    output.append_view(d.name)
                    output.append_view(":\"")
                    writePrimitiveAttrValue(page.pageJs, d.value)
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
            writePrimitiveAttrValue(page.pageJs, *special.classes[i])
        }
        output.append('"')
        is_first = false
    }

    if (special.style_count > 0) {
        if (!is_first) output.append_view(", ")
        output.append_view("style:\"")
        for (var i = 0; i < special.style_count; i++) {
            if (i > 0) output.append(';')
            writePrimitiveAttrValue(page.pageJs, *special.styles[i])
        }
        output.append('"')
    }
}