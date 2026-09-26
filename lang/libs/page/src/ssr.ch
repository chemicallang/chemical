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

// Serializes an `SsrText` payload as an attribute value. Lets a struct-valued
// expression be used as an SSR attribute (the universal converter looks this
// method up on the struct declaration). `public` so a backend that drops
// unreferenced internal functions still emits it for generated call sites.
public func (t : &SsrText) getSsrAttributeValue(page : &mut HtmlPage) : SsrAttributeValue {
    return SsrAttributeValue.Text(*t)
}

public struct SsrAttributeList {
	var data : *SsrAttribute
	var size : u64
}

// Extracts the array payload from an attribute value, returning an empty list
// when the value is not a Multiple (array). Lets SSR iterate `.map()` sources
// (props arrays, state arrays) uniformly at runtime.
public func getMultipleAttributeValues(val : &SsrAttributeValue) : MultipleAttributeValues {
    switch(val) {
        Multiple(v) => return v
        default => return MultipleAttributeValues { data: null, size: 0 }
    }
}

// Fetches a single element from a Multiple array at runtime. Kept as a
// function (rather than an inline `data[i]` expression in generated code) so
// the SSR evaluator can emit it as a normal, fully type-resolved call — the
// generated SSR function bodies bypass symres type-determination, which
// left the index operator's type unresolved and crashed LLVM codegen.
public func ssrMultipleGet(values : MultipleAttributeValues, index : ubigint) : SsrAttributeValue {
    if(index < values.size) {
        return values.data[index]
    }
    return SsrAttributeValue.None()
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

public func ssrAttrValueProp(val : &SsrAttributeValue, name : SsrText) : SsrAttributeValue {
    switch(val) {
        Spread(list) => return getSsrAttributeValue(list, name)
        default => return SsrAttributeValue.None()
    }
}

// Text view of an attribute value for runtime string predicates
// (`.includes`, `.startsWith`, `.endsWith`). Only text-like values resolve;
// numeric/other values yield an empty view.
func ssrTextOf(val : &SsrAttributeValue) : SsrText {
    switch(val) {
        Text(v) => return v
        PtrChar(v) => return SsrText { data: v, size: strlen(v) }
        default => return SsrText { data: null, size: 0 }
    }
}

// Runtime equivalents of the compile-time predicate operations, used when a
// `.filter()` predicate runs over a props/runtime array at SSR time.
public func ssrTextIncludes(hay : &SsrAttributeValue, needle : &SsrAttributeValue) : bool {
    const h = ssrTextOf(hay)
    const n = ssrTextOf(needle)
    if(n.size == 0) return true
    if(h.size < n.size) return false
    var i : u64 = 0
    while(i + n.size <= h.size) {
        if(strncmp(h.data + i, n.data, n.size) == 0) return true
        i = i + 1
    }
    return false
}

public func ssrTextStartsWith(hay : &SsrAttributeValue, needle : &SsrAttributeValue) : bool {
    const h = ssrTextOf(hay)
    const n = ssrTextOf(needle)
    if(n.size > h.size) return false
    return strncmp(h.data, n.data, n.size) == 0
}

public func ssrTextEndsWith(hay : &SsrAttributeValue, needle : &SsrAttributeValue) : bool {
    const h = ssrTextOf(hay)
    const n = ssrTextOf(needle)
    if(n.size > h.size) return false
    return strncmp(h.data + (h.size - n.size), n.data, n.size) == 0
}

// ASCII case folding for `.toLowerCase()` predicates evaluated at SSR. Only
// ASCII A-Z is folded (matching the compile-time static predicate evaluator).
func ssr_ascii_lower(c : char) : char {
    if(c >= 'A' && c <= 'Z') return (c + 32) as char
    return c
}

func ssr_text_region_fold_equals(a : *char, b : *char, len : u64) : bool {
    var i : u64 = 0
    while(i < len) {
        if(ssr_ascii_lower(a[i]) != ssr_ascii_lower(b[i])) return false
        i = i + 1
    }
    return true
}

public func ssrTextIncludesFold(hay : &SsrAttributeValue, needle : &SsrAttributeValue) : bool {
    const h = ssrTextOf(hay)
    const n = ssrTextOf(needle)
    if(n.size == 0) return true
    if(h.size < n.size) return false
    var i : u64 = 0
    while(i + n.size <= h.size) {
        if(ssr_text_region_fold_equals(h.data + i, n.data, n.size)) return true
        i = i + 1
    }
    return false
}

public func ssrTextStartsWithFold(hay : &SsrAttributeValue, needle : &SsrAttributeValue) : bool {
    const h = ssrTextOf(hay)
    const n = ssrTextOf(needle)
    if(n.size > h.size) return false
    return ssr_text_region_fold_equals(h.data, n.data, n.size)
}

public func ssrTextEndsWithFold(hay : &SsrAttributeValue, needle : &SsrAttributeValue) : bool {
    const h = ssrTextOf(hay)
    const n = ssrTextOf(needle)
    if(n.size > h.size) return false
    return ssr_text_region_fold_equals(h.data + (h.size - n.size), n.data, n.size)
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

// Compares an attribute value's text content against an expected literal.
// Used by the SSR evaluator for `props.x === "lit"` style expressions.
public func ssrTextEquals(val : &SsrAttributeValue, expected : SsrText) : bool {
    switch(val) {
        // An unset value equals "undefined"/"null" in JS semantics, so
        // `props.label !== undefined` correctly evaluates to false for unset.
        None() => return (expected.size == 0) ||
            (expected.size == 9 && strncmp(expected.data, "undefined", 9) == 0) ||
            (expected.size == 4 && strncmp(expected.data, "null", 4) == 0)
        Text(v) => return v.equals_text(&expected)
        PtrChar(v) => {
            const len = strlen(v)
            return len == expected.size && strncmp(v, expected.data, expected.size) == 0
        }
        Boolean(v) => {
            if(v) return expected.size == 4 && strncmp(expected.data, "true", 4) == 0
            return expected.size == 5 && strncmp(expected.data, "false", 5) == 0
        }
        Char(v) => return expected.size == 1 && expected.data[0] == v
        UInteger(v) => {
            // Numeric equality against the decimal representation (JS loose ==).
            var tmp = std::string()
            tmp.append_uinteger(v)
            return expected.size == tmp.size() && strncmp(expected.data, tmp.data(), expected.size) == 0
        }
        Integer(v) => {
            var tmp = std::string()
            tmp.append_integer(v)
            return expected.size == tmp.size() && strncmp(expected.data, tmp.data(), expected.size) == 0
        }
        Double(v, precision) => {
            // Compare against the formatted representation, stripping trailing
            // zeros so "2.00" matches "2" (JS loose equality).
            var tmp = std::string()
            tmp.append_double(v, precision)
            var end = tmp.size()
            while(end > 0 && tmp.get(end - 1) == '0') { end-- }
            if(end > 0 && tmp.get(end - 1) == '.') { end-- }
            if(end == 0) return false
            const view = std::string_view(tmp.data(), end)
            return expected.size == view.size() && strncmp(expected.data, view.data(), view.size()) == 0
        }
        Multiple(_) => return false
        Spread(_) => return false
        Callable(_) => return false
        default => return false
    }
}

// Canonical text representation of an attribute value, used by ssrValuesEqual.
// None maps to the empty string; numeric/boolean values map to their decimal
// representation (JS loose `==` coercion); Multiple/Spread/Callable are not
// comparable. Returns false for non-comparable values.
func appendSsrValueText(val : &SsrAttributeValue, out : &mut std::string) : bool {
    switch(val) {
        None() => return true
        Boolean(v) => {
            if(v) out.append_view("true") else out.append_view("false")
            return true
        }
        Char(v) => {
            out.append(v)
            return true
        }
        UInteger(v) => {
            out.append_uinteger(v)
            return true
        }
        Integer(v) => {
            out.append_integer(v)
            return true
        }
        Double(v, precision) => {
            out.append_double(v, precision)
            var end = out.size()
            while(end > 0 && out.get(end - 1) == '0') { end-- }
            if(end > 0 && out.get(end - 1) == '.') { end-- }
            if(end == 0) out.append('0')
            else out.resize(end)
            return true
        }
        Text(v) => {
            out.append_view(std::string_view(v.data, v.size))
            return true
        }
        PtrChar(v) => {
            if(v == null) return true
            out.append_view(std::string_view(v, strlen(v)))
            return true
        }
        default => return false
    }
}

// Loose equality between two attribute values (JS `==` semantics for the value
// kinds the SSR evaluator produces). Used for `active == index`-style runtime
// comparisons where the right side is another attribute value rather than a
// literal (loop indices, bound map items, state values).
public func ssrValuesEqual(left : &SsrAttributeValue, right : &SsrAttributeValue) : bool {
    var a = std::string()
    var b = std::string()
    if(!appendSsrValueText(left, &mut a)) return false
    if(!appendSsrValueText(right, &mut b)) return false
    return a.equals(&b)
}

// Picks between two attribute values based on a runtime condition.
// Used by the SSR evaluator for ternary expressions like
// `class={props.variant === "primary" ? "a" : "b"}`.
public func ssrPickValue(cond : bool, ifTrue : SsrAttributeValue, ifFalse : SsrAttributeValue) : SsrAttributeValue {
    if(cond) return ifTrue else return ifFalse
}

// Wraps a static text into an attribute value.
// Kept as a function (rather than a direct variant construction) so the SSR
// evaluator can nest it inside ssrPickValue/ssrConcatAttrValues calls.
public func ssrMakeTextValue(text : SsrText) : SsrAttributeValue {
    return SsrAttributeValue.Text(text)
}

public func ssrMakeBoolValue(value : bool) : SsrAttributeValue {
    return SsrAttributeValue.Boolean(value)
}

public func ssrMakeUIntegerValue(value : ubigint) : SsrAttributeValue {
    return SsrAttributeValue.UInteger(value)
}

public func ssrMakeIntegerValue(value : bigint) : SsrAttributeValue {
    return SsrAttributeValue.Integer(value)
}

public func ssrMakeMultipleValue(value : MultipleAttributeValues) : SsrAttributeValue {
    return SsrAttributeValue.Multiple(value)
}

public func ssrNoneValue() : SsrAttributeValue {
    return SsrAttributeValue.None()
}

func appendHtmlEscaped(output : &mut std::string, text : &std::string_view) {
    for(var i = 0u; i < text.size(); i++) {
        const c = text.data()[i];
        switch(c) {
            '&' => output.append_view("&amp;")
            '<' => output.append_view("&lt;")
            '>' => output.append_view("&gt;")
            '"' => output.append_view("&quot;")
            '\'' => output.append_view("&#39;")
            default => output.append(c)
        }
    }
}

func appendJsHex2(output : &mut std::string, v : uint) {
    const hex = "0123456789abcdef"
    output.append(hex[(v >> 4) & 0xF]);
    output.append(hex[v & 0xF]);
}

func appendJsEscaped(output : &mut std::string, text : &std::string_view) {
    for(var i = 0u; i < text.size(); i++) {
        const c = text.data()[i];
        switch(c) {
            '"' => output.append_view("\\\"")
            '\\' => output.append_view("\\\\")
            '\n' => output.append_view("\\n")
            '\r' => output.append_view("\\r")
            '\t' => output.append_view("\\t")
            '<' => {
                // Defensively escape `</` so values cannot break out of an
                // inline <script> block (semantically identical in JS strings).
                if(i + 1 < text.size() && text.data()[i+1] == '/') {
                    output.append_view("\\u003C/");
                    i++;
                } else {
                    output.append(c);
                }
            }
            default => {
                if((c as u8) < 0x20) {
                    output.append_view("\\u00");
                    appendJsHex2(output, c as uint);
                } else {
                    output.append(c);
                }
            }
        }
    }
}

enum AttrValueTarget {
    Html,
    Js
}

// Single attribute-value writer, parameterized by output target and escaping
// policy. HTML emits unquoted, HTML-escaped text (the caller adds the attribute
// quotes); JS emits quoted JS-escaped text / char literals and `undefined` for
// None. This replaces the two near-duplicate writers.
func writeAttrValue(page : &mut HtmlPage, output : &mut std::string, attrVal : &SsrAttributeValue, target : AttrValueTarget) {
    switch(attrVal) {
        None() => {
            // Unresolvable value: HTML degrades to nothing; JS props become undefined
            if(target == AttrValueTarget.Js) output.append_view("undefined")
        }
        Boolean(value) => {
            if(value) output.append_view("true") else output.append_view("false")
        }
        Char(value) => {
            if(target == AttrValueTarget.Js) {
                output.append('\'')
                if(value == '\'') output.append_view("\\'")
                else if(value == '\\') output.append_view("\\\\")
                else output.append(value)
                output.append('\'')
            } else {
                output.append(value)
            }
        }
        UInteger(value) => output.append_uinteger(value)
        Integer(value) => output.append_integer(value)
        Double(value, precision) => output.append_double(value, precision)
        Text(value) => {
            const view = std::string_view(value.data, value.size)
            if(target == AttrValueTarget.Js) {
                output.append('"')
                appendJsEscaped(output, &view)
                output.append('"')
            } else {
                appendHtmlEscaped(output, &view)
            }
        }
        PtrChar(value) => {
            const view = std::string_view(value, strlen(value))
            if(target == AttrValueTarget.Js) {
                output.append('"')
                appendJsEscaped(output, &view)
                output.append('"')
            } else {
                appendHtmlEscaped(output, &view)
            }
        }
        Multiple(value) => {
            var curr = value.data
            const end = curr + value.size
            while(curr != end) {
                writeAttrValue(page, output, &*curr, target)
                curr++
            }
        }
        Callable(value) => {
            value.fn_ptr(value.object, page, output)
        }
        Spread(value) => {} // Unreachable for primitive values
    }
}

func writePrimitiveAttrValue(page : &mut HtmlPage, output : &mut std::string, attrVal : &SsrAttributeValue) {
    writeAttrValue(page, output, attrVal, AttrValueTarget.Html)
}

func writeJsPrimitiveAttrValue(page : &mut HtmlPage, output : &mut std::string, attrVal : &SsrAttributeValue) {
    writeAttrValue(page, output, attrVal, AttrValueTarget.Js)
}

// Attribute accumulator: merges class/style and deduplicates the rest
// (last-wins). Growable storage — no fixed limits, no silent truncation.
struct SpecialAttrs {
    var classes : std::vector<*SsrAttributeValue>
    var styles : std::vector<*SsrAttributeValue>
    var others_names : std::vector<SsrText>
    var others_values : std::vector<*SsrAttributeValue>
}

func make_special_attrs() : SpecialAttrs {
    return SpecialAttrs {
        classes : std::vector<*SsrAttributeValue>(),
        styles : std::vector<*SsrAttributeValue>(),
        others_names : std::vector<SsrText>(),
        others_values : std::vector<*SsrAttributeValue>()
    }
}

func accumulateAttrs(list : &SsrAttributeList, special : &mut SpecialAttrs) {
    var d = list.data
    const end = d + list.size
    while(d != end) {
        switch(d.value) {
            Spread(value) => {
                // Recursively accumulate the spread's attributes
                accumulateAttrs(&value, special)
            }
            default => {
                if(d.value is SsrAttributeValue.None) {
                    d++
                    continue
                }
                // "key" is a reconciliation hint — never rendered to DOM or HTML
                if(d.name.equals("key")) {
                    d++
                    continue
                }
                if(d.name.equals("class")) {
                    special.classes.push(&raw d.value)
                } else if(d.name.equals("style")) {
                    special.styles.push(&raw d.value)
                } else {
                    if(d.value is SsrAttributeValue.Boolean) {
                        var Boolean(value) = d.value else unreachable
                        if(!value) {
                            d++
                            continue
                        }
                    }
                    // Last-wins dedup: replace existing or append
                    var found = false
                    for(var j : size_t = 0; j < special.others_names.size(); j++) {
                        if(special.others_names.get(j).equals_text(&d.name)) {
                            special.others_values.set(j, &raw d.value)
                            found = true
                            break
                        }
                    }
                    if(!found) {
                        special.others_names.push(d.name)
                        special.others_values.push(&raw d.value)
                    }
                }
            }
        }
        d++
    }
}

public func renderHtmlAttrs(page : &mut HtmlPage, list : &SsrAttributeList) {
    var special = make_special_attrs()
    accumulateAttrs(list, &mut special)

    var output = &mut page.pageHtml

    // 1. Render merged classes
    if (special.classes.size() > 0) {
        output.append_view(" class=\"")
        for (var i : size_t = 0; i < special.classes.size(); i++) {
            if (i > 0) output.append(' ') // Space-separated classes
            writePrimitiveAttrValue(page, output, &*special.classes.get(i))
        }
        output.append_view("\"")
    }

    // 2. Render merged styles
    if (special.styles.size() > 0) {
        output.append_view(" style=\"")
        for (var i : size_t = 0; i < special.styles.size(); i++) {
            if (i > 0) output.append(';') // Semicolon-separated styles
            writePrimitiveAttrValue(page, &mut *output, &*special.styles.get(i))
        }
        output.append_view("\"")
    }

    // 3. Render deferred non-special attributes (already dedup'd, last-wins)
    for (var i : size_t = 0; i < special.others_names.size(); i++) {
        output.append(' ')
        output.append_with_len(special.others_names.get(i).data, special.others_names.get(i).size)
        output.append_view("=\"")
        writePrimitiveAttrValue(page, &mut *output, &*special.others_values.get(i))
        output.append_view("\"")
    }
}

// Like `renderHtmlAttrs`, but prepends `baseClass` to the element's class list.
// Used by #styled components so the generated CSS class is applied alongside any
// user-provided `class` attribute.
public func renderHtmlAttrsWithBase(page : &mut HtmlPage, list : &SsrAttributeList, baseClass : SsrText) {
    var special = make_special_attrs()
    accumulateAttrs(list, &mut special)

    var output = &mut page.pageHtml

    // 1. Render merged classes (baseClass first, then user classes)
    if (special.classes.size() > 0 || baseClass.size > 0) {
        output.append_view(" class=\"")
        var first = true
        if (baseClass.size > 0) {
            output.append_with_len(baseClass.data, baseClass.size)
            first = false
        }
        for (var i : size_t = 0; i < special.classes.size(); i++) {
            if (!first) output.append(' ') // Space-separated classes
            first = false
            writePrimitiveAttrValue(page, output, &*special.classes.get(i))
        }
        output.append_view("\"")
    }

    // 2. Render merged styles
    if (special.styles.size() > 0) {
        output.append_view(" style=\"")
        for (var i : size_t = 0; i < special.styles.size(); i++) {
            if (i > 0) output.append(';') // Semicolon-separated styles
            writePrimitiveAttrValue(page, &mut *output, &*special.styles.get(i))
        }
        output.append_view("\"")
    }

    // 3. Render deferred non-special attributes (already dedup'd, last-wins)
    for (var i : size_t = 0; i < special.others_names.size(); i++) {
        output.append(' ')
        output.append_with_len(special.others_names.get(i).data, special.others_names.get(i).size)
        output.append_view("=\"")
        writePrimitiveAttrValue(page, &mut *output, &*special.others_values.get(i))
        output.append_view("\"")
    }
}

public func renderJsAttrs(page : &mut HtmlPage, list : &SsrAttributeList) {
    var special = make_special_attrs()
    accumulateAttrs(list, &mut special)

    var output = &mut page.pageJs
    var is_first = true

    // 1. Render merged classes
    if (special.classes.size() > 0) {
        if (!is_first) output.append_view(", ")
        output.append_view("class:\"")
        for (var i : size_t = 0; i < special.classes.size(); i++) {
            if (i > 0) output.append(' ')
            writeJsPrimitiveAttrValue(page, &mut *output, &*special.classes.get(i))
        }
        output.append('"')
        is_first = false
    }

    // 2. Render merged styles
    if (special.styles.size() > 0) {
        if (!is_first) output.append_view(", ")
        output.append_view("style:\"")
        for (var i : size_t = 0; i < special.styles.size(); i++) {
            if (i > 0) output.append(';')
            writeJsPrimitiveAttrValue(page, &mut *output, &*special.styles.get(i))
        }
        output.append('"')
        is_first = false
    }

    // 3. Render deferred non-special attributes (already dedup'd, last-wins)
    for (var i : size_t = 0; i < special.others_names.size(); i++) {
        if (!is_first) output.append_view(", ")
        is_first = false
        output.append_with_len(special.others_names.get(i).data, special.others_names.get(i).size)
        output.append(':')
        writeJsPrimitiveAttrValue(page, &mut *output, &*special.others_values.get(i))
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

