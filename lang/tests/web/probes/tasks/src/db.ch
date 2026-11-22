// Probe 4: the application's own state — users and their todos.
//
// A web app's data is not the session. It has to live somewhere the whole process
// can reach. `web::App` now carries it: `app.set_state(&raw mut db)` hands the app
// one pointer, and every route recovers the typed value with `ctx.state<Db>()` —
// the same context that carries the session.
//
// This used to be the probe's one workaround: a mutable global is `SAFE-GLOBAL-001`,
// so the app kept an `@never_destructed` global reached through an `unsafe`
// accessor. Now the value is a stack local in `main` and the only `unsafe` left is
// `state_of`, which dereferences the pointer the caller registered.
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

/// The app state, as the type handed to `app.set_state`. Null only when a route runs
/// in an app that never set one; the dereference is the caller's assertion, which is
/// why it is the single `unsafe` in this file.
func state_of(ctx : &web::RequestContext) : &mut Db {
    return unsafe(&mut *(ctx.state<Db>()))
}

func state_init() : Db {
    return Db {
        users : vector<User>(),
        todos : vector<Todo>(),
        next_user : 1,
        next_todo : 1,
        handled : 0
    }
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

func db_init(db : &mut Db) {
    var email = string_view("ada@example.com")
    var password = string_view("correct horse")
    var seed = 0x5eed
    var id = db_create_user(db, email, hash_password(seed, password), seed)
    db_add_todo(db, id, string_view("Read the web stack survey"))
    var first = db_todos_of(db, id, true)
    var started = db_find_todo(db, unsafe(*first.get_ptr(0u)))
    if(started is Option.Some) {
        var Some(index) = started else unsafe { unreachable }
        var t = unsafe(&mut *db.todos.get_ptr(index))
        t.done = true
    }
}

func db_create_user(db : &mut Db, email : string_view, hash : bigint, salt : bigint) : bigint {
    var id = db.next_user
    db.next_user = db.next_user + 1
    var stored = string()
    stored.append_view(&email)
    db.users.push_back(User { id : id, email : stored, hash : hash, salt : salt })
    return id
}

func db_find_user(db : &mut Db, email : string_view) : Option<bigint> {
    var i : ubigint = 0
    while(i < db.users.size()) {
        var u = unsafe(&*db.users.get_ptr(i))
        if(u.email.to_view().equals(&email)) { return Option.Some<bigint>(u.id) }
        i = i + 1
    }
    return Option.None<bigint>()
}

func db_user_email(db : &mut Db, id : bigint) : string_view {
    var i : ubigint = 0
    while(i < db.users.size()) {
        var u = unsafe(&*db.users.get_ptr(i))
        if(u.id == id) { return u.email.to_view() }
        i = i + 1
    }
    return string_view("")
}

func db_password_ok(db : &mut Db, id : bigint, password : string_view) : bool {
    var i : ubigint = 0
    while(i < db.users.size()) {
        var u = unsafe(&*db.users.get_ptr(i))
        if(u.id == id) { return u.hash == hash_password(u.salt, password) }
        i = i + 1
    }
    return false
}

func db_next_todo_id(db : &mut Db) : bigint {
    var id = db.next_todo
    db.next_todo = db.next_todo + 1
    return id
}

func db_add_todo(db : &mut Db, owner : bigint, title : string_view) : bigint {
    var id = db_next_todo_id(db)
    var stored = string()
    stored.append_view(&title)
    db.todos.push_back(Todo { id : id, owner : owner, title : stored, done : false })
    return id
}

/// The index of todo `id`, or `None`. Returned as an index because the store's
/// vector is what every mutation needs a `*mut` into.
func db_find_todo(db : &mut Db, id : bigint) : Option<ubigint> {
    var i : ubigint = 0
    while(i < db.todos.size()) {
        var t = unsafe(&*db.todos.get_ptr(i))
        if(t.id == id) { return Option.Some<ubigint>(i) }
        i = i + 1
    }
    return Option.None<ubigint>()
}

/// A todo, or `None` when it does not exist **or belongs to someone else** —
/// ownership is part of the lookup, so a handler cannot forget it.
func db_find_owned_todo(db : &mut Db, id : bigint, owner : bigint) : Option<ubigint> {
    var found = db_find_todo(db, id)
    if(found is Option.None) { return found }
    var Some(index) = found else unsafe { unreachable }
    var t = unsafe(&*db.todos.get_ptr(index))
    if(t.owner != owner) { return Option.None<ubigint>() }
    return Option.Some<ubigint>(index)
}

/// Todo ids of one user, optionally filtered by completion.
func db_todos_of(db : &mut Db, owner : bigint, all_status : bool) : vector<bigint> {
    var out = vector<bigint>()
    var i : ubigint = 0
    while(i < db.todos.size()) {
        var t = unsafe(&*db.todos.get_ptr(i))
        if(t.owner == owner && (all_status || !t.done)) {
            out.push_back(t.id)
        }
        i = i + 1
    }
    return out
}

func db_todo_title(db : &mut Db, id : bigint) : string_view {
    var found = db_find_todo(db, id)
    if(found is Option.None) { return string_view("") }
    var Some(index) = found else unsafe { unreachable }
    var t = unsafe(&*db.todos.get_ptr(index))
    return t.title.to_view()
}

func db_todo_done(db : &mut Db, id : bigint) : bool {
    var found = db_find_todo(db, id)
    if(found is Option.None) { return false }
    var Some(index) = found else unsafe { unreachable }
    var t = unsafe(&*db.todos.get_ptr(index))
    return t.done
}

func db_toggle_todo(db : &mut Db, id : bigint) {
    var found = db_find_todo(db, id)
    if(found is Option.None) { return }
    var Some(index) = found else unsafe { unreachable }
    var t = unsafe(&mut *db.todos.get_ptr(index))
    t.done = !t.done
}

func db_delete_todo(db : &mut Db, id : bigint) {
    var found = db_find_todo(db, id)
    if(found is Option.None) { return }
    var Some(index) = found else unsafe { unreachable }
    db.todos.remove(index)
}

func db_user_count(db : &mut Db) : bigint {
    return db.users.size() as bigint
}

func db_todo_count(db : &mut Db) : bigint {
    return db.todos.size() as bigint
}

func db_note_handled(db : &mut Db) : bigint {
    db.handled = db.handled + 1
    return db.handled
}
