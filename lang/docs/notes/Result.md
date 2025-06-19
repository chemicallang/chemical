THIS IS A CONCEPT, IT MAY CONTAIN BUGS, IT MAY LITERALLY BE IMPOSSIBLE BECAUSE OF AN
IMPLEMENTATION BARRIER, TREAT IT LIKE AN UNFINISHED PROPOSAL

```
struct Error {
    var message : std::string
}

variant Result<T, E = Error> {
    
    Success(value : T)
    Failure(error : E)
    
}

func create_result(condition : bool) : Result<int> {
    if(condition) {
        return Result.Success<int>(10)
    } else {
        return Result.Failure<int>({ message : "couldn't get result" })
    }
}

func switch_on_result(result : Result<int>) {
    switch(result) {
        Success(value) => {
            printf("I got the value from result %d\n", value)
        }
        Failure(error) => {
            printf("I got an error %s\n", error.message)
        }
    }
}

func check_less_optimized_branches(result: Result<int>) {
    if var Success(value) = result {
        printf("I got the result %d\n", value)
    }
    else if var Failure(error) = result {
        printf("I got the error %s\n", error.message)
    }
    else {
        unreachable
    }
}

func check_less_optimized_early_return(result: Result<int>) : Result<int> {
    // early-return on error
    if var Failure(_) = result {
        return result
    }
    // extract success or abort
    var Success(value) = result else unreachable
    printf("I got the result %d\n", value)
    return result
}

func check_early_return_with_is(result: Result<int>) : Result<int> {
    // you can still test the tag directly if you like
    if result is Result.Failure {
        return result
    }
    var Success(value) = result else unreachable
    printf("I got the result %d\n", value)
    return result
}

func check_return_of_result(result: Result<int>) : Result<int> {
    // bind or propagate the whole result
    var Success(value) = result else return result
    printf("I got the result %d\n", value)
    return result
}

func check_default_value_using_of(result: Result<int>) {
    // fallback default of 434 for errors
    var Success(value) = result else 434
    printf("I got the result %d\n", value)
}


```