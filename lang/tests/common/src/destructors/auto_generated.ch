// ============================================================
// Tests for auto-generated destructors
//
// When a struct contains fields with destructible types but does
// NOT have its own @delete destructor, the compiler should
// auto-generate one that calls destructors of all destructible fields.
//
// This is critical for types like std::function<> which have
// internal destructors for capture memory.
//
// These tests verify auto-generation works across:
//   - Simple containment (field type has destructor)
//   - Generic structs with concrete destructible fields
//   - Generic structs with generic destructible fields
//   - Struct ordering (forward references)
//   - Composition chains (auto -> auto -> manual)
// ============================================================

// --- Helper: a field type with a manually written destructor ---

struct AutoGenCountedField {
    var counter : *mut int
    @delete
    func delete(&self) {
        *counter = *counter + 1
    }
}

// ─────────────────────────────────────────────────────────────
// Scenario 1: Simple struct containing a destructible field
// ─────────────────────────────────────────────────────────────
// struct Container { var field : SomeTypeWithDestructor }
// Compiler MUST auto-generate ~Container() that calls field's destructor

struct S1_SimpleDestrContainer {
    var field : AutoGenCountedField
}

// ─────────────────────────────────────────────────────────────
// Scenario 2: Generic struct with a concrete destructible field
// The field type is NOT generic — it's always AutoGenCountedField.
// Destructor should be auto-generated regardless of what T is.
// ─────────────────────────────────────────────────────────────

struct S2_GenericStructWithConcreteField<T> {
    var field : AutoGenCountedField
}

// ─────────────────────────────────────────────────────────────
// Scenario 3: Generic struct with a generic field T,
// where T is instantiated to a destructible type.
// Destructor for S3_DataContainer<AutoGenCountedField> must be generated.
// ─────────────────────────────────────────────────────────────

struct S3_DataContainer<T> {
    var data : T
}

// ─────────────────────────────────────────────────────────────
// Scenario 4: Struct ordering — middle struct references both
// a "before" struct (defined earlier, manual destructor)
// and an "after" struct (defined later, auto-generated destructor).
//
// S4_EarlyManual (defined FIRST, manual @delete)
// S4_MiddleComposer (defined SECOND, references both)
// S4_LateAutoField (defined THIRD, auto-generated destructor)
//
// The middle struct's auto-generated destructor must:
//   a) Call S4_EarlyManual's @delete (the manual one) through the S4_LateAutoField's destructor
// ─────────────────────────────────────────────────────────────

struct S4_EarlyManual {         // defined FIRST
    var counter : *mut int
    @delete
    func delete(&self) {
        *counter = *counter + 1
    }
}

struct S4_MiddleComposer {      // defined SECOND
    var after_field  : S4_LateAutoField  // auto destructor, defined below
}

struct S4_LateAutoField {       // defined THIRD (auto-generated destructor)
    var tracked : S4_EarlyManual
}

// ─────────────────────────────────────────────────────────────
// Scenario 5: Struct composed of another struct defined ABOVE it
// that has a manually-written destructor.
// ─────────────────────────────────────────────────────────────

struct S5_WrapperWithManual {
    var inner : S4_EarlyManual
}

// ─────────────────────────────────────────────────────────────
// Scenario 6: Struct composed of another struct defined ABOVE it
// that has its own auto-generated destructor (which in turn
// comes from a manual field).
// ─────────────────────────────────────────────────────────────

struct S6_WrapperWithAuto {
    var inner : S4_LateAutoField
}

// ─────────────────────────────────────────────────────────────
// Scenario 7: Deep chain — three levels of auto-generated
// destructors ending in a manual one.
// S7_DeepInner (manual)
// S7_MidLevel  (auto-generated from S7_DeepInner)
// S7_TopLevel  (auto-generated from S7_MidLevel)
// ─────────────────────────────────────────────────────────────

struct S7_DeepInner {
    var counter : *mut int
    @delete
    func delete(&self) {
        *counter = *counter + 1
    }
}

struct S7_MidLevel {
    var inner : S7_DeepInner
}

struct S7_TopLevel {
    var inner : S7_MidLevel
}

// ─────────────────────────────────────────────────────────────
// Scenario 8: Generic struct with two generic fields, both
// destructible types, verifying both get destructed.
// ─────────────────────────────────────────────────────────────

struct S8_DualGeneric<T, U> {
    var first  : T
    var second : U
}

// ─────────────────────────────────────────────────────────────
// Scenario 9: Struct with array of destructible elements.
// The struct has no manual destructor, so an auto-generated one
// should iterate and destruct each array element.
// ─────────────────────────────────────────────────────────────

struct S9_ArrayContainer {
    var arr : [5]AutoGenCountedField
}

// ─────────────────────────────────────────────────────────────
// Test runners
// ─────────────────────────────────────────────────────────────

func test_auto_generated_simple_containment() {
    test("struct with destructible field auto-generates destructor", () => {
        var counter = 0
        {
            var s = S1_SimpleDestrContainer {
                field : AutoGenCountedField { counter : &raw mut counter }
            }
        }
        return counter == 1
    })
}

func test_auto_generated_generic_concrete_field() {
    test("generic struct with concrete destructible field auto-generates destructor", () => {
        var counter = 0
        {
            var s = S2_GenericStructWithConcreteField<int> {
                field : AutoGenCountedField { counter : &raw mut counter }
            }
        }
        return counter == 1
    })

    test("generic struct with concrete destructible field, T=uint, auto-generates destructor", () => {
        var counter = 0
        {
            var s = S2_GenericStructWithConcreteField<uint> {
                field : AutoGenCountedField { counter : &raw mut counter }
            }
        }
        return counter == 1
    })
}

func test_auto_generated_generic_field() {
    test("generic struct with generic destructible field auto-generates destructor", () => {
        var counter = 0
        {
            var s = S3_DataContainer<AutoGenCountedField> {
                data : AutoGenCountedField { counter : &raw mut counter }
            }
        }
        return counter == 1
    })
}

func test_auto_generated_composition_chain() {
    test("middle struct with before (manual) and after (auto) destructor references", () => {
        var counter = 0
        {
            var s = S4_MiddleComposer {
                after_field  : S4_LateAutoField {
                    tracked : S4_EarlyManual { counter : &raw mut counter }
                }
            }
        }
        return counter == 1
    })
}

func test_auto_generated_wrapper_with_manual() {
    test("wrapper struct with manually-destructible inner auto-generates destructor", () => {
        var counter = 0
        {
            var s = S5_WrapperWithManual {
                inner : S4_EarlyManual { counter : &raw mut counter }
            }
        }
        return counter == 1
    })
}

func test_auto_generated_wrapper_with_auto() {
    test("wrapper struct with auto-destructor inner auto-generates destructor", () => {
        var counter = 0
        {
            var s = S6_WrapperWithAuto {
                inner : S4_LateAutoField {
                    tracked : S4_EarlyManual { counter : &raw mut counter }
                }
            }
        }
        return counter == 1
    })
}

func test_auto_generated_deep_chain() {
    test("three-level auto-generated destructor chain ends with manual destructor called", () => {
        var counter = 0
        {
            var s = S7_TopLevel {
                inner : S7_MidLevel {
                    inner : S7_DeepInner { counter : &raw mut counter }
                }
            }
        }
        return counter == 1
    })
}

func test_auto_generated_dual_generic() {
    test("dual generic field struct auto-generates destructor that destructs both fields", () => {
        var counter1 = 0
        var counter2 = 0
        {
            var s = S8_DualGeneric<AutoGenCountedField, AutoGenCountedField> {
                first  : AutoGenCountedField { counter : &raw mut counter1 },
                second : AutoGenCountedField { counter : &raw mut counter2 }
            }
        }
        return counter1 == 1 && counter2 == 1
    })
}

func test_auto_generated_array_container() {
    test("struct with array of destructible elements auto-generates destructor that destructs all", () => {
        var counters = 0
        {
            var s = S9_ArrayContainer {
                arr : [
                    AutoGenCountedField { counter : &raw mut counters },
                    AutoGenCountedField { counter : &raw mut counters },
                    AutoGenCountedField { counter : &raw mut counters },
                    AutoGenCountedField { counter : &raw mut counters },
                    AutoGenCountedField { counter : &raw mut counters }
                ]
            }
        }
        return counters == 5
    })
}

// ─────────────────────────────────────────────────────────────
// Master runner
// ─────────────────────────────────────────────────────────────

func test_auto_generated_destructors() {
    test_auto_generated_simple_containment()
    test_auto_generated_generic_concrete_field()
    test_auto_generated_generic_field()
    test_auto_generated_composition_chain()
    test_auto_generated_wrapper_with_manual()
    test_auto_generated_wrapper_with_auto()
    test_auto_generated_deep_chain()
    test_auto_generated_dual_generic()
    test_auto_generated_array_container()
}
