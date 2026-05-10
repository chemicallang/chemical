NOT A PROFESSIONAL DOCUMENT, JUST A SNIPPET

### Note on Type Functions

This is not scalable, it needs to be improved

```
func <T> has_nested_type() : bool {
    type nested = using T::nested else nullptr_t
    return nested !is nullptr_t
}

func usage() {
    var works = has_nested_type<*void>() // true
    var not_works = has_nested_type<void>() // false
}
```