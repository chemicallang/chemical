// Shadcn-style RadioGroup: a set of mutually-exclusive radio options.
//
// Two modes:
//
// 1. Options mode (stateful, like Tabs/Pagination):
//      <RadioGroup options={["a", "b", "c"]} defaultValue="b" onValueChange={...} />
//    `options` is an array of strings (value == label). The group owns the
//    selection; `value` switches to controlled mode, `defaultValue` sets the
//    initial (and SSR) selection, `onValueChange` fires with the new value.
//
// 2. Children mode: wrap explicit <RadioGroupItem> children. Each item takes
//    `value`, `checked`, `onClick`/`onChange` and a `name` — the parent owns
//    the state (useful for custom labels).
//
// `disabled` dims the whole group; `direction` ("row" | "column") lays out the
// items. Other props pass through via the spread.

func radio_group_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        gap: 0.75rem 1.25rem;
        &[data-direction="column"] {
            flex-direction: column;
            align-items: flex-start;
        }
        &[data-direction="row"] {
            flex-direction: row;
            flex-wrap: wrap;
            align-items: center;
        }
        &[data-disabled="true"] {
            opacity: 0.55;
        }
    }
}

func radio_item_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        gap: 0.5rem;
        font-size: 0.875rem;
        font-weight: 500;
        color: hsl(var(--foreground));
        cursor: pointer;
        user-select: none;
        position: relative;
        .chx-radio-box {
            width: 18px;
            height: 18px;
            border-radius: 999px;
            border: 1px solid hsl(var(--border));
            background: hsl(var(--card));
            display: inline-flex;
            align-items: center;
            justify-content: center;
            flex-shrink: 0;
            transition: border-color 0.18s ease, box-shadow 0.18s ease;
        }
        .chx-radio-dot {
            width: 8px;
            height: 8px;
            border-radius: 999px;
            background: hsl(var(--primary));
            transform: scale(0.4);
            opacity: 0;
            transition: opacity 0.18s ease, transform 0.18s ease;
        }
        .chx-radio-input {
            position: absolute;
            inset: 0;
            width: 100%;
            height: 100%;
            opacity: 0;
            margin: 0;
            cursor: pointer;
            z-index: 1;
            &:focus-visible + .chx-radio-box {
                box-shadow: 0 0 0 4px hsl(var(--ring) / 0.4);
            }
            &:checked + .chx-radio-box {
                border-color: hsl(var(--primary));
                box-shadow: 0 0 0 4px hsl(var(--ring) / 0.15);
            }
            &:checked + .chx-radio-box .chx-radio-dot {
                opacity: 1;
                transform: scale(1);
            }
        }
        &[data-disabled="true"] {
            opacity: 0.55;
            cursor: not-allowed;
        }
    }
}

public #universal RadioGroup(props) {
    state value = props.defaultValue || ""
    var current = props.value != null ? props.value : value
    var disabled = props.disabled || false
    var direction = props.direction || "column"
    var select = (v) => {
        if(disabled) {
            return
        }
        if(props.value != null) {
            if(props.onValueChange) { props.onValueChange(v) }
        } else {
            value = v
            if(props.onValueChange) { props.onValueChange(v) }
        }
    }
    if(props.options) {
        return <div role="radiogroup" {...props} class={${radio_group_styles(page)}} data-direction={direction} data-disabled={disabled ? "true" : "false"}>
            {props.options.map((opt, i) => (
                <RadioGroupItem value={opt} checked={current == opt} disabled={disabled} name={props.name} onClick={() => select(opt)}>{opt}</RadioGroupItem>
            ))}
        </div>
    }
    return <div role="radiogroup" {...props} class={${radio_group_styles(page)}} data-direction={direction} data-disabled={disabled ? "true" : "false"}>{props.children}</div>
}

public #universal RadioGroupItem(props) {
    var disabled = props.disabled || false
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    // `checked` is passed straight through to the input (not through a local)
    // so the runtime keeps it reactive: when the parent group's selection
    // changes, the signal recomputes and this radio updates.
    return <label {...props} class={classes + " " + ${radio_item_styles(page)}} data-disabled={disabled ? "true" : "false"}>
        <input type="radio" class="chx-radio-input" checked={props.checked} disabled={disabled} name={props.name} value={props.value} onChange={props.onChange} id={props.id} aria-label={props.ariaLabel} />
        <span class="chx-radio-box">
            <span class="chx-radio-dot"></span>
        </span>
        <span>{props.children}</span>
    </label>
}
