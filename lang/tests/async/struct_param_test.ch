// Regression tests for B24: a struct-typed parameter of an `async func` that
// crosses a suspension lives in the frame *by value*, but the 2c lowering used
// to emit `frame->slot->field` (treating it as a hidden pointer) and `fn(frame->slot)`
// for a method receiver. Both were C compile errors. The field access must use
// `.`, and a method call must take the field's address.

using namespace std;

public struct B24Pair {
    var x : int
    var y : int

    func sum(&self) : int {
        return self.x + self.y
    }
}

public struct B24Inner {
    var v : int
}

public struct B24Outer {
    var inner : B24Inner
    var n : int
}

public struct B24Named {
    var s : string
    var n : int
}

async func b24_fields(p : B24Pair) : int {
    var u = await async::yield_now()
    return p.x + p.y
}

async func b24_method(p : B24Pair) : int {
    var u = await async::yield_now()
    return p.sum()
}

async func b24_nested(o : B24Outer) : int {
    var u = await async::yield_now()
    return o.inner.v + o.n
}

async func b24_destructible(nm : B24Named) : int {
    var u = await async::yield_now()
    return (nm.s.size() as int) + nm.n
}

async func b24_addr(p : B24Pair) : int {
    var u = await async::yield_now()
    var ptr = &raw p.x
    return *ptr + p.y
}

@test
func test_async_struct_param_fields(env : &mut TestEnv) {
    var p = B24Pair { x : 20, y : 22 }
    if(async::block_on<int>(b24_fields(p)) != 42) {
        env.error("struct-param field access across await should be 42")
    }
}

@test
func test_async_struct_param_method(env : &mut TestEnv) {
    var p = B24Pair { x : 20, y : 22 }
    if(async::block_on<int>(b24_method(p)) != 42) {
        env.error("method call on a struct param across await should be 42")
    }
}

@test
func test_async_struct_param_nested(env : &mut TestEnv) {
    var o = B24Outer { inner : B24Inner { v : 40 }, n : 2 }
    if(async::block_on<int>(b24_nested(o)) != 42) {
        env.error("nested struct-param field access should be 42")
    }
}

@test
func test_async_struct_param_destructible(env : &mut TestEnv) {
    var nm = B24Named { s : string("hello"), n : 37 }
    if(async::block_on<int>(b24_destructible(nm)) != 42) {
        env.error("destructible struct-param field access should be 42")
    }
}

@test
func test_async_struct_param_address_of(env : &mut TestEnv) {
    var p = B24Pair { x : 20, y : 22 }
    if(async::block_on<int>(b24_addr(p)) != 42) {
        env.error("address of a struct-param field across await should be 42")
    }
}
