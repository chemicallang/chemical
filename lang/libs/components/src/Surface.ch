func paper_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: linear-gradient(180deg, rgba(255, 255, 255, 0.03), transparent 55%), hsl(var(--background));
        border: 1px solid hsl(var(--border));
        border-radius: var(--radius);
        box-shadow: var(--shadow);
        padding: 1.25rem;
    }
}

func appbar_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 1rem;
        padding: 0.9rem 1.15rem;
        border-radius: 18px;
        border: 1px solid hsl(var(--border));
        background: rgba(15, 23, 42, 0.04);
        box-shadow: var(--shadow-sm);
    }
}

func drawer_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 280px;
        max-width: 100%;
        display: grid;
        gap: 0.85rem;
        padding: 1.1rem;
        border: 1px solid hsl(var(--border));
        border-radius: var(--radius);
        background: hsl(var(--background));
        box-shadow: var(--shadow);
    }
}

func menu_styles(page : &mut HtmlPage) : *char {
    // Portaled into document.body and positioned fixed by $__uni_floating;
    // visibility is driven by data-open (inline style would wipe positioning).
    return #css {
        min-width: 220px;
        position: fixed;
        top: 0;
        left: 0;
        z-index: 20;
        display: none;
        gap: 0.35rem;
        padding: 0.55rem;
        border: 1px solid hsl(var(--border));
        border-radius: 14px;
        background: hsl(var(--background));
        box-shadow: var(--shadow);
        &[data-open="true"] {
            display: grid;
        }
    }
}

func menu_item_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 0.75rem;
        padding: 0.7rem 0.85rem;
        border-radius: 10px;
        color: hsl(var(--foreground));
        text-decoration: none;
        transition: background 0.18s ease, color 0.18s ease;
        &:hover {
            background: hsl(var(--muted));
        }
    }
}

func popover_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 280px;
        max-width: 100%;
        display: grid;
        gap: 0.55rem;
        padding: 0.9rem 1rem;
        border: 1px solid hsl(var(--border));
        border-radius: 14px;
        background: hsl(var(--background));
        box-shadow: var(--shadow);
    }
}

func dialog_overlay_styles(page : &mut HtmlPage) : *char {
    return #css {
        position: fixed;
        inset: 0;
        display: flex;
        align-items: center;
        justify-content: center;
        padding: 1.5rem;
        z-index: 50;
    }
}

func dialog_backdrop_styles(page : &mut HtmlPage) : *char {
    return #css {
        position: absolute;
        inset: 0;
        background: rgba(2, 6, 23, 0.62);
        backdrop-filter: blur(6px);
    }
}

func dialog_content_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 420px;
        max-width: 100%;
        position: relative;
        z-index: 1;
        border: 1px solid hsl(var(--border));
        border-radius: 20px;
        padding: 1.15rem;
        background: hsl(var(--background));
        box-shadow: var(--shadow-lg);
        color: hsl(var(--foreground));
    }
}

func dialog_header_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 1rem;
        margin-bottom: 0.85rem;
    }
}

func dialog_actions_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        justify-content: flex-end;
        gap: 0.75rem;
        margin-top: 1rem;
    }
}

func snackbar_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        gap: 0.8rem;
        padding: 0.85rem 1rem;
        border-radius: 999px;
        border: 1px solid hsl(var(--border));
        background: hsl(var(--background));
        color: hsl(var(--foreground));
        box-shadow: var(--shadow);
    }
}

func tooltip_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        padding: 0.45rem 0.65rem;
        border-radius: 10px;
        border: 1px solid hsl(var(--border));
        background: hsl(var(--muted));
        color: hsl(var(--foreground));
        font-size: 0.82rem;
        box-shadow: var(--shadow-sm);
        transition: opacity 0.15s ease;
    }
}

func bottom_bar_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 100%;
        max-width: 420px;
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 0.75rem;
        padding: 0.85rem 1rem;
        border: 1px solid hsl(var(--border));
        border-radius: 999px;
        background: rgba(255, 255, 255, 0.85);
        box-shadow: var(--shadow-lg);
        backdrop-filter: blur(18px);
    }
}

func empty_state_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: grid;
        gap: 0.75rem;
        justify-items: start;
        padding: 1.3rem;
        border: 1px dashed hsl(var(--border));
        border-radius: var(--radius);
        background: linear-gradient(180deg, rgba(59, 130, 246, 0.04), transparent 60%), hsl(var(--background));
    }
}

func stat_card_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: grid;
        gap: 0.5rem;
        padding: 1.15rem;
        border: 1px solid hsl(var(--border));
        border-radius: 18px;
        background: hsl(var(--background));
        box-shadow: var(--shadow-sm);
    }
}

func icon_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        width: 2rem;
        height: 2rem;
        border-radius: 999px;
        background: hsl(var(--muted));
        border: 1px solid hsl(var(--border));
        color: hsl(var(--foreground));
        font-weight: 700;
        line-height: 1;
        font-family: ui-monospace, "SFMono-Regular", monospace;
    }
}

public #universal Paper(props) {
    return <section {...props} class={${paper_styles(page)}}>{props.children}</section>
}

public #universal AppBar(props) {
    return <header {...props} class={${appbar_styles(page)}}>{props.children}</header>
}

public #universal Drawer(props) {
    return <aside {...props} class={${drawer_styles(page)}}>{props.children}</aside>
}

public #universal Menu(props) {
    return <div {...props} role="menu" class={${menu_styles(page)}}>{props.children}</div>
}

public #universal MenuItem(props) {
    return <a {...props} role="menuitem" class={${menu_item_styles(page)}}>{props.children}</a>
}

public #universal Popover(props) {
    return <div {...props} class={${popover_styles(page)}}>{props.children}</div>
}

// Modal dialog in two modes (shadcn-style `open`/`onOpenChange`):
// - Controlled: pass `open={...}` + `onClose`; visibility tracks the prop
//   reactively, so parent state drives show/hide.
// - Uncontrolled: `defaultOpen` sets the initial (and SSR) state; backdrop
//   click and the Escape key dismiss via internal state.
// Both modes fire `props.onClose` on dismissal.
public #universal Dialog(props) {
    state open = props.defaultOpen ? true : false
    var isOpen = props.open != null ? props.open : open
    const contentRef = useRef(null)
    var close = () => {
        open = false
        if(props.onClose) {
            props.onClose()
        }
    }
    useEffect(() => {
        const handler = (e) => {
            if(e.key === "Escape") {
                close()
            }
        }
        document.addEventListener("keydown", handler)
        return () => document.removeEventListener("keydown", handler)
    }, [])
    // Focus management (WAI-ARIA dialog pattern): on open, remember the
    // trigger, move focus to the first focusable element inside the dialog and
    // trap Tab/Shift+Tab within it. On close, restore focus to the trigger.
    useEffect(() => {
        if(!isOpen) {
            return () => {}
        }
        const dialogEl = contentRef.current
        if(!dialogEl) {
            return () => {}
        }
        const previouslyFocused = document.activeElement
        const focusables = () => dialogEl.querySelectorAll('button:not([disabled]), a[href], input:not([disabled]), select:not([disabled]), textarea:not([disabled]), [tabindex]:not([tabindex="-1"])')
        const firstEl = () => {
            const list = focusables()
            return list.length > 0 ? list[0] : null
        }
        const lastEl = () => {
            const list = focusables()
            return list.length > 0 ? list[list.length - 1] : null
        }
        const focusFirst = () => {
            const el = firstEl()
            if(el) { el.focus() }
        }
        const onKeyDown = (e) => {
            if(e.key !== "Tab") {
                return
            }
            const list = focusables()
            if(list.length === 0) {
                e.preventDefault()
                return
            }
            const active = document.activeElement
            if(e.shiftKey) {
                if(active === firstEl() || !dialogEl.contains(active)) {
                    e.preventDefault()
                    const last = lastEl()
                    if(last) { last.focus() }
                }
            } else {
                if(active === lastEl() || !dialogEl.contains(active)) {
                    e.preventDefault()
                    const first = firstEl()
                    if(first) { first.focus() }
                }
            }
        }
        document.addEventListener("keydown", onKeyDown)
        focusFirst()
        return () => {
            document.removeEventListener("keydown", onKeyDown)
            if(previouslyFocused && previouslyFocused.focus) {
                previouslyFocused.focus()
            }
        }
    }, [isOpen])
    // WAI-ARIA modal pattern: while the dialog is open the background becomes
    // inert (not focusable/clickable for AT and keyboard users). The modal flag
    // marks this portal as a modal overlay for the inert manager; the effect
    // re-scans on open/close (the overlay hides via display:none when closed).
    useEffect(() => {
        window.$__uni_inert_scan();
        return () => window.$__uni_inert_scan();
    }, [isOpen])
    // The overlay renders into document.body via createPortal so position: fixed
    // is viewport-relative even inside transform/overflow ancestors (shadcn does
    // the same). SSR renders it inline; hydration moves it to body.
    return createPortal(
        <div {...props} class={${dialog_overlay_styles(page)}} style={isOpen ? "" : "display:none;"}>
            <DialogBackdrop onClick={close}></DialogBackdrop>
            <DialogContent ref={contentRef} role="dialog" aria-modal="true" aria-label={props.ariaLabel}>{props.children}</DialogContent>
        </div>,
        { modal: true }
    )
}

public #universal DialogBackdrop(props) {
    return <div {...props} class={${dialog_backdrop_styles(page)}}></div>
}

public #universal DialogContent(props) {
    return <div {...props} class={${dialog_content_styles(page)}}>{props.children}</div>
}

public #universal DialogHeader(props) {
    return <div {...props} class={${dialog_header_styles(page)}}>{props.children}</div>
}

public #universal DialogActions(props) {
    return <div {...props} class={${dialog_actions_styles(page)}}>{props.children}</div>
}

public #universal Snackbar(props) {
    return <div {...props} role="status" class={${snackbar_styles(page)}}>{props.children}</div>
}

// Interactive tooltip in two modes:
// - `label` (required for the overlay mode) + optional `position`
//   (top/bottom/left/right, default top): shows a hover/focus bubble over the
//   children (the trigger). Visibility is stateful and SSR matches the hidden
//   initial render.
// - Without `label`: the plain styled span (children rendered inline).
public #universal Tooltip(props) {
    state visible = false
    if(props.label) {
        var posStyle = props.position == "bottom" ? "top:100%;left:50%;transform:translateX(-50%);margin-top:0.5rem;" : props.position == "left" ? "right:100%;top:50%;transform:translateY(-50%);margin-right:0.5rem;" : props.position == "right" ? "left:100%;top:50%;transform:translateY(-50%);margin-left:0.5rem;" : "bottom:100%;left:50%;transform:translateX(-50%);margin-bottom:0.5rem;"
        return <span style="position:relative;display:inline-flex;">
            <span onMouseEnter={() => visible = true} onMouseLeave={() => visible = false} onFocus={() => visible = true} onBlur={() => visible = false}>{props.children}</span>
            <span class={${tooltip_styles(page)}} role="tooltip" style={"position:absolute;z-index:20;white-space:nowrap;pointer-events:none;" + (visible ? "opacity:1;" : "opacity:0;") + posStyle}>{props.label}</span>
        </span>
    }
    return <span {...props} class={${tooltip_styles(page)}}>{props.children}</span>
}

public #universal Icon(props) {
    return <span {...props} class={${icon_styles(page)}}>{props.children}</span>
}

public #universal BottomBar(props) {
    return <nav {...props} class={${bottom_bar_styles(page)}}>{props.children}</nav>
}

public #universal EmptyState(props) {
    return <section {...props} class={${empty_state_styles(page)}}>{props.children}</section>
}

public #universal StatCard(props) {
    return <section {...props} class={${stat_card_styles(page)}}>{props.children}</section>
}

// Dropdown menu in two modes (shadcn-style `open`/`onOpenChange`):
// - Controlled: pass `open={...}` + `onClose`; visibility tracks the prop
//   reactively (parent state drives open/close).
// - Uncontrolled: `defaultOpen` sets the initial (and SSR) state; the
//   trigger toggles it.
// A fixed overlay behind the menu closes it on outside click; the Escape key
// dismisses too. `props.trigger` may be text or JSX; children are items.
// `props.menuStyle`/`props.itemClassName` allow per-use styling tweaks.
public #universal Dropdown(props) {
    state open = props.defaultOpen ? true : false
    var isOpen = props.open != null ? props.open : open
    const triggerRef = useRef(null)
    const menuRef = useRef(null)
    var close = () => {
        open = false
        if(props.onClose) {
            props.onClose()
        }
    }
    var toggle = () => {
        if(props.open != null) {
            if(props.onToggle) {
                props.onToggle()
            }
        } else {
            open = !open
        }
    }
    useEffect(() => {
        const handler = (e) => {
            if(e.key === "Escape") {
                close()
            }
        }
        document.addEventListener("keydown", handler)
        return () => document.removeEventListener("keydown", handler)
    }, [])
    // Portaled into document.body and anchored under the trigger by
    // $__uni_floating, so the menu escapes overflow/transform clipping.
    useEffect(() => {
        if(!isOpen || !triggerRef.current || !menuRef.current) {
            return () => {}
        }
        return window.$__uni_floating(triggerRef.current, menuRef.current, { gap: 8 })
    }, [isOpen])
    var menuStyle = ""
    if(props.menuStyle) {
        menuStyle = props.menuStyle
    }
    return <div style="position:relative;display:inline-block;">
        <div onClick={close} style={isOpen ? "position:fixed;inset:0;z-index:5;" : "display:none;"}></div>
        <Button ref={triggerRef} onClick={toggle} aria-haspopup="menu" aria-expanded={isOpen ? "true" : "false"}>{props.trigger}</Button>
        {createPortal(
            <Menu ref={menuRef} data-open={isOpen ? "true" : "false"} style={menuStyle}>
                {props.children}
            </Menu>
        )}
    </div>
}

public #universal DropdownItem(props) {
    return <MenuItem {...props}>{props.children}</MenuItem>
}
