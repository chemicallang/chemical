import "basic/lambda.ch"
import "basic/expression.ch"
import "basic/floating.ch"
import "basic/nodes.ch"
import "nodes/varinit.ch"
import "nodes/struct.ch"
import "nodes/enum.ch"
import "type/datatype/numbers.ch"
import "type/datatype/strings.ch"

func main() {
    test_var_init();
    test_lambda();
    test_bodmas();
    test_floating_expr();
    test_nodes();
    test_numbers();
    test_structs();
    test_enum();
    test_strings();
}