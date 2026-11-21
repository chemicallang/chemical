using std::Option;
using std::string;
using std::string_view;
using std::vector;

// One task, in the wire form the JSON endpoint serves:
//   `id|title|open`  per line, tasks separated by newlines.
//
// A real app would use the `json` library; a hand-rolled line format keeps this
// probe about the *shape* of an SPA, and the absence of a typed client-side JSON
// decode is one of the gaps recorded in `lang/docs/web-stack-gaps.md`.
@direct_init
public struct Task {
    public var id : int
    public var title : string_view
    public var open : bool
}

// Shared by both jobs: the server uses it for the first paint, the client for
// every navigation afterwards.
public func parse_tasks(text : string_view) : vector<Task> {
    var out = vector<Task>()
    var lines = text.split('\n')
    var i : ubigint = 0
    while(i < lines.size()) {
        var line = unsafe(&*(lines.get_ptr(i)))
        var view = line.to_view()
        if(!view.empty()) {
            var fields = view.split('|')
            if(fields.size() >= 3) {
                var id_text = unsafe(&*fields.get_ptr(0))
                var title = unsafe(&*fields.get_ptr(1))
                var state = unsafe(&*fields.get_ptr(2))
                var id = 0
                var parsed = id_text.to_view().to_int()
                if(parsed is std::Result.Ok) {
                    var Ok(n) = parsed else unsafe { unreachable }
                    id = n
                }
                out.push_back(Task {
                    id : id,
                    title : title.to_view(),
                    open : state.to_view().equals(&string_view("open"))
                })
            }
        }
        i = i + 1
    }
    return out
}

// The wire form, produced by the server (and by the client's optimistic toggle).
public func encode_tasks(tasks : &vector<Task>) : string {
    var out = string()
    var i : ubigint = 0
    while(i < tasks.size()) {
        var task = unsafe(&*(tasks.get_ptr(i)))
        if(i > 0) { out.append('\n') }
        out.append_integer(task.id as bigint)
        out.append('|')
        out.append_view(&task.title)
        out.append('|')
        if(task.open) { out.append_view("open") } else { out.append_view("done") }
        i = i + 1
    }
    return out
}

// How many tasks are still open — the header count both jobs show.
public func open_count(tasks : &vector<Task>) : int {
    var n = 0
    var i : ubigint = 0
    while(i < tasks.size()) {
        var task = unsafe(&*(tasks.get_ptr(i)))
        if(task.open) { n = n + 1 }
        i = i + 1
    }
    return n
}
