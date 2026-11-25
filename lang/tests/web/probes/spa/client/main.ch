// The SPA client: it owns every view after the first paint.
//
// On boot it reads the server's bootstrap payload, so the first screen is already
// rendered; clicking a view link re-renders locally, and `fetch` re-syncs from the
// JSON endpoint.
//
// What an SPA author has to do by hand today — each line is a gap recorded in
// `lang/docs/web-stack-gaps.md`:
//   * navigation is a click handler, because `dom` has no `location`/history API
//     (a real hash router needs the `#js` escape hatch);
//   * JSON is read with string splitting: no typed decode in `dom`;
//   * rows are built with `createElement` + `setHTML("")`, because `dom` has no
//     render/template primitive and no `innerHTML` on `Document`.
using std::Option;
using std::string;
using std::string_view;
using std::vector;

// The client's state. It has to be process-global because a `dom` callback carries
// no context argument, and a top-level container cannot be constructed in its
// initializer, so `main` sets it up and every access goes through one `unsafe`.
@direct_init
struct Board {
    var all : vector<Task>
    // Owns the bytes `Task.title` points at. Reserved up front so growth never
    // moves an entry the model already refers to.
    var titles : vector<string>
    var filter_open : bool
    var loaded : bool
}

@never_destructed
private var board : Board

func state() : &mut Board {
    return unsafe(&mut board)
}

func copy_of(v : string_view) : string {
    var out = string()
    out.append_view(&v)
    return out
}

func bootstrap_json() : string {
    var doc = dom.document()
    var node = doc.getElementById("bootstrap")
    if(node is Option.Some) {
        var Some(el) = node else unsafe { unreachable }
        return el.textContent()
    }
    return string()
}

// `{"tasks":"1|title|open\n2|…","open":3}` — field extraction, not a decoder.
func field_of(json : &string_view, name : string_view) : string {
    var key = string("\"")
    key.append_view(&name)
    key.append_view("\":")
    var at = json.find(&key.to_view())
    if(at is Option.None) { return string() }
    var Some(pos) = at else unsafe { unreachable }
    var start = pos + key.size()
    if(start < json.size() && json.at(start) == '"') {
        var end = start + 1
        while(end < json.size() && json.at(end) != '"') { end = end + 1 }
        return unescape_newlines(json.subview(start + 1, end))
    }
    var end = start
    while(end < json.size() && json.at(end) != ',' && json.at(end) != '}') { end = end + 1 }
    return copy_of(json.subview(start, end))
}

// The payload encodes newlines as `\n`, so the two characters become one byte.
func unescape_newlines(v : string_view) : string {
    var out = string()
    var i : size_t = 0
    while(i < v.size()) {
        var c = v.at(i)
        if(c == '\\' && i + 1 < v.size() && v.at(i + 1) == 'n') {
            out.append('\n')
            i = i + 2
        } else {
            out.append(c)
            i = i + 1
        }
    }
    return out
}

func load(tasks_json : &string_view) {
    var text = field_of(tasks_json, string_view("tasks"))
    var parsed = parse_tasks(text.to_view())
    var st = state()
    st.all.clear()
    st.titles.clear()
    var i : ubigint = 0
    while(i < parsed.size()) {
        var task = unsafe(&*(parsed.get_ptr(i)))
        st.titles.push_back(copy_of(task.title))
        var stored = unsafe(&*st.titles.get_ptr(st.titles.size() - 1u))
        var view = stored.to_view()
        st.all.push_back(Task { id : task.id, title : view, open : task.open })
        i = i + 1
    }
    st.loaded = true
    render()
}

func render() {
    var doc = dom.document()
    var list = doc.getElementById("tasks")
    if(list is Option.None) { return }
    var Some(ul) = list else unsafe { unreachable }
    // no `childCount`/`remove_child`: the list is emptied with `setHTML("")`
    ul.setHTML(string_view(""))
    var shown = 0
    var st = state()
    var index : ubigint = 0
    while(index < st.all.size()) {
        var task = unsafe(&*(st.all.get_ptr(index)))
        if(!st.filter_open || task.open) {
            var row = doc.createElement("li")
            row.setAttribute("class", "task")
            row.setText(task.title)
            if(!task.open) { row.setAttribute("data-done", "1") }
            ul.appendChild(&row)
            shown = shown + 1
        }
        index = index + 1
    }
    var status = doc.getElementById("status")
    if(status is Option.Some) {
        var Some(el) = status else unsafe { unreachable }
        var text = string()
        text.append_integer(shown as bigint)
        text.append_view(" shown")
        var view = text.to_view()
        el.setText(view)
    }
    var count = doc.getElementById("count")
    if(count is Option.Some) {
        var Some(el) = count else unsafe { unreachable }
        var text = string()
        text.append_integer(open_count(&st.all) as bigint)
        text.append_view(" open")
        var view = text.to_view()
        el.setText(view)
    }
    unsafe { printf("client render shown=%d total=%zu\n", shown, st.all.size()) }
}

// A view link: the href says which filter applies (`#/all` or `#/open`). Without a
// `location` API the click itself is the navigation event.
public func on_view_click(e : &dom::Event) {
    var target = e.target()
    if(target is Option.Some) {
        var Some(el) = target else unsafe { unreachable }
        var href = el.getAttribute("href")
        if(href is Option.Some) {
            var Some(v) = href else unsafe { unreachable }
            state().filter_open = v.to_view().contains(&string_view("#/open"))
        }
    }
    if(state().loaded) { render() }
}

// The fetch completion: `body` is borrowed for the duration of this call.
public func on_tasks(status : int, body : &string_view) {
    if(status == 200) {
        load(body)
    }
    unsafe { printf("client fetch status=%d\n", status) }
}

public func main() : int {
    // top-level containers cannot be constructed at initialisation, so the
    // process sets them up on entry
    unsafe {
        board.all = vector<Task>()
        board.titles = vector<string>()
        board.filter_open = false
        board.loaded = false
    }
    var st = state()
    st.titles.reserve(64)
    st.all.reserve(64)
    var initial = bootstrap_json()
    var initial_view = initial.to_view()
    load(&initial_view)
    unsafe { printf("client boot tasks=%zu\n", state().all.size()) }

    var doc = dom.document()
    var nav = doc.querySelector(string_view(".views"))
    if(nav is Option.Some) {
        var Some(nav_el) = nav else unsafe { unreachable }
        nav_el.addEventListener("click", on_view_click)
    }
    dom::fetch(string_view("/api/tasks"), on_tasks)
    return 0
}
