// Shadcn-style Button with variant/size props.
//
// The single `button_styles` block emits one hashed class that contains the
// base look plus every variant/size as `[data-variant=...]` / `[data-size=...]`
// attribute selectors. The component renders `data-variant={props.variant}` and
// `data-size={props.size}`, so the same CSS class drives every combination and
// both SSR HTML and the client bundle stay in sync.
func button_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        gap: 0.5rem;
        white-space: nowrap;
        border-radius: calc(var(--radius) - 2px);
        height: 2.5rem;
        padding: 0 1rem;
        font-size: 0.875rem;
        font-weight: 500;
        line-height: 1;
        background-color: hsl(var(--primary));
        color: hsl(var(--primary-foreground));
        border: 1px solid transparent;
        box-shadow: var(--shadow-sm);
        cursor: pointer;
        user-select: none;
        -webkit-user-select: none;
        transition: background-color 0.15s ease, color 0.15s ease, border-color 0.15s ease, box-shadow 0.15s ease, opacity 0.15s ease;
        &[data-variant="default"] {
            background-color: hsl(var(--primary));
            color: hsl(var(--primary-foreground));
            &:hover { background-color: hsl(var(--primary) / 0.9); }
        }
        &:hover {
            background-color: hsl(var(--primary) / 0.9);
        }
        &:focus-visible {
            outline: 2px solid hsl(var(--ring));
            outline-offset: 2px;
        }
        &:active {
            transform: translateY(0.5px);
        }
        &:disabled {
            opacity: 0.5;
            pointer-events: none;
        }

        &[data-variant="destructive"] {
            background-color: hsl(var(--destructive));
            color: hsl(var(--destructive-foreground));
            &:hover { background-color: hsl(var(--destructive) / 0.9); }
        }
        &[data-variant="outline"] {
            background-color: transparent;
            border-color: hsl(var(--border));
            color: hsl(var(--foreground));
            &:hover { background-color: hsl(var(--accent)); color: hsl(var(--accent-foreground)); }
        }
        &[data-variant="secondary"] {
            background-color: hsl(var(--secondary));
            color: hsl(var(--secondary-foreground));
            &:hover { background-color: hsl(var(--secondary) / 0.8); }
        }
        &[data-variant="ghost"] {
            background-color: transparent;
            color: hsl(var(--foreground));
            &:hover { background-color: hsl(var(--accent)); color: hsl(var(--accent-foreground)); }
        }
        &[data-variant="link"] {
            background-color: transparent;
            color: hsl(var(--primary));
            text-decoration: underline;
            text-underline-offset: 4px;
            &:hover { background-color: transparent; text-decoration: underline; }
        }
        &[data-variant="success"] {
            background-color: hsl(var(--success));
            color: hsl(var(--success-foreground));
            &:hover { background-color: hsl(var(--success) / 0.9); }
        }
        &[data-variant="warning"] {
            background-color: hsl(var(--warning));
            color: hsl(var(--warning-foreground));
            &:hover { background-color: hsl(var(--warning) / 0.9); }
        }
        &[data-variant="info"] {
            background-color: hsl(var(--info));
            color: hsl(var(--info-foreground));
            &:hover { background-color: hsl(var(--info) / 0.9); }
        }
        &[data-variant="accent"] {
            background-color: hsl(var(--accent));
            color: hsl(var(--accent-foreground));
            &:hover { background-color: hsl(var(--accent) / 0.8); }
        }

        &[data-size="sm"] {
            height: 2.25rem;
            padding: 0 0.75rem;
            font-size: 0.75rem;
            border-radius: calc(var(--radius) - 4px);
        }
        &[data-size="lg"] {
            height: 2.75rem;
            padding: 0 1.5rem;
            font-size: 0.9375rem;
            border-radius: calc(var(--radius) - 2px);
        }
        &[data-size="icon"] {
            width: 2.5rem;
            padding: 0;
        }
    }
}

func fab_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        gap: 0.5rem;
        height: 3.5rem;
        padding: 0 1.5rem;
        border-radius: calc(var(--radius) + 4px);
        border: none;
        background-color: hsl(var(--primary));
        color: hsl(var(--primary-foreground));
        font-weight: 600;
        box-shadow: var(--shadow-lg);
        cursor: pointer;
        transition: transform 0.15s ease, box-shadow 0.15s ease, background-color 0.15s ease;
        &:hover {
            transform: translateY(-1px);
            box-shadow: 0 16px 30px rgb(0 0 0 / 0.18);
        }
        &:active {
            transform: translateY(0);
        }
        &:disabled {
            opacity: 0.5;
            pointer-events: none;
        }
    }
}

// The flagship Button. Every prop is optional; unset variant/size resolve to
// the shadcn "default" look. className merges with the generated style class.
// Any other prop (style, data-*, aria-*, id, name, ...) passes through via the
// spread. `loading` swaps children for a "Loading..." label, disables the
// button and sets aria-busy.
public #universal Button(props) {
    var loading = props.loading || false
    return <button
        {...props}
        class={${button_styles(page)}}
        class={props.className || props.class}
        data-variant={props.variant || "default"}
        data-size={props.size}
        type={props.type || "button"}
        disabled={props.disabled || loading}
        aria-disabled={props.disabled || loading}
        aria-busy={loading ? "true" : "false"}
        onClick={props.onClick}
    >{loading ? "Loading..." : props.children}</button>
}

// Legacy wrappers — keep every previously-public name working.
public #universal ButtonPrimary(props) {
    return <Button {...props}>{props.children}</Button>
}

public #universal ButtonGhost(props) {
    return <Button {...props} variant="ghost">{props.children}</Button>
}

public #universal ButtonOutline(props) {
    return <Button {...props} variant="outline">{props.children}</Button>
}

public #universal ButtonDanger(props) {
    return <Button {...props} variant="destructive">{props.children}</Button>
}

public #universal ButtonSuccess(props) {
    return <Button {...props} variant="success">{props.children}</Button>
}

public #universal ButtonInfo(props) {
    return <Button {...props} variant="info">{props.children}</Button>
}

public #universal ButtonAccent(props) {
    return <Button {...props} variant="accent">{props.children}</Button>
}

public #universal ButtonSm(props) {
    return <Button {...props} size="sm">{props.children}</Button>
}

public #universal ButtonLg(props) {
    return <Button {...props} size="lg">{props.children}</Button>
}

public #universal IconButton(props) {
    return <Button {...props} size="icon">{props.children}</Button>
}

public #universal Fab(props) {
    return <button
        class={${fab_styles(page)}}
        class={props.className}
        onClick={props.onClick}
        disabled={props.disabled}
        title={props.title}
        aria-label={props.ariaLabel}
    >{props.children}</button>
}
