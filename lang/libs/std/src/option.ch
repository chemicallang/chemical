import "./std.ch"

public namespace std {

public variant Option<T> {
    Some(value : T)
    None()
}

}