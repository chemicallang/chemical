import "basic/lambda.ch"
import "basic/expression.ch"
import "basic/floating.ch"
import "basic/nodes.ch"
import "nodes/varinit.ch"
import "nodes/struct.ch"
import "nodes/enum.ch"
import "type/datatype/numbers.ch"
import "type/datatype/strings.ch"
import "basic/macros.ch"
import "basic/arrays.ch"
import "basic/pointers.ch"
import "basic/casts.ch"
import "basic/functions/functions.ch"
import "basic/functions/implicit.ch"
import "basic/destructors.ch"
import "nodes/union.ch"
import "nodes/namespaces.ch"
import "comptime/basic.ch"
import "comptime/vector.ch"
import "basic/external.ch"
import "generic/basic.ch"
import "generic/deduction.ch"
import "type/datatype/vectors.ch"
import "type/datatype/array_refs.ch"
import "type/datatype/optional.ch"
import "type/datatype/result.ch"
import "basic/dynamic.ch"
import "basic/variants.ch"
import "basic/modules1.ch"
import "basic/modules.ch"
import "basic/moves.ch"
import "comptime/is_value.ch"
import "cbi/html/basic.ch"

public func main() : int {
    test_var_init();
    test_lambda();
    test_bodmas();
    test_floating_expr();
    test_nodes();
    test_numbers();
    test_structs();
    test_enum();
    test_strings();
    test_macros();
    test_arrays();
    test_pointer_math();
    test_casts();
    test_functions();
    test_implicit_functions();
    test_destructors();
    test_unions();
    test_namespaces();
    test_comptime();
    test_compiler_vector();
    test_external_functions();
    test_basic_generics();
    test_generic_type_deduction();
    test_vectors();
    test_array_refs();
    test_dynamic_dispatch();
    test_variants();
    test_optional_type();
    test_result_type();
    test_is_value();
    test_imported_modules();
    test_modules_import();
    test_moves();
    test_html();
    print_test_stats();
    return 0;
}