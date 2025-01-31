import "basic/lambda.ch"
import "basic/values/expression.ch"
import "basic/values/floating.ch"
import "basic/values/inc_dec.ch"
import "basic/nodes.ch"
import "nodes/varinit.ch"
import "nodes/struct.ch"
import "nodes/typealias.ch"
import "nodes/enum.ch"
import "type/datatype/numbers.ch"
import "type/datatype/strings.ch"
import "basic/macros/macros.ch"
import "basic/values/arrays.ch"
import "basic/values/pointers.ch"
import "basic/values/casts.ch"
import "basic/functions/functions.ch"
import "basic/functions/extension_func.ch"
import "basic/functions/implicit.ch"
import "basic/references/basic.ch"
import "basic/references/auto_deref.ch"
import "basic/destructors.ch"
import "nodes/union.ch"
import "nodes/namespaces.ch"
import "comptime/basic.ch"
import "comptime/expressions.ch"
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
import "comptime/satisfies.ch"
import "cbi/html/basic.ch"
import "basic/values/new.ch"

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
    test_comptime_expressions();
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
    test_references();
    test_auto_deref();
    test_satisfies();
    test_new();
    test_extension_functions();
    test_typealias();
    test_inc_dec();
    print_test_stats();
    return 0;
}