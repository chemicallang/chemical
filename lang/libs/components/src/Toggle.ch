func toggle_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        gap: 0.75rem;
        color: var(--chx-text-main);
        font-weight: 600;
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
        }
        .chx-checkbox-box {
            width: 18px;
            height: 18px;
            border-radius: 4px;
            border: 1px solid var(--chx-border-strong);
            background: var(--chx-surface);
            display: inline-flex;
            align-items: center;
            justify-content: center;
            transition: background 0.18s ease, border-color 0.18s ease, box-shadow 0.18s ease;
            &::after {
                content: '';
                width: 5px;
                height: 9px;
                border-right: 2px solid #fff;
                border-bottom: 2px solid #fff;
                transform: rotate(45deg) scale(0.85);
                opacity: 0;
                transition: opacity 0.18s ease;
                margin-top: -1px;
            }
        }
        .chx-radio-box {
            width: 18px;
            height: 18px;
            border-radius: 999px;
            border: 1px solid var(--chx-border-strong);
            background: var(--chx-surface);
            display: inline-flex;
            align-items: center;
            justify-content: center;
            transition: background 0.18s ease, border-color 0.18s ease, box-shadow 0.18s ease;
        }
        .chx-radio-box::after {
            content: '';
            width: 8px;
            height: 8px;
            border-radius: 999px;
            background: #fff;
            transform: scale(0.5);
            opacity: 0;
            transition: opacity 0.18s ease, transform 0.18s ease;
        }
        .chx-toggle-input {
            &[checked] + .chx-checkbox-box {
                background: var(--chx-primary);
                border-color: var(--chx-primary);
                box-shadow: 0 0 0 4px rgba(59, 130, 246, 0.12);
            }
            &[checked] + .chx-checkbox-box::after {
                opacity: 1;
            }
            &[checked] + .chx-radio-box {
                background: var(--chx-primary);
                border-color: var(--chx-primary);
                box-shadow: 0 0 0 4px rgba(59, 130, 246, 0.12);
            }
            &[checked] + .chx-radio-box::after {
                opacity: 1;
                transform: scale(1);
            }
            &[checked] + .chx-switch-track {
                background: rgba(59, 130, 246, 0.16);
                border-color: rgba(59, 130, 246, 0.32);
                box-shadow: 0 0 0 4px rgba(59, 130, 246, 0.10);
            }
            &[checked] + .chx-switch-track::after {
                transform: translateX(20px);
            }
        }
        .chx-switch-track {
            width: 44px;
            height: 24px;
            border-radius: 999px;
            background: var(--chx-surface-2);
            border: 1px solid var(--chx-border);
            position: relative;
            display: inline-block;
            transition: background 0.18s ease, border-color 0.18s ease, box-shadow 0.18s ease;
            &::after {
                content: '';
                width: 18px;
                height: 18px;
                border-radius: 50%;
                background: var(--chx-primary);
                position: absolute;
                left: 2px;
                top: 2px;
                transition: transform 0.18s ease, background 0.18s ease;
                box-shadow: 0 3px 8px rgba(15, 23, 42, 0.25);
            }
        }
    }
}

public #universal Checkbox(props) {
    return <label class={${toggle_styles(page)}}>
        <input type="checkbox" checked={props.checked} onClick={props.onClick} class="chx-toggle-input" />
        <span class="chx-checkbox-box"></span>
        <span>{props.children}</span>
    </label>
}

public #universal Radio(props) {
    return <label class={${toggle_styles(page)}}>
        <input type="radio" checked={props.checked} name={props.name} onClick={props.onClick} class="chx-toggle-input" />
        <span class="chx-radio-box"></span>
        <span>{props.children}</span>
    </label>
}

public #universal Switch(props) {
    return <label class={${toggle_styles(page)}}>
        <input type="checkbox" checked={props.checked} onClick={props.onClick} class="chx-toggle-input" />
        <span class="chx-switch-track"></span>
        <span>{props.children}</span>
    </label>
}
