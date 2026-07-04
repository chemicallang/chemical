// Copyright (c) Chemical Language Foundation 2026.

// -------------------------------------------------------
// Tests for intrinsics::is_runtime() and intrinsics::is_comptime()
// These are comptime-evaluated boolean intrinsics.
// -------------------------------------------------------

comptime func test_is_runtime_comptime() : bool {
    // Inside a comptime function, is_comptime returns true, is_runtime returns false
    if(!intrinsics::is_comptime()) {
        return false;
    }
    if(intrinsics::is_runtime()) {
        return false;
    }
    return true;
}

comptime func test_is_runtime_comptime_nested() : bool {
    // Call another comptime function to verify nested comptime context
    if(!test_is_runtime_comptime()) {
        return false;
    }
    return true;
}

func test_comptime_intrinsics() {
    test("is_comptime returns true in comptime function", () => {
        return test_is_runtime_comptime();
    })

    test("is_comptime returns true in nested comptime call", () => {
        return test_is_runtime_comptime_nested();
    })

    test("is_runtime returns false in comptime function", () => {
        return test_is_runtime_comptime();
    })

    test("is_comptime and is_runtime are opposites in comptime if", () => {
        comptime if(intrinsics::is_runtime()) {
            // In compiled mode, is_runtime is true, is_comptime is false
            return !intrinsics::is_comptime();
        } else {
            // In interpretation mode, is_runtime is false, is_comptime is true
            return intrinsics::is_comptime();
        }
    })

    test("comptime if with is_runtime works correctly", () => {
        comptime if(intrinsics::is_interpretation()) {
            comptime if(intrinsics::is_runtime()) {
                return false;
            }
            comptime if(!intrinsics::is_comptime()) {
                return false;
            }
        } else {
            comptime if(!intrinsics::is_runtime()) {
                return false;
            }
            comptime if(intrinsics::is_comptime()) {
                return false;
            }
        }
        return true;
    })
}
