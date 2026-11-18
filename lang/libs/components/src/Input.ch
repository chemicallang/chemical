// Shadcn-style Input family with variant/size props.
//
// Each component owns a `style { }` block carrying the base look plus every
// variant/size as `[data-variant=...]` / `[data-size=...]` attribute selectors;
// the component renders matching data attributes so SSR HTML and the client
// bundle agree.

// Typed props (Phase 2) for the form controls. Every field has a default so an
// existing call site keeps compiling, and the values that must stay *absent* on the
// DOM (`placeholder`, `value`, `aria-label`) are rendered with `? … : null`, because
// an empty-string default would otherwise emit `placeholder=""` and make an
// uncontrolled input controlled.
public struct InputProps {
    var type : std::string_view = "text"
    var placeholder : std::string_view = ""
    // controlled value; omit for an uncontrolled input
    var value : std::string_view = ""
    var disabled : bool = false
    // "default" | "error" | "success"
    var variant : std::string_view = "default"
    // "sm" | "default" | "lg"
    var size : std::string_view = "default"
    var name : std::string_view = ""
    var id : std::string_view = ""
    var ariaLabel : std::string_view = ""
}

public #universal Input(props : InputProps) {
    var input = style {
        display: inline-flex;
        align-items: center;
        width: 100%;
        box-sizing: border-box;
        height: 2.5rem;
        padding: 0 0.9rem;
        border-radius: calc(var(--radius) - 2px);
        border: 1px solid hsl(var(--input));
        background: transparent;
        color: hsl(var(--foreground));
        font-size: 0.875rem;
        outline: none;
        transition: border-color 0.15s ease, box-shadow 0.15s ease, background 0.15s ease;
        &::placeholder {
            color: hsl(var(--muted-foreground));
        }
        &:hover {
            border-color: hsl(var(--input) / 0.8);
        }
        &:focus {
            border-color: hsl(var(--ring));
            box-shadow: 0 0 0 3px hsl(var(--ring) / 0.22);
        }
        &:disabled {
            opacity: 0.55;
            cursor: not-allowed;
            background: hsl(var(--muted) / 0.4);
        }
        &[data-variant="filled"] {
            background: hsl(var(--muted) / 0.55);
            border-color: transparent;
            &:hover { border-color: transparent; }
        }
        &[data-variant="ghost"] {
            background: transparent;
            border-color: transparent;
            border-bottom: 1px solid hsl(var(--input));
            border-radius: 0;
            padding-left: 0;
            padding-right: 0;
            &:hover { border-color: hsl(var(--input) / 0.8); }
        }
        &[data-variant="error"] {
            border-color: hsl(var(--destructive) / 0.6);
            &:focus { box-shadow: 0 0 0 3px hsl(var(--destructive) / 0.14); }
        }
        &[data-variant="success"] {
            border-color: hsl(var(--success) / 0.55);
            &:focus { box-shadow: 0 0 0 3px hsl(var(--success) / 0.14); }
        }
        &[data-size="sm"] {
            height: 2.25rem;
            padding: 0 0.75rem;
            font-size: 0.8125rem;
        }
        &[data-size="lg"] {
            height: 2.75rem;
            padding: 0 1rem;
            font-size: 1rem;
        }
    }
    return <input
        {...props}
        type={props.type || "text"}
        placeholder={props.placeholder ? props.placeholder : null}
        value={props.value ? props.value : null}
        disabled={props.disabled}
        data-variant={props.variant || "default"}
        data-size={props.size || "default"}
        class={input}
        class={props.className || props.class}
        onClick={props.onClick}
        onChange={props.onChange}
        id={props.id ? props.id : null}
        name={props.name ? props.name : null}
        aria-label={props.ariaLabel ? props.ariaLabel : null}
    />
}

public struct TextAreaProps {
    var placeholder : std::string_view = ""
    var value : std::string_view = ""
    var disabled : bool = false
    var variant : std::string_view = "default"
    var size : std::string_view = "default"
    var name : std::string_view = ""
    var id : std::string_view = ""
    // 3 is a sane default height; a `rows` of 0 would hide the control
    var rows : int = 3
}

public #universal TextArea(props : TextAreaProps) {
    var textarea = style {
        display: flex;
        width: 100%;
        box-sizing: border-box;
        min-height: 120px;
        padding: 0.75rem 0.9rem;
        border-radius: calc(var(--radius) - 2px);
        border: 1px solid hsl(var(--input));
        background: transparent;
        color: hsl(var(--foreground));
        font-size: 0.875rem;
        font-family: inherit;
        outline: none;
        resize: vertical;
        transition: border-color 0.15s ease, box-shadow 0.15s ease, background 0.15s ease;
        &::placeholder { color: hsl(var(--muted-foreground)); }
        &:hover { border-color: hsl(var(--input) / 0.8); }
        &:focus {
            border-color: hsl(var(--ring));
            box-shadow: 0 0 0 3px hsl(var(--ring) / 0.22);
        }
        &:disabled {
            opacity: 0.55;
            cursor: not-allowed;
        }
        &[data-variant="error"] {
            border-color: hsl(var(--destructive) / 0.6);
            &:focus { box-shadow: 0 0 0 3px hsl(var(--destructive) / 0.14); }
        }
        &[data-size="sm"] {
            min-height: 96px;
            padding: 0.5rem 0.75rem;
            font-size: 0.8125rem;
        }
        &[data-size="lg"] {
            min-height: 150px;
            padding: 1rem;
            font-size: 1rem;
        }
    }
    return <textarea
        {...props}
        placeholder={props.placeholder ? props.placeholder : null}
        value={props.value ? props.value : null}
        disabled={props.disabled}
        data-variant={props.variant || "default"}
        data-size={props.size || "default"}
        class={textarea}
        class={props.className || props.class}
        onClick={props.onClick}
        onChange={props.onChange}
        id={props.id ? props.id : null}
        name={props.name ? props.name : null}
        rows={props.rows}
    >{props.children}</textarea>
}

// Native <select> (kept for plain forms; the styled dropdown lives in
// Select.ch). `placeholder` renders a disabled placeholder option; `variant`/
// `size` mirror the Input props; everything else passes through the spread.
public struct NativeSelectProps {
    var value : std::string_view = ""
    var disabled : bool = false
    var variant : std::string_view = "default"
    var size : std::string_view = "default"
    var name : std::string_view = ""
    var id : std::string_view = ""
    var ariaLabel : std::string_view = ""
    // rendered as a disabled placeholder option when non-empty
    var placeholder : std::string_view = ""
}

public #universal NativeSelect(props : NativeSelectProps) {
    var input = style {
        display: inline-flex;
        align-items: center;
        width: 100%;
        box-sizing: border-box;
        height: 2.5rem;
        padding: 0 0.9rem;
        border-radius: calc(var(--radius) - 2px);
        border: 1px solid hsl(var(--input));
        background: transparent;
        color: hsl(var(--foreground));
        font-size: 0.875rem;
        outline: none;
        transition: border-color 0.15s ease, box-shadow 0.15s ease, background 0.15s ease;
        &::placeholder {
            color: hsl(var(--muted-foreground));
        }
        &:hover {
            border-color: hsl(var(--input) / 0.8);
        }
        &:focus {
            border-color: hsl(var(--ring));
            box-shadow: 0 0 0 3px hsl(var(--ring) / 0.22);
        }
        &:disabled {
            opacity: 0.55;
            cursor: not-allowed;
            background: hsl(var(--muted) / 0.4);
        }
        &[data-variant="filled"] {
            background: hsl(var(--muted) / 0.55);
            border-color: transparent;
            &:hover { border-color: transparent; }
        }
        &[data-variant="ghost"] {
            background: transparent;
            border-color: transparent;
            border-bottom: 1px solid hsl(var(--input));
            border-radius: 0;
            padding-left: 0;
            padding-right: 0;
            &:hover { border-color: hsl(var(--input) / 0.8); }
        }
        &[data-variant="error"] {
            border-color: hsl(var(--destructive) / 0.6);
            &:focus { box-shadow: 0 0 0 3px hsl(var(--destructive) / 0.14); }
        }
        &[data-variant="success"] {
            border-color: hsl(var(--success) / 0.55);
            &:focus { box-shadow: 0 0 0 3px hsl(var(--success) / 0.14); }
        }
        &[data-size="sm"] {
            height: 2.25rem;
            padding: 0 0.75rem;
            font-size: 0.8125rem;
        }
        &[data-size="lg"] {
            height: 2.75rem;
            padding: 0 1rem;
            font-size: 1rem;
        }
    }
    return <select
        {...props}
        value={props.value}
        disabled={props.disabled}
        data-variant={props.variant || "default"}
        data-size={props.size || "default"}
        class={input}
        class={props.className || props.class}
        onChange={props.onChange}
        id={props.id}
        name={props.name}
        aria-label={props.ariaLabel}
    >
        {props.placeholder ? <option value="" disabled>{props.placeholder}</option> : null}
        {props.children}
    </select>
}

// Legacy wrappers — keep every previously-public name working.
public #universal InputFilled(props) {
    return <Input {...props} variant="filled">{props.children}</Input>
}

public #universal InputSuccess(props) {
    return <Input {...props} variant="success">{props.children}</Input>
}

public #universal InputError(props) {
    return <Input {...props} variant="error">{props.children}</Input>
}

public #universal InputGhost(props) {
    return <Input {...props} variant="ghost">{props.children}</Input>
}

public #universal InputSm(props) {
    return <Input {...props} size="sm">{props.children}</Input>
}

public #universal InputLg(props) {
    return <Input {...props} size="lg">{props.children}</Input>
}

public #universal InputDisabled(props) {
    return <Input {...props} disabled={true}>{props.children}</Input>
}

public struct FieldProps {
    // rendered above the control as a FieldLabel when non-empty
    var label : std::string_view = ""
    // rendered below as a FieldHint / FieldError when non-empty
    var hint : std::string_view = ""
    var error : std::string_view = ""
}

public #universal Field(props : FieldProps) {
    var field = style {
        display: grid;
        gap: 0.5rem;
        text-align: left;
    }
    return <label class={field} class={props.className || props.class}>
        {props.label ? <FieldLabel>{props.label}</FieldLabel> : null}
        {props.children}
        {props.hint ? <FieldHint>{props.hint}</FieldHint> : null}
        {props.error ? <FieldError>{props.error}</FieldError> : null}
    </label>
}

// Only `class`/`className`/`children` — an empty struct still rejects unknown
// semantic props at the call site.
public struct SlotProps {
}

public #universal FieldLabel(props : SlotProps) {
    var field_label = style {
        font-size: 0.875rem;
        font-weight: 600;
        line-height: 1;
        color: hsl(var(--foreground));
    }
    return <span class={field_label} class={props.className || props.class}>{props.children}</span>
}

public #universal FieldHint(props : SlotProps) {
    var field_hint = style {
        font-size: 0.8125rem;
        color: hsl(var(--muted-foreground));
    }
    return <span class={field_hint} class={props.className || props.class}>{props.children}</span>
}

public #universal FieldError(props : SlotProps) {
    var field_error = style {
        font-size: 0.8125rem;
        font-weight: 500;
        color: hsl(var(--destructive));
    }
    return <span role="alert" class={field_error} class={props.className || props.class}>{props.children}</span>
}

// Shadcn InputGroup: wraps an Input with leading/trailing icons or addons
public struct InputGroupProps {
    // disables the wrapper's focus ring affordance
    var disabled : bool = false
}

public #universal InputGroup(props : InputGroupProps) {
    var input_group = style {
        display: inline-flex;
        align-items: center;
        width: 100%;
        border-radius: calc(var(--radius) - 2px);
        border: 1px solid hsl(var(--input));
        background: transparent;
        transition: border-color 0.15s ease, box-shadow 0.15s ease;
        &:focus-within {
            border-color: hsl(var(--ring));
            box-shadow: 0 0 0 3px hsl(var(--ring) / 0.22);
        }
        &[data-disabled="true"] {
            opacity: 0.55;
            cursor: not-allowed;
        }
        input {
            border: 0;
            box-shadow: none;
            border-radius: 0;
            background: transparent;
            flex: 1;
            min-width: 0;
            height: 2.5rem;
            padding: 0 0.75rem;
            font-size: 0.875rem;
            outline: none;
            color: hsl(var(--foreground));
            &::placeholder { color: hsl(var(--muted-foreground)); }
            &[data-size="sm"] { height: 2.25rem; font-size: 0.8125rem; }
            &[data-size="lg"] { height: 2.75rem; font-size: 1rem; }
        }
    }
    var disabled = props.disabled || false
    var classes = (props.className || props.class) || ""
    return <div {...props} class={classes + " " + input_group} data-disabled={disabled ? "true" : "false"}>{props.children}</div>
}

public struct InputIconProps {
    // "start" | "end" — trailing icons are ordered after the input
    var position : std::string_view = "start"
}

public #universal InputIcon(props : InputIconProps) {
    var input_icon = style {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        flex-shrink: 0;
        width: 2.5rem;
        height: 100%;
        color: hsl(var(--muted-foreground));
        pointer-events: none;
    }
    var classes = (props.className || props.class) || ""
    var position = props.position || "start"
    var posStyle = position == "end" ? "order:10;" : ""
    return <span class={classes + " " + input_icon} style={posStyle}>{props.children}</span>
}

public #universal InputAddon(props : SlotProps) {
    var input_addon = style {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        flex-shrink: 0;
        padding: 0 0.75rem;
        font-size: 0.875rem;
        color: hsl(var(--muted-foreground));
        border-left: 1px solid hsl(var(--border));
        background: hsl(var(--muted) / 0.4);
    }
    var classes = (props.className || props.class) || ""
    return <span class={classes + " " + input_addon}>{props.children}</span>
}
