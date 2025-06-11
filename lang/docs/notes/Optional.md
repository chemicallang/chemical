THIS IS A CONCEPT, IT MAY CONTAIN BUGS, IT MAY LITERALLY BE IMPOSSIBLE BECAUSE OF AN
IMPLEMENTATION BARRIER, TREAT IT LIKE AN UNFINISHED PROPOSAL

```
variant Optional<T> {

    Some(value : T)
    None()

}

func create_optional(condition : bool) : Optional<int> {
    if(condition) {
        return Optional.Some<int>()
    } else {
        return Optional.None()
    }
}

func check_gives_usage_in_without_else(opt : Optional<int>) {
    if(opt gives value) {
        // automatically declares 'value'
        printf("value has been given, %d\n", value);
    } else {
        printf("couldn't get value");
    }
}

func check_gives_usage_with_is(opt : Optional<int>) {
    if(opt is Optional.None) {
        printf("couldn't get value");
    }
}

func check_switch_with_optional(opt : Optional<int>) {
    switch(opt) {
        Some(value) => {
            printf("value has been given, %d\n", value);
        }
        None => {
            printf("couldn't get option");
        }
    }
}

// ---------- Lets see 'of' being used outside of if statement
// ---------- where it always requires 'else' path 

func check_default_values_in_optional(opt : Optional<int>) {
    var value = opt gives value else return;
    printf("value has been given, %d\n", value);
}

func check_return_on_no_value(opt : Optional<int>) {
    var value = opt gives value else -1;
    printf("value has been given, %d\n", value);
}

func check_unreachable_path_in_optional(opt : Optional<int>) {
    var value = opt gives value else unreachable
    printf("value has been given, %d\n", value);
}
```