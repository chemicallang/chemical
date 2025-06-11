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
            printf("I got the value from result %d\n", value);
        }
        Error(error) => {
            printf("I got an error %s\n", error.message);
        }
    }
}

func check_less_optimized_branches(result : Result<int>) {
    if(result gives value) {
        printf("I got the result %d\n", value);
    } else if(result gives error) {
        printf("I got the error %s\n", error.message);
    } else {
        unreachable
    }
}

func check_less_optimized_early_return(result : Result<int>) : Result<int> {
    if(result gives error) {
        return result;
    }
    var value = result gives value else unreachable
    printf("I got the result %d\n", value);
}

func check_early_return_with_is(result : Result<int>) : Result<int> {
    if(result is Result.Failure) {
        return result;
    }
    var value = result gives value else unreachable
    printf("I got the result %d\n", value);
}

func check_return_of_result(result : Result<int>) : Result<int> {
    var value = result gives value else return result;
    printf("I got the result %d\n", value);
}

func check_default_value_using_of(result : Result<int>) {
    var result = result gives value else 434
    printf("I got the result %d\n, result);
}


```