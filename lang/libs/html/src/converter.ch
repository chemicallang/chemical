/**
 * Runtime html converter.
 *
 * Walks the shared html_parser AST (HtmlRoot) and re-emits the HTML source
 * into a std::string. Only static HTML content is emitted: text, elements,
 * attributes, and comments. Chemical-value children and @if statements are
 * skipped (they cannot be evaluated without the compiler).
 */
using namespace std;

public struct HtmlRuntimeConverter {
    var str : *mut std::string
}

public func make_html_runtime_converter(str : &mut std::string) : HtmlRuntimeConverter {
    return HtmlRuntimeConverter { str : (&raw mut str) as *mut std::string }
}

func strip_quotes(value : std::string_view) : std::string_view {
    if(value.size() >= 2) {
        const first = value.data()[0]
        const last = value.data()[value.size() - 1]
        if((first == '\"' || first == '\'' || first == '`') && first == last) {
            return std::string_view(value.data() + 1, value.size() - 2)
        }
    }
    return value
}

func (converter : &mut HtmlRuntimeConverter) emit_view(view : &std::string_view) {
    converter.str.append_view(view)
}

func (converter : &mut HtmlRuntimeConverter) convert_text(text : *mut HtmlText) {
    converter.emit_view(&text.value)
}

func (converter : &mut HtmlRuntimeConverter) convert_attribute(attr : *mut HtmlAttribute) {
    converter.str.append(' ')
    converter.emit_view(&attr.name)
    if(attr.value != null) {
        converter.str.append_view("=\"")
        if(attr.value.kind == AttributeValueKind.Text) {
            const text_val = attr.value as *mut TextAttributeValue
            const stripped = strip_quotes(text_val.text)
            converter.emit_view(&stripped)
        } else if(attr.value.kind == AttributeValueKind.Number) {
            const text_val = attr.value as *mut TextAttributeValue
            converter.emit_view(&text_val.text)
        }
        converter.str.append('"')
    }
}

func (converter : &mut HtmlRuntimeConverter) convert_element(element : *mut HtmlElement) {
    converter.str.append('<')
    converter.emit_view(&element.name)
    for(var i = 0u; i < element.attributes.size(); i++) {
        converter.convert_attribute(element.attributes.get(i))
    }
    if(element.isSelfClosing) {
        converter.str.append_view("/>")
        return
    }
    converter.str.append('>')
    for(var j = 0u; j < element.children.size(); j++) {
        converter.convert_child(element.children.get(j))
    }
    converter.str.append_view("</")
    converter.emit_view(&element.name)
    converter.str.append('>')
}

func (converter : &mut HtmlRuntimeConverter) convert_comment(comment : *mut HtmlComment) {
    converter.str.append_view("<!--")
    converter.emit_view(&comment.value)
    converter.str.append_view("-->")
}

func (converter : &mut HtmlRuntimeConverter) convert_child(child : *mut HtmlChild) {
    switch(child.kind) {
        HtmlChildKind.Text => {
            converter.convert_text(child as *mut HtmlText)
        }
        HtmlChildKind.Element => {
            converter.convert_element(child as *mut HtmlElement)
        }
        HtmlChildKind.Comment => {
            converter.convert_comment(child as *mut HtmlComment)
        }
        HtmlChildKind.ChemicalNode, HtmlChildKind.ChemicalValue, HtmlChildKind.IfStatement => {
            // cannot be evaluated without the compiler; skip
        }
    }
}

public func convert_html_root(root : *mut HtmlRoot, str : &mut std::string) {
    var converter = make_html_runtime_converter(str)
    for(var i = 0u; i < root.children.size(); i++) {
        converter.convert_child(root.children.get(i))
    }
}
