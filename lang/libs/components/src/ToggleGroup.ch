// Shadcn-style ToggleGroup: a group of toggle buttons.
//
// Props:
//   type           "single" (default, one pressed at a time) | "multiple"
//   options        required array of strings (value == label)
//   value          controlled selection; a string (single) or array (multiple)
//   defaultValue   uncontrolled initial selection; string or array
//   onValueChange  fired with the new selection (string or array)
//   variant        "default" | "outline" | "ghost" (default "default")
//   size           "sm" | "default" | "lg" (default "default")
//   disabled       locks all items
//   className      merged with the group class
//
// In single mode the selection is a string; in multiple mode it is an array.
// Like Tabs/Pagination, the group owns selection state and renders its items
// from `options` (value == label). For custom labels, use children with
// <ToggleGroupItem pressed={...} onClick={...}>.

func toggle_group_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        flex-wrap: wrap;
        align-items: center;
        gap: 0.25rem;
        border-radius: var(--radius-md);
        &[data-disabled="true"] {
            opacity: 0.55;
        }
    }
}

func toggle_group_item_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        gap: 0.375rem;
        height: 2.25rem;
        padding: 0 0.75rem;
        border-radius: calc(var(--radius) - 2px);
        border: 1px solid transparent;
        background: transparent;
        color: hsl(var(--muted-foreground));
        font-size: 0.8125rem;
        font-weight: 500;
        cursor: pointer;
        user-select: none;
        transition: background-color 0.15s ease, color 0.15s ease, border-color 0.15s ease, box-shadow 0.15s ease;
        &:hover {
            background: hsl(var(--accent));
            color: hsl(var(--accent-foreground));
        }
        &:focus-visible {
            outline: 2px solid hsl(var(--ring));
            outline-offset: 2px;
        }
        &[data-pressed="true"] {
            background: hsl(var(--accent));
            color: hsl(var(--accent-foreground));
        }
        &[data-variant="outline"] {
            border-color: hsl(var(--border));
            &[data-pressed="true"] {
                background: hsl(var(--accent));
                border-color: hsl(var(--border));
            }
        }
        &[data-variant="ghost"] {
            &[data-pressed="true"] {
                background: hsl(var(--accent));
            }
        }
        &[data-size="sm"] {
            height: 1.875rem;
            padding: 0 0.625rem;
            font-size: 0.75rem;
        }
        &[data-size="lg"] {
            height: 2.625rem;
            padding: 0 1rem;
            font-size: 0.9375rem;
        }
        &[data-disabled="true"] {
            opacity: 0.5;
            pointer-events: none;
        }
    }
}

public #universal ToggleGroup(props) {
    var multiple = props.type == "multiple"
    state selected = props.defaultValue || ""
    var current = props.value != null ? props.value : selected
    var disabled = props.disabled || false
    var variant = props.variant || "default"
    var size = props.size || "default"
    var toggle = (v) => {
        if(disabled) {
            return
        }
        var next = current
        if(multiple) {
            if(current.indexOf(v) != -1) {
                next = current.filter(x => x != v)
            } else {
                next = current.concat(v)
            }
        } else {
            next = v
        }
        if(props.value != null) {
            if(props.onValueChange) { props.onValueChange(next) }
        } else {
            selected = next
            if(props.onValueChange) { props.onValueChange(next) }
        }
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    if(props.options) {
        if(multiple) {
            // Multiple mode: the pressed set is an array; SSR shows the initial
            // selection via defaultValue comparison and hydration refines it.
            return <div role="group" class={classes + " " + ${toggle_group_styles(page)}} data-disabled={disabled ? "true" : "false"}>
                {props.options.map((opt, i) => (
                    <ToggleGroupItem value={opt} pressed={current.indexOf(opt) != -1} disabled={disabled} variant={variant} size={size} onClick={() => toggle(opt)}>{opt}</ToggleGroupItem>
                ))}
            </div>
        }
        return <div role="group" class={classes + " " + ${toggle_group_styles(page)}} data-disabled={disabled ? "true" : "false"}>
            {props.options.map((opt, i) => (
                <ToggleGroupItem value={opt} pressed={current == opt} disabled={disabled} variant={variant} size={size} onClick={() => toggle(opt)}>{opt}</ToggleGroupItem>
            ))}
        </div>
    }
    return <div role="group" class={classes + " " + ${toggle_group_styles(page)}} data-disabled={disabled ? "true" : "false"}>{props.children}</div>
}

public #universal ToggleGroupItem(props) {
    var pressed = props.pressed || false
    var disabled = props.disabled || false
    var variant = props.variant || "default"
    var size = props.size || "default"
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <button type="button" aria-pressed={pressed ? "true" : "false"} data-pressed={pressed ? "true" : "false"} data-variant={variant} data-size={size} data-disabled={disabled ? "true" : "false"} disabled={disabled} onClick={props.onClick} class={classes + " " + ${toggle_group_item_styles(page)}}>{props.children}</button>
}
