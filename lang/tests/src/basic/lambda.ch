struct LambdaProvider {
    var provide : () => int
}

struct Nested {
    var provider : LambdaProvider
}

func capturing(lambda : ||() => bool) : bool {
    return lambda();
}

func delegate(lambda : ||() => bool) : bool {
    return capturing(lambda);
}

func dontCapture(lambda : () => bool) : bool {
    return lambda();
}

func passed(pass : () => int) : int {
    return pass();
}

/**
func ret_cap_lambda(message : []()=>bool) : []()=>bool {
    return message;
}
**/

func create_lamb(first : bool) : () => int {
    if(first) {
        return () => 5;
    } else {
        return () => 10;
    }
}

/**
func ret_new_cap_lamb() : []()=>bool {
    var captured = true;
    return |captured|() => {
        return captured;
    }
}
**/

struct SelfLambda {
    var i : int
    var lambda : (&self) => int;
}

struct PointSome {
    var a : int
    var b : int
}

func lamb_ret_struct() : (a : int, b : int) => PointSome {
    return (a, b) => {
        return PointSome {
            a : a,
            b : b
        }
    }
}

struct ProvideStructLamb {
    var lamb : (a : int, b : int) => PointSome
}

struct CapSelf {
    var i : int
    var lamb : ||(&self) => int
}

struct ProvideSelfRefStructLamb {
    var mul : int
    var lamb : (&self, a : int, b : int) => PointSome
}

struct LambFactory {

    func create_lamb() : () => int {
        return () => 233
    }

}

func take_lambda_with_param(lambda : (param : int) => int) : int {
    return lambda(4)
}

func test_lambda() {
    test("testing non capturing lambda works", () => {
        return true;
    })
    test("testing lambda without braces works", () => true)
    test("testing non capturing lambda works", () => {
        return dontCapture(() => {
            return true;
        });
    });
    test("lambdas created by structs work", () => {
        var factory = LambFactory{}
        return factory.create_lamb()() == 233;
    })
    test("testing non capturing lambda works without body", () => {
        return dontCapture(() => true);
    });
    test("testing non capturing lambda can be passed to capturing lambda", () => {
        return capturing(() => {
            return true;
        });
    });
    test("testing capturing lambda type works with empty capturing lambda", () => {
        return capturing(||() => {
            return true;
        });
    });
    test("testing capturing lambda can capture primitive bool value - 1", () => {
        var captured = true;
        return capturing(|captured|() => {
            return captured;
        });
    });
    test("testing capturing lambda can capture primitive bool value - 2", () => {
        var captured = false;
        return capturing(|captured|() => {
            return !captured;
        });
    });
    test("testing capturing lambda can capture by ref", () => {
        var captured = true;
        return capturing(|&captured|() => {
            return *captured;
        });
    });
    test("testing capturing lambda can be passed between functions", () => {
        var captured = true;
        return delegate(|captured|() => {
            return captured;
        });
    });
    /**
    test("testing returning capturing lambda works", () => {
        var captured = true;
        var message = ret_cap_lambda(|captured|() => {
            return captured;
        });
        return message();
    })
    **/
    test("can initialize and call a capturing lambda", () => {
        var x = true;
        var message = |x|() => {
            return x;
        };
        return message();
    })
    /**
    test("returned capturing lambda can be called directly", () => {
        var message = ret_new_cap_lamb()();
        return message;
    })
    test("packs lambda at return", () => {
        var message = ret_new_cap_lamb();
        return message();
    })
    **/
    test("can pass function pointer as lambda 1", () => {
        return passed(fn_rets_1) == 1;
    })
    test("can pass function pointer as lambda 2", () => {
        return passed(fn_rets_2) == 2;
    })
    test("can call lambda from a variable", () => {
        var x : () => int = () => {
            return 252;
        }
        return x() == 252;
    })
    test("can call lambda without knowing type", () => {
        var x = () => {
            return 253;
        }
        return x() == 253;
    })
    test("can call lambda stored in struct", () => {
        var p = LambdaProvider {
            provide : () => {
                return 252;
            }
        }
        return p.provide() == 252;
    })
    test("can call lambda stored nested in a struct", () => {
        var n = Nested {
            provider : LambdaProvider {
               provide : () => {
                   return 252;
               }
           }
        }
        return n.provider.provide() == 252;
    })
    test("can call lambda stored in an array", () => {
        type MyLamb = () => int
        var arr : MyLamb[2] = [];
        arr[0] = () => 5;
        arr[1] = () => 10;
        return arr[0]() == 5 && arr[1]() == 10;
    })
    test("a function can return lambdas", () => {
        return create_lamb(true)() == 5 && create_lamb(false)() == 10;
    })
    test("supports self lambdas", () => {
        var self_lamb = SelfLambda {
            i : 55,
            lambda : (self) => {
                return self.i * 2;
            }
        }
        return self_lamb.lambda() == 110;
    })
    test("lambda can return struct - 1", () => {
        var p = lamb_ret_struct()(10, 20);
        return p.a == 10 && p.b == 20;
    })
    test("lambda can return struct - 2", () => {
        var p = ProvideStructLamb {
            lamb : (a : int, b : int) => {
                return PointSome {
                    a : a,
                    b : b
                }
            }
        }
        var c = p.lamb(20, 30);
        return c.a == 20 && c.b == 30;
    })
    test("lambdas with self reference can return a struct", () => {
        var provide = ProvideSelfRefStructLamb {
            mul : 2,
            lamb : (self, a, b) => {
                return PointSome {
                   a : self.mul * a,
                   b : self.mul * b
                }
            }
        }
        var p = provide.lamb(10, 20);
        return p.a == 20 && p.b == 40;
    })
    test("capturing lambda can take a self reference", () => {
        var c = CapSelf {
            i : 14,
            lamb : ||(self) => {
                return self.i;
            }
        }
        return c.lamb() == 14;
    })
    test("capturing lambda taking self reference can access captured variables", () => {
        var d = 100
        var c = CapSelf {
            i : 14,
            lamb : |d|(self) => {
                return self.i + d;
            }
        }
        return c.lamb() == 114;
    })
    test("the parameters of lambda are determined from the lambda type and need not be mentioned", () => {
        var x = take_lambda_with_param(() => {
            return 33;
        })
        return x == 33;
    })
}

func fn_rets_1() : int {
    return 1;
}

func fn_rets_2() : int {
    return 2;
}