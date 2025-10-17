public namespace std {

public variant Option<T> {
    Some(value : T)
    None()

    func take(&self) : T {
        if(self is Option.None) {
            panic("cannot take on a option that's none")
        }
        var Some(value) = self else unreachable
        // moved out
        var temp = value
        // assign without destruction None<T>
        new(&self) Option.None<T>()
        return temp;
    }

}

}