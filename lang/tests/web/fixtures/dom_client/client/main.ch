// The client entry point: an interaction written in Chemical with the typed `dom`
// module, compiled to JavaScript by `--web`'s client pass.
//
// It touches every part of the v1 slice: `document` roots (title, documentElement,
// activeElement), `Element` (text, attributes, value, children), document/window
// listeners, timers, clipboard, `fetch`, `localStorage` and `URL`.
// `scripts/web-dom-test.sh` runs the emitted bundle under Node with a DOM harness
// and compares these exact lines.
using std::Option;
using std::string;
using std::string_view;

func flag_of(found : Option<dom::Element>) : int {
    if(found is Option.Some) { return 1 }
    return 0
}

// A Chemical callback invoked by the browser. `e` is a real DOM event handle.
public func on_click(e : &dom::Event) {
    e.preventDefault()
    unsafe {
        printf("dom pointer x=%.0f y=%.0f button=%d id=%d\n",
            e.clientX(), e.clientY(), e.button(), e.pointerId())
    }
    // The listener's attachment point, not the event target.
    var origin = e.currentTarget()
    var origin_id = string()
    if(origin is Option.Some) {
        var Some(origin_el) = origin else unsafe { unreachable }
        var origin_attr = origin_el.getAttribute("id")
        if(origin_attr is Option.Some) {
            var Some(origin_value) = origin_attr else unsafe { unreachable }
            origin_id.append_view(origin_value.to_view())
        }
    }
    var origin_view = origin_id.to_view()
    unsafe { printf("dom current=%.*s\n", origin_view.size() as int, origin_view.data()) }
    var doc = dom.document()
    var node = doc.getElementById("count")
    if(node is Option.Some) {
        var Some(label) = node else unsafe { unreachable }
        // The page is the state: read what it shows, apply the shared rule, write it
        // back. Two dispatches therefore print two different counts.
        var current = label.textContent()
        var next = shared_next(shared_parse_int(current.to_view()))
        var text = shared_label(next)
        var view = text.to_view()
        label.setText(view)
        unsafe { printf("dom click %.*s\n", view.size() as int, view.data()) }
    }
}

// A fetch completion: `body` is borrowed for the duration of this call.
public func on_fetch(status : int, body : &string_view) {
    unsafe {
        printf("dom fetch status=%d body=%.*s\n", status, body.size() as int, body.data())
    }
}

// Document/window listeners remove themselves after the first dispatch, so the
// harness firing each event twice proves `removeEventListener` reached the host.
public func on_document_event(e : &dom::Event) {
    e.preventDefault()
    dom.document().removeEventListener("domtest", on_document_event)
    unsafe { printf("dom event doc=1\n") }
}

public func on_window_event(e : &dom::Event) {
    e.stopPropagation()
    dom.window().removeEventListener("domwin", on_window_event)
    unsafe { printf("dom event win=1\n") }
}

// The live timer callback; the cleared timer must never reach it.
public func on_timeout() {
    unsafe { printf("dom timeout=1\n") }
}

public func main() : int {
    var doc = dom.document()

    var found = doc.getElementById("greeting")
    var missing = doc.getElementById("nope")
    unsafe {
        printf("dom found=%d missing=%d\n", flag_of(found), flag_of(missing))
    }

    var root = doc.getElementById("app")
    if(root is Option.Some) {
        var Some(app) = root else unsafe { unreachable }

        // Create, fill, attach, then read back through the DOM.
        var card = doc.createElement("div")
        card.setAttribute("data-role", "card")
        card.setText("Hello dom")
        app.appendChild(&card)

        var text = card.textContent()
        var role = card.getAttribute("data-role")
        var role_text = string()
        if(role is Option.Some) {
            var Some(value) = role else unsafe { unreachable }
            role_text.append_view(value.to_view())
        }
        var text_view = text.to_view()
        var role_view = role_text.to_view()
        unsafe {
            printf("dom text=%.*s role=%.*s has=%d\n",
                text_view.size() as int, text_view.data(),
                role_view.size() as int, role_view.data(),
                card.hasAttribute("data-role") as int)
        }

        // Class tokens and traversal: the element surface a client module needs to
        // move around the tree without `#js`.
        var had_none = card.hasClass("card") as int
        card.addClass("card")
        card.addClass("card")
        var added = card.hasClass("card") as int
        var toggled = card.toggleClass("active") as int
        var matches_self = card.matches(".active") as int
        var closest_self = flag_helper(card.closest(".active"))
        var parent_flag = flag_helper(card.parent())
        var kids = app.childCount()
        var first_kid = flag_helper(app.childAt(0))
        var past_end = flag_helper(app.childAt(kids))
        card.removeClass("card")
        var removed_class = card.hasClass("card") as int
        unsafe {
            printf("dom classes none=%d added=%d toggled=%d matches=%d closest=%d parent=%d kids=%d first=%d past_end=%d removed=%d\n",
                had_none, added, toggled, matches_self, closest_self, parent_flag, kids,
                first_kid, past_end, removed_class)
        }

        // Deep DOM surface: querySelectorAll copies the host NodeList into a
        // vector, so indexing and iteration are ordinary Chemical operations.
        var all_roles = app.querySelectorAll("[data-role]")
        var role_count : int = 0
        var role_labels = string()
        if(all_roles is Option.Some) {
            var Some(found_roles) = all_roles else unsafe { unreachable }
            role_count = found_roles.size() as int
            var role_i : size_t = 0
            while(role_i < found_roles.size()) {
                var role_el = found_roles.get_ptr(role_i)
                var role_content = unsafe(role_el.textContent())
                role_labels.append_view(role_content.to_view())
                if(role_i + 1 < found_roles.size()) {
                    role_labels.append_view(string_view("|"))
                }
                role_i = role_i + 1
            }
        }
        var document_roles = doc.querySelectorAll("[data-role]")
        var document_role_count : int = 0
        if(document_roles is Option.Some) {
            var Some(found_document_roles) = document_roles else unsafe { unreachable }
            document_role_count = found_document_roles.size() as int
        }
        var roles_view = role_labels.to_view()
        unsafe {
            printf("dom qsa doc=%d element=%d texts=%.*s\n",
                document_role_count, role_count,
                roles_view.size() as int, roles_view.data())
        }

        // classList / style: the browser spelling over the same host operations.
        var tokens = card.classList()
        var token_before = tokens.contains("deep") as int
        tokens.add("deep")
        tokens.add("deep")
        var token_added = tokens.contains("deep") as int
        var token_off = tokens.toggle("deep") as int
        var token_after_off = tokens.contains("deep") as int
        var token_on = tokens.toggle("deep") as int
        var token_after_on = tokens.contains("deep") as int
        tokens.remove("deep")
        var token_removed = tokens.contains("deep") as int
        unsafe {
            printf("dom classlist before=%d added=%d off=%d after_off=%d on=%d after_on=%d removed=%d\n",
                token_before, token_added, token_off, token_after_off,
                token_on, token_after_on, token_removed)
        }

        var inline = card.style()
        inline.setProperty("color", "rgb(1, 2, 3)")
        inline.setProperty("display", "block")
        var color_value = inline.getPropertyValue("color")
        inline.removeProperty("display")
        var display_value = inline.getPropertyValue("display")
        var color_view = color_value.to_view()
        var display_empty = (display_value.size() == 0) as int
        unsafe {
            printf("dom style color=%.*s display_empty=%d\n",
                color_view.size() as int, color_view.data(), display_empty)
        }

        var rect = card.getBoundingClientRect()
        unsafe {
            printf("dom rect x=%.1f y=%.1f w=%.1f h=%.1f top=%.1f right=%.1f bottom=%.1f left=%.1f\n",
                rect.x, rect.y, rect.width, rect.height,
                rect.top, rect.right, rect.bottom, rect.left)
        }

        var input = doc.getElementById("name")
        if(input is Option.Some) {
            var Some(field) = input else unsafe { unreachable }
            field.focus()
            field.setValue("typed")
            var value = field.value()
            var value_view = value.to_view()
            unsafe { printf("dom value=%.*s\n", value_view.size() as int, value_view.data()) }
        }

        // Root accessors: title, documentElement and activeElement all come from
        // the host, and the focused input is what activeElement names.
        var active = doc.activeElement()
        var active_id = string()
        if(active is Option.Some) {
            var Some(active_el) = active else unsafe { unreachable }
            var id_attr = active_el.getAttribute("id")
            if(id_attr is Option.Some) {
                var Some(id_value) = id_attr else unsafe { unreachable }
                active_id.append_view(id_value.to_view())
            }
        }
        var title = doc.title()
        var root_el = doc.documentElement()
        var root_attr = root_el.getAttribute("data-root")
        var root_text = string()
        if(root_attr is Option.Some) {
            var Some(root_value) = root_attr else unsafe { unreachable }
            root_text.append_view(root_value.to_view())
        }
        var title_view = title.to_view()
        var active_view = active_id.to_view()
        var root_view = root_text.to_view()
        unsafe {
            printf("dom document title=%.*s root=%.*s active=%.*s\n",
                title_view.size() as int, title_view.data(),
                root_view.size() as int, root_view.data(),
                active_view.size() as int, active_view.data())
        }

        // Viewport metrics come from the host window.
        var viewport = dom.window()
        unsafe {
            printf("dom window w=%.0f h=%.0f x=%.0f y=%.0f\n",
                viewport.innerWidth(), viewport.innerHeight(),
                viewport.scrollX(), viewport.scrollY())
        }

        var button = doc.getElementById("count-button")
        if(button is Option.Some) {
            var Some(element) = button else unsafe { unreachable }
            element.addEventListener("click", on_click)
        }
    }

    // localStorage: write, read, remove.
    var storage = dom.localStorage()
    storage.setItem("theme", "dark")
    var theme = storage.getItem("theme")
    var theme_text = string()
    if(theme is Option.Some) {
        var Some(value) = theme else unsafe { unreachable }
        theme_text.append_view(value.to_view())
    }
    storage.removeItem("theme")
    var removed = storage.getItem("theme")
    var theme_view = theme_text.to_view()
    unsafe {
        printf("dom storage=%.*s removed=%d\n", theme_view.size() as int, theme_view.data(),
            (!flag_helper(removed)) as int)
    }

    // URL parsing through the host.
    var url = string_view("https://example.com/a/b?q=7")
    var origin = dom::urlOrigin(url)
    var path = dom::urlPathname(url)
    var q = dom::urlSearchParam(url, string_view("q"))
    var q_text = string()
    if(q is Option.Some) {
        var Some(value) = q else unsafe { unreachable }
        q_text.append_view(value.to_view())
    }
    var origin_view = origin.to_view()
    var path_view = path.to_view()
    var q_view = q_text.to_view()
    unsafe {
        printf("dom url origin=%.*s path=%.*s q=%.*s\n",
            origin_view.size() as int, origin_view.data(),
            path_view.size() as int, path_view.data(),
            q_view.size() as int, q_view.data())
    }

    // Canvas + pointer + animation frame: the drawing surface an app writes
    // instead of `#js`.
    var canvas = doc.createElement("canvas")
    canvas.setAttribute("width", "320")
    canvas.setAttribute("height", "200")
    var context = canvas.getContext2D()
    if(context is Option.Some) {
        var Some(ctx) = context else unsafe { unreachable }
        ctx.save()
        ctx.setFillStyle(string_view("rgb(0, 0, 0)"))
        ctx.fillRect(0.0, 0.0, 320.0, 200.0)
        ctx.setStrokeStyle(string_view("rgb(255, 0, 0)"))
        ctx.setLineWidth(2.0)
        ctx.beginPath()
        ctx.moveTo(0.0, 0.0)
        ctx.lineTo(320.0, 200.0)
        ctx.stroke()
        ctx.restore()
        ctx.translate(4.0, 6.0)
        ctx.setTransform(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
        ctx.setFont(string_view("16px sans-serif"))
        ctx.fillText(string_view("chemical"), 8.0, 24.0)
        unsafe { printf("dom canvas surface=1\n") }
    } else {
        unsafe { printf("dom canvas surface=0\n") }
    }

    var track = doc.getElementById("count-button")
    if(track is Option.Some) {
        var Some(button) = track else unsafe { unreachable }
        button.setPointerCapture(1)
    }

    // Document and Window listeners: the harness fires each event twice and the
    // handler removes itself, so one line proves add + remove.
    doc.addEventListener("domtest", on_document_event)
    var win = dom.window()
    win.addEventListener("domwin", on_window_event)

    // Timers: the cleared one never fires; the second one does.
    var timer = win.setTimeout(on_timeout, 1000)
    win.clearTimeout(timer)
    win.setTimeout(on_timeout, 0)
    win.requestAnimationFrame(on_frame)

    // Clipboard: the harness records the text and prints it after the run.
    var clipboard = dom.navigator().clipboard()
    clipboard.writeText(string_view("chemical clip"))

    // fire-and-forget: the callback prints when the response arrives.
    dom::fetch(string_view("/api/ok"), on_fetch)
    return 0
}

// The frame callback: a browser hands it the frame time in milliseconds.
public func on_frame(time_ms : f64) {
    unsafe { printf("dom raf t=%.0f\n", time_ms) }
}

func <T> flag_helper(found : Option<T>) : int {
    if(found is Option.Some) { return 1 }
    return 0
}
