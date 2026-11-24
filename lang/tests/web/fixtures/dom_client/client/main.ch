// The client entry point: an interaction written in Chemical with the typed `dom`
// module, compiled to JavaScript by `--web`'s client pass.
//
// It touches every part of the v1 slice: `document`, `Element` (text, attributes,
// value, children), an event listener with a Chemical callback, `fetch`, `localStorage`
// and `URL`. `scripts/web-dom-test.sh` runs the emitted bundle under Node with a DOM
// harness and compares these exact lines.
using std::Option;
using std::string;
using std::string_view;

func flag_of(found : Option<dom::Element>) : int {
    if(found is Option.Some) { return 1 }
    return 0
}

// A Chemical callback invoked by the browser. `e` is a real DOM event handle.
public func on_click(e : &dom::Event) {
    e.prevent_default()
    unsafe {
        printf("dom pointer x=%.0f y=%.0f button=%d id=%d\n",
            e.client_x(), e.client_y(), e.button(), e.pointer_id())
    }
    var doc = dom.document()
    var node = doc.get_element_by_id("count")
    if(node is Option.Some) {
        var Some(label) = node else unsafe { unreachable }
        // The page is the state: read what it shows, apply the shared rule, write it
        // back. Two dispatches therefore print two different counts.
        var current = label.text()
        var next = shared_next(shared_parse_int(current.to_view()))
        var text = shared_label(next)
        var view = text.to_view()
        label.set_text(view)
        unsafe { printf("dom click %.*s\n", view.size() as int, view.data()) }
    }
}

// A fetch completion: `body` is borrowed for the duration of this call.
public func on_fetch(status : int, body : &string_view) {
    unsafe {
        printf("dom fetch status=%d body=%.*s\n", status, body.size() as int, body.data())
    }
}

public func main() : int {
    var doc = dom.document()

    var found = doc.get_element_by_id("greeting")
    var missing = doc.get_element_by_id("nope")
    unsafe {
        printf("dom found=%d missing=%d\n", flag_of(found), flag_of(missing))
    }

    var root = doc.get_element_by_id("app")
    if(root is Option.Some) {
        var Some(app) = root else unsafe { unreachable }

        // Create, fill, attach, then read back through the DOM.
        var card = doc.create_element("div")
        card.set_attribute("data-role", "card")
        card.set_text("Hello dom")
        app.append_child(&card)

        var text = card.text()
        var role = card.attribute("data-role")
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
                card.has_attribute("data-role") as int)
        }

        // Class tokens and traversal: the element surface a client module needs to
        // move around the tree without `#js`.
        var had_none = card.has_class("card") as int
        card.add_class("card")
        card.add_class("card")
        var added = card.has_class("card") as int
        var toggled = card.toggle_class("active") as int
        var matches_self = card.matches(".active") as int
        var closest_self = flag_helper(card.closest(".active"))
        var parent_flag = flag_helper(card.parent())
        var kids = app.child_count()
        var first_kid = flag_helper(app.child_at(0))
        var past_end = flag_helper(app.child_at(kids))
        card.remove_class("card")
        var removed_class = card.has_class("card") as int
        unsafe {
            printf("dom classes none=%d added=%d toggled=%d matches=%d closest=%d parent=%d kids=%d first=%d past_end=%d removed=%d\n",
                had_none, added, toggled, matches_self, closest_self, parent_flag, kids,
                first_kid, past_end, removed_class)
        }

        var input = doc.get_element_by_id("name")
        if(input is Option.Some) {
            var Some(field) = input else unsafe { unreachable }
            field.set_value("typed")
            var value = field.value()
            var value_view = value.to_view()
            unsafe { printf("dom value=%.*s\n", value_view.size() as int, value_view.data()) }
        }

        var button = doc.get_element_by_id("count-button")
        if(button is Option.Some) {
            var Some(element) = button else unsafe { unreachable }
            element.add_event_listener("click", on_click)
        }
    }

    // localStorage: write, read, remove.
    dom::storage_set("theme", "dark")
    var theme = dom::storage_get("theme")
    var theme_text = string()
    if(theme is Option.Some) {
        var Some(value) = theme else unsafe { unreachable }
        theme_text.append_view(value.to_view())
    }
    dom::storage_remove("theme")
    var removed = dom::storage_get("theme")
    var theme_view = theme_text.to_view()
    unsafe {
        printf("dom storage=%.*s removed=%d\n", theme_view.size() as int, theme_view.data(),
            (!flag_helper(removed)) as int)
    }

    // URL parsing through the host.
    var url = string_view("https://example.com/a/b?q=7")
    var origin = dom::url_origin(url)
    var path = dom::url_pathname(url)
    var q = dom::url_search_param(url, string_view("q"))
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
    var canvas = doc.create_element("canvas")
    canvas.set_attribute("width", "320")
    canvas.set_attribute("height", "200")
    var context = canvas.get_context_2d()
    if(context is Option.Some) {
        var Some(ctx) = context else unsafe { unreachable }
        ctx.save()
        ctx.set_fill_style(string_view("rgb(0, 0, 0)"))
        ctx.fill_rect(0.0, 0.0, 320.0, 200.0)
        ctx.set_stroke_style(string_view("rgb(255, 0, 0)"))
        ctx.set_line_width(2.0)
        ctx.begin_path()
        ctx.move_to(0.0, 0.0)
        ctx.line_to(320.0, 200.0)
        ctx.stroke()
        ctx.restore()
        ctx.translate(4.0, 6.0)
        ctx.set_transform(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
        ctx.set_font(string_view("16px sans-serif"))
        ctx.fill_text(string_view("chemical"), 8.0, 24.0)
        unsafe { printf("dom canvas surface=1\n") }
    } else {
        unsafe { printf("dom canvas surface=0\n") }
    }

    var track = doc.get_element_by_id("count-button")
    if(track is Option.Some) {
        var Some(button) = track else unsafe { unreachable }
        button.set_pointer_capture(1)
    }

    dom::request_animation_frame(on_frame)

    // fire-and-forget: the callback prints when the response arrives.
    dom::fetch_text(string_view("/api/ok"), on_fetch)
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
