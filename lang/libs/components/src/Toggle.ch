func toggle_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        gap: 0.625rem;
        color: hsl(var(--foreground));
        font-weight: 500;
        cursor: pointer;
        position: relative;
        user-select: none;
        .chx-toggle-input {
            position: absolute;
            inset: 0;
            width: 100%;
            height: 100%;
            opacity: 0;
            margin: 0;
            cursor: pointer;
            z-index: 1;
            &:focus-visible + .chx-checkbox-box,
            &:focus-visible + .chx-radio-box,
            &:focus-visible + .chx-switch-track {
                box-shadow: 0 0 0 4px hsl(var(--ring) / 0.4);
            }
            &:checked + .chx-checkbox-box {
                background: hsl(var(--primary));
                border-color: hsl(var(--primary));
                box-shadow: 0 0 0 4px hsl(var(--ring) / 0.15);
            }
            &:checked + .chx-checkbox-box .chx-checkbox-mark {
                opacity: 1;
            }
            &:checked + .chx-radio-box {
                border-color: hsl(var(--primary));
                box-shadow: 0 0 0 4px hsl(var(--ring) / 0.15);
            }
            &:checked + .chx-radio-box .chx-radio-dot {
                opacity: 1;
                transform: scale(1);
            }
            &:checked + .chx-switch-track {
                background: hsl(var(--primary));
                border-color: hsl(var(--primary));
                box-shadow: 0 0 0 4px hsl(var(--ring) / 0.15);
            }
            &:checked + .chx-switch-track .chx-switch-thumb {
                transform: translateX(20px);
                background: hsl(var(--primary-foreground));
            }
        }
        .chx-checkbox-box {
            width: 18px;
            height: 18px;
            border-radius: 6px;
            border: 1px solid hsl(var(--border));
            background: hsl(var(--card));
            display: inline-flex;
            align-items: center;
            justify-content: center;
            transition: background 0.18s ease, border-color 0.18s ease, box-shadow 0.18s ease;
            flex-shrink: 0;
        }
        .chx-checkbox-mark {
            width: 5px;
            height: 9px;
            border-right: 2px solid hsl(var(--primary-foreground));
            border-bottom: 2px solid hsl(var(--primary-foreground));
            transform: rotate(45deg) scale(0.85);
            opacity: 0;
            transition: opacity 0.18s ease;
            margin-top: -1px;
        }
        .chx-radio-box {
            width: 18px;
            height: 18px;
            border-radius: 999px;
            border: 1px solid hsl(var(--border));
            background: hsl(var(--card));
            display: inline-flex;
            align-items: center;
            justify-content: center;
            transition: background 0.18s ease, border-color 0.18s ease, box-shadow 0.18s ease;
            flex-shrink: 0;
        }
        .chx-radio-dot {
            width: 8px;
            height: 8px;
            border-radius: 999px;
            background: hsl(var(--primary));
            transform: scale(0.5);
            opacity: 0;
            transition: opacity 0.18s ease, transform 0.18s ease;
        }
        .chx-switch-track {
            width: 44px;
            height: 24px;
            border-radius: 999px;
            background: hsl(var(--muted));
            border: 1px solid hsl(var(--border));
            position: relative;
            display: inline-block;
            transition: background 0.18s ease, border-color 0.18s ease, box-shadow 0.18s ease;
            flex-shrink: 0;
        }
        .chx-switch-thumb {
            width: 18px;
            height: 18px;
            border-radius: 50%;
            background: hsl(var(--muted-foreground));
            position: absolute;
            left: 2px;
            top: 2px;
            transition: transform 0.18s ease, background 0.18s ease;
            box-shadow: 0 2px 6px rgba(15, 23, 42, 0.25);
        }

        &[data-size="sm"] {
            gap: 0.5rem;
            .chx-checkbox-box { width: 16px; height: 16px; border-radius: 5px; }
            .chx-radio-box { width: 16px; height: 16px; }
            .chx-radio-dot { width: 7px; height: 7px; }
            .chx-switch-track { width: 36px; height: 20px; }
            .chx-switch-thumb { width: 14px; height: 14px; left: 2px; top: 2px; }
            .chx-toggle-input:checked + .chx-switch-track .chx-switch-thumb { transform: translateX(16px); }
        }
        &[data-size="lg"] {
            gap: 0.75rem;
            .chx-checkbox-box { width: 22px; height: 22px; border-radius: 7px; }
            .chx-radio-box { width: 22px; height: 22px; }
            .chx-radio-dot { width: 10px; height: 10px; }
            .chx-switch-track { width: 52px; height: 28px; }
            .chx-switch-thumb { width: 22px; height: 22px; left: 2px; top: 2px; }
            .chx-toggle-input:checked + .chx-switch-track .chx-switch-thumb { transform: translateX(24px); }
        }

        &[data-disabled="true"] {
            opacity: 0.55;
            cursor: not-allowed;
            .chx-checkbox-box, .chx-radio-box, .chx-switch-track {
                background: hsl(var(--muted) / 0.6);
                border-color: hsl(var(--border));
            }
        }
    }
}

// Every prop is optional: `checked` drives state, `disabled` dims the
// control, `size` (sm/md/lg) scales it, `id`/`name` wire up native form
// semantics, and `onChange`/`onClick` both fire on interaction
// (onChange receives the native change event). className merges with the
// control styles.
public #universal Checkbox(props) {
    var size = props.size || "md"
    var disabled = props.disabled || false
    var classes = (props.className || props.class) || ""
    return <label {...props} class={${toggle_styles(page)}} class={classes} data-size={size} data-disabled={disabled ? "true" : "false"}>
        <input type="checkbox" checked={props.checked} disabled={disabled} onChange={props.onChange} id={props.id} name={props.name} aria-label={props.ariaLabel} class="chx-toggle-input" />
        <span class="chx-checkbox-box">
            <span class="chx-checkbox-mark"></span>
        </span>
        <span>{props.children}</span>
    </label>
}

public #universal Radio(props) {
    var size = props.size || "md"
    var disabled = props.disabled || false
    var classes = (props.className || props.class) || ""
    return <label {...props} class={${toggle_styles(page)}} class={classes} data-size={size} data-disabled={disabled ? "true" : "false"}>
        <input type="radio" checked={props.checked} disabled={disabled} name={props.name} onChange={props.onChange} id={props.id} aria-label={props.ariaLabel} class="chx-toggle-input" />
        <span class="chx-radio-box">
            <span class="chx-radio-dot"></span>
        </span>
        <span>{props.children}</span>
    </label>
}

public #universal Switch(props) {
    var size = props.size || "md"
    var disabled = props.disabled || false
    var classes = (props.className || props.class) || ""
    return <label {...props} class={${toggle_styles(page)}} class={classes} data-size={size} data-disabled={disabled ? "true" : "false"}>
        <input type="checkbox" checked={props.checked} disabled={disabled} onChange={props.onChange} id={props.id} name={props.name} aria-label={props.ariaLabel} class="chx-toggle-input" />
        <span class="chx-switch-track">
            <span class="chx-switch-thumb"></span>
        </span>
        <span>{props.children}</span>
    </label>
}
