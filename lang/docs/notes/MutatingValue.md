### When a value changes

a value changes because it can be mutated by implicit stuff, for example

var p : dyn Phone = SmartPhone { }

here apparently The SmartPhone object is being created, but implicitly assigned to dyn Phone object, which is a fat pointer
so in reality Phone { SmartPhone*, Impl* } is being created

### Taking into account proper mutation of values

When a value is to be mutated based on type, where does the implicit mutation occurs

1 - Var Init `var x : Type = Value`
2 - Assignment  `x = Value`
3 - Function Return `func sum() : Type { return Value }`
4 - Function Call Argument `sum(Value)`
5 - Struct Member `var y = Container { member : Value }`
6 - Array Value `var z : Type[] = { Value }`
7 - Variant Call `Optional.Some(value)`