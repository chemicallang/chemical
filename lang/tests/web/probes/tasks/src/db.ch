// Probe 4: the application's own state — users and their todos.
//
// A web app's data is not the session. It has to live somewhere the whole process
// can reach, and `web::App` gives an app nowhere to put it (its only owned state is
// the `SessionStore`). A top-level mutable global is `SAFE-GLOBAL-001`, so the
// honest public-API answer today is an `@never_destructed` global reached through
// an `unsafe` accessor — exactly what this file does, once, so the rest of the app
// does not repeat it.
using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

@direct_init
struct User {
    var id : bigint
    var email : string
    var hash : bigint
    /// A real app stores a bcrypt hash (`lang/libs/bcrypt`); the probe keeps a
    /// non-cryptographic one so the shape is the only thing being measured.
    var salt : bigint
}

@direct_init
struct Todo {
    var id : bigint
    var owner : bigint
    var title : string
    var done : bool
}

struct Db {
    var users : vector<User>
    var todos : vector<Todo>
    var next_user : bigint
    var next_todo : bigint
    /// Counted so the shell route has something that changes every request.
    var handled : bigint
}

// The one unsafe boundary the app has to write for its own state.
@never_destructed var g_db : Db

func db() : &mut Db {
    return unsafe(&mut g_db)
}

func hash_password(salt : bigint, password : string_view) : bigint {
    var h : bigint = 1469598103934665603
    var seed = salt as ubigint
    h = h ^ (seed as bigint)
    var i : ubigint = 0
    while(i < password.size()) {
        h = h ^ (password.at(i) as bigint)
        h = h * 1099511628211
        i = i + 1
    }
    return h
}

func db_init() {
    var empty_users = vector<User>()
    var empty_todos = vector<Todo>()
    var d = Db {
        users : empty_users,
        todos : empty_todos,
        next_user : 1,
        next_todo : 1,
        handled : 0
    }
    unsafe { g_db = d }
    var email = string_view("ada@example.com")
    var password = string_view("correct horse")
    var seed = 0x5eed
    var id = db_create_user(email, hash_password(seed, password), seed)
    db_add_todo(id, string_view("Read the web stack survey"))
    var first = db_todos_of(id, true)
    var started = db_find_todo(unsafe(*first.get_ptr(0u)))
    if(started is Option.Some) {
        var Some(index) = started else unsafe { unreachable }
        var t = unsafe(&mut *g_db.todos.get_ptr(index))
        t.done = true
    }
}

func db_create_user(email : string_view, hash : bigint, salt : bigint) : bigint {
    var d = db()
    var id = d.next_user
    d.next_user = d.next_user + 1
    var stored = string()
    stored.append_view(&email)
    d.users.push_back(User { id : id, email : stored, hash : hash, salt : salt })
    return id
}

func db_find_user(email : string_view) : Option<bigint> {
    var d = db()
    var i : ubigint = 0
    while(i < d.users.size()) {
        var u = unsafe(&*d.users.get_ptr(i))
        if(u.email.to_view().equals(&email)) { return Option.Some<bigint>(u.id) }
        i = i + 1
    }
    return Option.None<bigint>()
}

func db_user_email(id : bigint) : string_view {
    var d = db()
    var i : ubigint = 0
    while(i < d.users.size()) {
        var u = unsafe(&*d.users.get_ptr(i))
        if(u.id == id) { return u.email.to_view() }
        i = i + 1
    }
    return string_view("")
}

func db_password_ok(id : bigint, password : string_view) : bool {
    var d = db()
    var i : ubigint = 0
    while(i < d.users.size()) {
        var u = unsafe(&*d.users.get_ptr(i))
        if(u.id == id) { return u.hash == hash_password(u.salt, password) }
        i = i + 1
    }
    return false
}

func db_next_todo_id() : bigint {
    var d = db()
    var id = d.next_todo
    d.next_todo = d.next_todo + 1
    return id
}

func db_add_todo(owner : bigint, title : string_view) : bigint {
    var id = db_next_todo_id()
    var d = db()
    var stored = string()
    stored.append_view(&title)
    d.todos.push_back(Todo { id : id, owner : owner, title : stored, done : false })
    return id
}

/// The index of todo `id`, or `None`. Returned as an index because the store's
/// vector is what every mutation needs a `*mut` into.
func db_find_todo(id : bigint) : Option<ubigint> {
    var d = db()
    var i : ubigint = 0
    while(i < d.todos.size()) {
        var t = unsafe(&*d.todos.get_ptr(i))
        if(t.id == id) { return Option.Some<ubigint>(i) }
        i = i + 1
    }
    return Option.None<ubigint>()
}

/// A todo, or `None` when it does not exist **or belongs to someone else** —
/// ownership is part of the lookup, so a handler cannot forget it.
func db_find_owned_todo(id : bigint, owner : bigint) : Option<ubigint> {
    var found = db_find_todo(id)
    if(found is Option.None) { return found }
    var Some(index) = found else unsafe { unreachable }
    var d = db()
    var t = unsafe(&*d.todos.get_ptr(index))
    if(t.owner != owner) { return Option.None<ubigint>() }
    return Option.Some<ubigint>(index)
}

/// Todo ids of one user, optionally filtered by completion.
func db_todos_of(owner : bigint, all_status : bool) : vector<bigint> {
    var out = vector<bigint>()
    var d = db()
    var i : ubigint = 0
    while(i < d.todos.size()) {
        var t = unsafe(&*d.todos.get_ptr(i))
        if(t.owner == owner && (all_status || !t.done)) {
            out.push_back(t.id)
        }
        i = i + 1
    }
    return out
}

func db_todo_title(id : bigint) : string_view {
    var found = db_find_todo(id)
    if(found is Option.None) { return string_view("") }
    var Some(index) = found else unsafe { unreachable }
    var d = db()
    var t = unsafe(&*d.todos.get_ptr(index))
    return t.title.to_view()
}

func db_todo_done(id : bigint) : bool {
    var found = db_find_todo(id)
    if(found is Option.None) { return false }
    var Some(index) = found else unsafe { unreachable }
    var d = db()
    var t = unsafe(&*d.todos.get_ptr(index))
    return t.done
}

func db_toggle_todo(id : bigint) {
    var found = db_find_todo(id)
    if(found is Option.None) { return }
    var Some(index) = found else unsafe { unreachable }
    var d = db()
    var t = unsafe(&mut *d.todos.get_ptr(index))
    t.done = !t.done
}

func db_delete_todo(id : bigint) {
    var found = db_find_todo(id)
    if(found is Option.None) { return }
    var Some(index) = found else unsafe { unreachable }
    var d = db()
    d.todos.remove(index)
}

func db_user_count() : bigint {
    return db().users.size() as bigint
}

func db_todo_count() : bigint {
    return db().todos.size() as bigint
}

func db_note_handled() : bigint {
    var d = db()
    d.handled = d.handled + 1
    return d.handled
}
