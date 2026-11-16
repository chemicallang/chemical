// JS backend — a real library module (Phase 1 exit criterion).
//
// `json` is the strongest available test of the backend: parsing, a variant with
// aggregate payloads (`JsonValue`), string keys in
// `std::ordered_map<string, JsonValue>`, vectors, `Result`, and re-encoding.
// Identical stdout under the native and JS backends means the variant payloads,
// the map, and the hash function all agree too.

using std::Result;

func field(fields : &std::ordered_map<std::string, JsonValue>, key : std::string_view) : *JsonValue {
    return fields.get_ptr(std::string(key))
}

public func main() : int {
    unsafe {
        const text = std::string_view("{\"name\":\"chemical\",\"n\":42,\"list\":[1,2,3],\"ok\":true,\"nested\":{\"x\":-7}}")

        var parsed = json::parse(text)
        if(parsed is Result.Err) {
            printf("json_error\n")
            return 1
        }
        var Ok(value) = parsed else unreachable

        if(value is JsonValue.Object) {
            var Object(fields) = value else unreachable

            var name_ptr = field(&fields, "name")
            if(name_ptr != null && name_ptr is JsonValue.String) {
                var String(s) = *name_ptr else unreachable
                printf("name=%s\n", s.data())
            } else {
                printf("name=missing\n")
            }

            var n_ptr = field(&fields, "n")
            if(n_ptr != null && n_ptr is JsonValue.Number) {
                var Number(num) = *n_ptr else unreachable
                var as_int = num.to_int()
                if(as_int is Result.Ok) {
                    var Ok(i) = as_int else unreachable
                    printf("n=%d\n", i)
                } else {
                    printf("n=not_a_number\n")
                }
            } else {
                printf("n=missing\n")
            }

            var ok_ptr = field(&fields, "ok")
            if(ok_ptr != null && ok_ptr is JsonValue.Bool) {
                var Bool(b) = *ok_ptr else unreachable
                if(b) {
                    printf("ok=true\n")
                } else {
                    printf("ok=false\n")
                }
            } else {
                printf("ok=missing\n")
            }

            var list_ptr = field(&fields, "list")
            if(list_ptr != null && list_ptr is JsonValue.Array) {
                var Array(items) = *list_ptr else unreachable
                var sum : int = 0
                var idx : uint = 0
                while(idx < items.size()) {
                    var item = items.get_ptr(idx)
                    if(item is JsonValue.Number) {
                        var Number(num) = *item else unreachable
                        var as_int = num.to_int()
                        if(as_int is Result.Ok) {
                            var Ok(v) = as_int else unreachable
                            sum = sum + v
                        } else {
                            sum = sum
                        }
                    } else {
                        sum = sum
                    }
                    idx = idx + 1
                }
                printf("list_sum=%d|%zu\n", sum, items.size())
            } else {
                printf("list=missing\n")
            }
        } else {
            printf("not_an_object\n")
        }

        // re-encode: the map's order must match the native backend
        var encoded = json::stringify(&value)
        printf("json=%s\n", encoded.data())
    }
    return 0
}
