public namespace std {

public variant Result<T, E> {
    Ok(value : T)
    Err(error : E)

    func is_ok(&self) : bool [no_panic] {
        return self is Result.Ok
    }

    func is_err(&self) : bool [no_panic] {
        return self is Result.Err
    }
}

}