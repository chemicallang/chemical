using std::string;
using std::function;

// The counter demo: a `div` containing a `button` and a `span`.
// The handler closure owns a heap cell holding the count, so the state outlives
// `start` (the browser keeps the callback, not `start`'s stack frame).
@no_mangle
public func start() {
    var body = browser.document_body()

    var root = browser.create_element("div")
    browser.set_attribute(root, "class", "wasm-counter")

    var button = browser.create_element("button")
    browser.set_text(button, "Increment")

    var span = browser.create_element("span")
    browser.set_text(span, "0")

    browser.append_child(root, button)
    browser.append_child(root, span)
    browser.append_child(body, root)

    var count = unsafe(malloc(sizeof(int))) as *mut int
    unsafe { *count = 0 }

    var handler : function<() => void> = |count, span|() => {
        unsafe { *count = *count + 1 }
        var text = string()
        text.append_integer(unsafe(*count) as bigint)
        browser.set_text(span, text.to_view())
    }
    browser.on_click(button, handler)
}

public func main() : int {
    return 0
}
