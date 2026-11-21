using std::Option;
using std::string;
using std::string_view;
using std::vector;

// The shell: one page, no server-side view logic. Everything below `#app` is
// produced in the browser, so the shell is a constant string plus the bundle.
public func Shell(page : &mut HtmlPage, req : &http::Request) {
    page.appendTitle("Task board")
    page.append_head("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">")
    var initial = initial_payload()
    var initial_view = initial.to_view()
    #html {
        <main class="board">
            <header class="board-head">
                <h1>Task board</h1>
                <span id="count" class="count">0 open</span>
                <nav class="views">
                    <a href="#/all" class="view-link">All</a>
                    <a href="#/open" class="view-link">Open</a>
                </nav>
            </header>
            <ul id="tasks" class="tasks"></ul>
            <p id="status" class="status">loading…</p>
        </main>
        <script type="application/json" id="bootstrap">{initial_view}</script>
    }
}

// The payload the client reads on boot, so the first screen does not wait for a
// round trip. An SPA that wants to be fast has to do this by hand today.
func initial_payload() : string {
    var tasks = seeded_tasks()
    var encoded = encode_tasks(&tasks)
    var out = string("{\"tasks\":\"")
    out.append_view(encoded.to_view())
    out.append_view("\",\"open\":")
    out.append_integer(open_count(&tasks) as bigint)
    out.append('}')
    return out
}

// The database, for a probe: five tasks in memory.
func seeded_tasks() : vector<Task> {
    var out = vector<Task>()
    out.push_back(Task { id : 1, title : "write the shell", open : true })
    out.push_back(Task { id : 2, title : "fetch the payload", open : true })
    out.push_back(Task { id : 3, title : "render rows", open : false })
    out.push_back(Task { id : 4, title : "wire the hash router", open : true })
    out.push_back(Task { id : 5, title : "ship it", open : false })
    return out
}

// `GET /api/tasks` — the only dynamic endpoint the SPA needs.
public func tasks_json() : string {
    var tasks = seeded_tasks()
    var encoded = encode_tasks(&tasks)
    var out = string("{\"tasks\":\"")
    out.append_view(encoded.to_view())
    out.append_view("\",\"open\":")
    out.append_integer(open_count(&tasks) as bigint)
    out.append('}')
    return out
}
