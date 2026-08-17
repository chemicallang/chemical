// Shadcn-style Input family with variant/size props.
//
// One `input_styles` block carries the base look plus every variant/size as
// `[data-variant=...]` / `[data-size=...]` attribute selectors; the component
// renders matching data attributes so SSR HTML and the client bundle agree.

func input_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        width: 100%;
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
}

func textarea_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        width: 100%;
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
}

func field_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: grid;
        gap: 0.5rem;
        text-align: left;
    }
}

func field_label_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 0.875rem;
        font-weight: 600;
        line-height: 1;
        color: hsl(var(--foreground));
    }
}

func field_hint_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 0.8125rem;
        color: hsl(var(--muted-foreground));
    }
}

func field_error_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 0.8125rem;
        font-weight: 500;
        color: hsl(var(--destructive));
    }
}

public #universal Input(props) {
    return <input
        {...props}
        type={props.type || "text"}
        placeholder={props.placeholder}
        value={props.value}
        disabled={props.disabled}
        data-variant={props.variant || "default"}
        data-size={props.size || "default"}
        class={${input_styles(page)}}
        class={props.className || props.class}
        onClick={props.onClick}
        onChange={props.onChange}
        id={props.id}
        name={props.name}
        aria-label={props.ariaLabel}
    />
}

public #universal TextArea(props) {
    return <textarea
        {...props}
        placeholder={props.placeholder}
        value={props.value}
        disabled={props.disabled}
        data-variant={props.variant || "default"}
        data-size={props.size || "default"}
        class={${textarea_styles(page)}}
        class={props.className || props.class}
        onClick={props.onClick}
        onChange={props.onChange}
        id={props.id}
        name={props.name}
        rows={props.rows}
    >{props.children}</textarea>
}

// Native <select> (kept for plain forms; the styled dropdown lives in
// Select.ch). `placeholder` renders a disabled placeholder option; `variant`/
// `size` mirror the Input props; everything else passes through the spread.
public #universal NativeSelect(props) {
    return <select
        {...props}
        value={props.value}
        disabled={props.disabled}
        data-variant={props.variant || "default"}
        data-size={props.size || "default"}
        class={${input_styles(page)}}
        class={props.className || props.class}
        onChange={props.onChange}
        id={props.id}
        name={props.name}
        aria-label={props.ariaLabel}
    >
        {props.placeholder !== undefined ? <option value="" disabled>{props.placeholder}</option> : null}
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

public #universal Field(props) {
    return <label class={${field_styles(page)}} class={props.className || props.class}>
        {props.label !== undefined ? <FieldLabel>{props.label}</FieldLabel> : null}
        {props.children}
        {props.hint !== undefined ? <FieldHint>{props.hint}</FieldHint> : null}
        {props.error !== undefined ? <FieldError>{props.error}</FieldError> : null}
    </label>
}

public #universal FieldLabel(props) {
    return <span class={${field_label_styles(page)}} class={props.className || props.class}>{props.children}</span>
}

public #universal FieldHint(props) {
    return <span class={${field_hint_styles(page)}} class={props.className || props.class}>{props.children}</span>
}

public #universal FieldError(props) {
    return <span role="alert" class={${field_error_styles(page)}} class={props.className || props.class}>{props.children}</span>
}
