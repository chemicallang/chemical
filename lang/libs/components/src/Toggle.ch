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
            opacity: 0;
            width: 1px;
            height: 1px;
            margin: 0;
            pointer-events: none;
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
    }
}

public #universal Checkbox(props) {
    return <label class={${toggle_styles(page)}}>
        <input type="checkbox" checked={props.checked} onClick={props.onClick} class="chx-toggle-input" />
        <span class="chx-checkbox-box">
            <span class="chx-checkbox-mark"></span>
        </span>
        <span>{props.children}</span>
    </label>
}

public #universal Radio(props) {
    return <label class={${toggle_styles(page)}}>
        <input type="radio" checked={props.checked} name={props.name} onClick={props.onClick} class="chx-toggle-input" />
        <span class="chx-radio-box">
            <span class="chx-radio-dot"></span>
        </span>
        <span>{props.children}</span>
    </label>
}

public #universal Switch(props) {
    return <label class={${toggle_styles(page)}}>
        <input type="checkbox" checked={props.checked} onClick={props.onClick} class="chx-toggle-input" />
        <span class="chx-switch-track">
            <span class="chx-switch-thumb"></span>
        </span>
        <span>{props.children}</span>
    </label>
}
