NOT A PROFESSIONAL DOCUMENT, JUST A SNIPPET

### Note on Type Functions

This is not scalable, it needs to be improved, the typeof is also not supported and may not
be supported

```
func <T> has_nested_type() : bool {
    type nested = using T::nested else typeof(null)
    return nested !is typeof(null)
}

func usage() {
    var works = has_nested_type<*void>() // true
    var not_works = has_nested_type<void>() // false
}
```