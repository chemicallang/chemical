// Shadcn-style RadioGroup: a set of mutually-exclusive radio options built from
// children:
//
//      <RadioGroup name="plan" defaultValue="pro" onValueChange={(v) => ...}>
//          <RadioGroupItem value="free">Free</RadioGroupItem>
//          <RadioGroupItem value="pro">Pro</RadioGroupItem>
//      </RadioGroup>
//
// The group owns the selection via context (keyed by `name`, defaulting to
// "default"). `defaultValue` sets the initial selection; `value` switches to
// controlled mode (the write callback still updates the context so items stay
// in sync after clicks). `disabled` dims the whole group; `direction`
// ("row" | "column") lays out the items. Other props pass through via the
// spread.
//
// Context model (see universal skill, "Context system"): the group publishes
// its selection signal to the runtime registry under `rg-<name>`, items read
// it back reactively. The group threads its `name` into item vnodes so items
// resolve their key without repeating `name` on every item.

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
    const ctx = createContext("rg-" + (props.name || "default"), "")
    var disabled = props.disabled || false
    var direction = props.direction || "column"
    // Thread the group name (and input name) into item children so items can
    // resolve their context key without repeating `name` on every item.
    if(props.children && props.children.map) {
        props.children = props.children.map((c) => {
            if(c && c.p && c.p.props) {
                c.p.props.__rgName = props.name || "default"
                if(props.name && !c.p.props.name) { c.p.props.name = props.name }
            }
            return c
        })
    }
    // Publish the selection to consumers. Assigning the state signal wires the
    // context to follow it; writes also update it directly so controlled mode
    // keeps items in sync.
    ctx.value = value
    ctx.write = (v) => {
        if(disabled) { return }
        ctx.value = v
        if(props.value != null) {
            if(props.onValueChange) { props.onValueChange(v) }
        } else {
            value = v
            if(props.onValueChange) { props.onValueChange(v) }
        }
    }
    return <div role="radiogroup" {...props} class={${radio_group_styles(page)}} data-direction={direction} data-disabled={disabled ? "true" : "false"}>{props.children}</div>
}

public #universal RadioGroupItem(props) {
    const ctx = useContext("rg-" + (props.__rgName || props.name || "default"))
    var disabled = props.disabled || false
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var onSelect = () => {
        if(disabled) { return }
        if(ctx.write) { ctx.write(props.value) }
    }
    return <label {...props} class={classes + " " + ${radio_item_styles(page)}} data-disabled={disabled ? "true" : "false"}>
        <input type="radio" class="chx-radio-input" checked={ctx.value == props.value} disabled={disabled} name={props.name} value={props.value} onChange={onSelect} id={props.id} aria-label={props.ariaLabel} />
        <span class="chx-radio-box">
            <span class="chx-radio-dot"></span>
        </span>
        <span>{props.children}</span>
    </label>
}
