struct test_struct {
    var x : int
}

func ret_ac() {
    var t = test_struct {
        x : 5
    }
    return t.x;
}

func ret_ac_passed(ac : int) {
    return ac;
}

func test_access_chain() {

   var t = test_struct {
       x : 5
   }

   print("access-chain:", t.x, '\n');
   print("access-chain-return:", ret_ac(), '\n');
   print("access-chain-passed:", ret_ac_passed(t.x), '\n');

}

func ret_id() {
    var x = 5;
    return x;
}

func ret_id_passed(id : int) {
    return id;
}

func test_id() {
    // test initializing a identifier
    var x = 5;
    print("init-id-int:", x, '\n');

    // test assigning an int
    var y : int
    y = 5;
    print("assign-id-int:", y, '\n');

    // test returning an id
    print("ret_id:", ret_id(), '\n');

    // test passing an id
    var z = 5;
    print("ret_id_passed:", ret_id_passed(z), '\n');

}


func ret_prim() {
    return 5;
}
func ret_prim_passed(prim : int) {
    return prim;
}
func test_primitive() {

    // init
    print("init-int:", 5, '\n');

    // primitive
    print("ret_prim:", ret_prim(), '\n');

    // primitive passed to function
    print("ret_prim_passed:", ret_prim_passed(5), '\n');

}

func main() {
    test_primitive();
    test_id();
    test_access_chain();
    return 0;
}

var __main__ = main();