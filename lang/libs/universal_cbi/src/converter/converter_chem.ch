func (converter : &mut JsConverter) put_wrapping(value : *mut Value) {
    const wrapped = converter.builder.make_value_wrapper(value, converter.parent)
    converter.vec.push(wrapped);
}

func (converter : &mut JsConverter) put_wrapped_chemical_value_in(value : *mut Value) {
    // Strings are embedded inside JS string literals at the call sites, so they
    // must be escaped for that context (quotes/backslashes/control chars).
    if(converter.target == BufferType.JavaScript) {
        const chain = converter.make_escaped_char_ptr_value_call(value)
        converter.vec.push(chain)
    } else {
        const chain = converter.make_char_ptr_value_call(value)
        converter.vec.push(chain)
    }
}

func (converter : &mut JsConverter) put_wrapped_chemical_char_value_in(value : *mut Value) {
    var chain = converter.make_char_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_wrapped_chemical_integer_value_in(value : *mut Value) {
    var chain = converter.make_integer_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_wrapped_chemical_uinteger_value_in(value : *mut Value) {
    var chain = converter.make_uinteger_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_wrapped_chemical_float_value_in(value : *mut Value) {
    var chain = converter.make_float_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_wrapped_chemical_double_value_in(value : *mut Value) {
    var chain = converter.make_double_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_by_type(type : *mut BaseType, value : *mut Value) {
    switch(type.getKind()) {
        BaseTypeKind.Void => {
            converter.put_wrapping(value);
        }
        BaseTypeKind.IntN => {
            const intN = type as *mut IntNType;
            const kind = intN.get_intn_type_kind()
            if(kind == IntNTypeKind.Char || kind == IntNTypeKind.UChar) {
                converter.put_wrapped_chemical_char_value_in(value)
            } else if(kind <= IntNTypeKind.Int128) {
                // signed
                converter.put_wrapped_chemical_integer_value_in(value)
            } else {
                // unsigned
                converter.put_wrapped_chemical_uinteger_value_in(value)
            }
        }
        BaseTypeKind.Float => {
            converter.put_wrapped_chemical_float_value_in(value)
        }
        BaseTypeKind.Double => {
            converter.put_wrapped_chemical_double_value_in(value)
        }
        BaseTypeKind.ExpressiveString => {
            if(value.getKind() == ValueKind.ExpressiveString) {
                const exprString = value as *mut ExpressiveString
                const values = exprString.getValues()
                const size = values.size()
                var i = 0u;
                while(i < size) {
                    const ptr = values.get(i)
                    converter.put_by_type(ptr.getType(), ptr);
                    i++;
                }
            }
        }
        default => {
            converter.put_wrapped_chemical_value_in(value);
        }
    }
}

func (converter : &mut JsConverter) put_chemical_value_in(value : *mut Value) {
    converter.put_by_type(value.getType(), value)
}

func (converter : &mut JsConverter) convertChemicalValue(chem : *mut JsChemicalValue) {
    // Inside the JS bundle, a ${...} embed of a string value must be wrapped in
    // quotes so the emitted JS text references a string literal instead of a bare
    // identifier (the mangled Chemical variable name). Numeric/boolean embeds
    // stay unquoted — put_by_type handles their serialization.
    var quoted = false;
    if(converter.target == BufferType.JavaScript && chem.value != null) {
        const cvType = chem.value.getType();
        const isStr = cvType != null && (cvType.getKind() == BaseTypeKind.String ||
            cvType.getKind() == BaseTypeKind.Pointer ||
            cvType.getKind() == BaseTypeKind.ExpressiveString);
        if(isStr) {
            converter.str.append('"');
            quoted = true;
        }
    }
    converter.put_chain_in()
    converter.put_chemical_value_in(chem.value)
    if(quoted) {
        converter.str.append('"');
    }
}
