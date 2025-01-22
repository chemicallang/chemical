import "/test.ch"
import "@std/result.ch"

using namespace std;

enum ErrType {
    Simple,
    Complex
}

func get_result_int(some : bool, simple : bool) : Result<int, ErrType> {
    if(some) {
        return Result.Ok(42)
    } else {
        if(simple) {
            return Result.Err(ErrType.Simple)
        } else {
            return Result.Err(ErrType.Complex)
        }
    }
}

func get_result_value(o : Result<int, ErrType>) : int {
    switch(o) {
        Ok(value) => {
            return value;
        }
        Err(error) => {
            if(error == ErrType.Simple) {
                return -1;
            } else if(error == ErrType.Complex) {
                return -2;
            } else {
                return -3;
            }
        }
    }
}

func test_result_type() {
    test("check result type works - 1", () => {
        var o = get_result_int(true, false);
        return get_result_value(o) == 42
    })
    test("check result type works - 2", () => {
        var o = get_result_int(false, true);
        return get_result_value(o) == -1
    })
    test("check result type works - 3", () => {
        var o = get_result_int(false, false);
        return get_result_value(o) == -2
    })
}