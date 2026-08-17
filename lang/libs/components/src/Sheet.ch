// Shadcn-style Sheet: a modal panel that slides in from an edge of the
// screen (like shadcn's Sheet / Dialog with side placement).
//
// Props:
//   open          controlled visibility (bool)
//   defaultOpen   uncontrolled initial visibility (SSR uses this)
//   onClose       fired when the sheet is dismissed
//   side          "left" | "right" (default) | "top" | "bottom"
//   title         optional accessible title
//   width         optional panel width for left/right sides
//   children      sheet body content
//
// Like Dialog, both controlled and uncontrolled modes are supported; the
// backdrop click and the Escape key dismiss it.

func sheet_overlay_styles(page : &mut HtmlPage) : *char {
    return #css {
        position: fixed;
        inset: 0;
        z-index: 50;
        display: flex;
    }
}

func sheet_backdrop_styles(page : &mut HtmlPage) : *char {
    return #css {
        position: absolute;
        inset: 0;
        background: rgba(2, 6, 23, 0.6);
        backdrop-filter: blur(4px);
        animation: chx-fade-in 0.2s ease;
    }
}

func sheet_content_styles(page : &mut HtmlPage) : *char {
    return #css {
        position: relative;
        z-index: 1;
        display: flex;
        flex-direction: column;
        background: hsl(var(--background));
        color: hsl(var(--foreground));
        box-shadow: var(--shadow-lg);
        overflow-y: auto;
        &[data-side="right"] {
            margin-left: auto;
            height: 100%;
            width: 400px;
            max-width: 100%;
            animation: chx-sheet-in-right 0.25s var(--ease);
        }
        &[data-side="left"] {
            margin-right: auto;
            height: 100%;
            width: 400px;
            max-width: 100%;
            animation: chx-sheet-in-left 0.25s var(--ease);
        }
        &[data-side="top"] {
            margin-bottom: auto;
            width: 100%;
            max-height: 60vh;
            animation: chx-sheet-in-top 0.25s var(--ease);
        }
        &[data-side="bottom"] {
            margin-top: auto;
            width: 100%;
            max-height: 60vh;
            animation: chx-sheet-in-bottom 0.25s var(--ease);
        }
    }
}

func sheet_header_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 1rem;
        padding: 1rem 1.25rem;
        border-bottom: 1px solid hsl(var(--border));
    }
}

func sheet_title_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 1.125rem;
        font-weight: 600;
        color: hsl(var(--foreground));
    }
}

func sheet_close_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        width: 2rem;
        height: 2rem;
        border-radius: 6px;
        border: none;
        background: transparent;
        color: hsl(var(--muted-foreground));
        font-size: 1.25rem;
        line-height: 1;
        cursor: pointer;
        transition: background 0.15s ease, color 0.15s ease;
        &:hover {
            background: hsl(var(--accent));
            color: hsl(var(--foreground));
        }
    }
}

func sheet_body_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 1.25rem;
        display: grid;
        gap: 1rem;
    }
}

public #universal Sheet(props) {
    state open = props.defaultOpen ? true : false
    var isOpen = props.open != null ? props.open : open
    var side = props.side || "right"
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
    // Focus management: on open, focus the sheet close button (first
    // focusable) and trap Tab within it; restore focus on close.
    useEffect(() => {
        if(!isOpen) {
            return () => {}
        }
        const sheetEl = contentRef.current
        if(!sheetEl) {
            return () => {}
        }
        const previouslyFocused = document.activeElement
        const focusables = () => sheetEl.querySelectorAll('button:not([disabled]), a[href], input:not([disabled]), select:not([disabled]), textarea:not([disabled]), [tabindex]:not([tabindex="-1"])')
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
                if(active === firstEl() || !sheetEl.contains(active)) {
                    e.preventDefault()
                    const last = lastEl()
                    if(last) { last.focus() }
                }
            } else {
                if(active === lastEl() || !sheetEl.contains(active)) {
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
    var width = props.width || ""
    var style = ""
    if(width && (side == "left" || side == "right")) {
        style = "width:" + width + ";"
    }
    // Renders into document.body via createPortal (shadcn pattern) so the
    // fixed-position overlay stays viewport-relative inside transform/overflow
    // ancestors. SSR renders it inline; hydration moves it to body.
    return createPortal(
        <div class={${sheet_overlay_styles(page)}} style={isOpen ? "" : "display:none;"}>
            <SheetBackdrop onClick={close}></SheetBackdrop>
            <SheetContent ref={contentRef} side={side} style={style} role="dialog" aria-modal="true" aria-label={props.title}>
                <SheetHeader>
                    {props.title ? <SheetTitle>{props.title}</SheetTitle> : <span></span>}
                    <SheetClose onClick={close} aria-label="Close">×</SheetClose>
                </SheetHeader>
                <SheetBody>{props.children}</SheetBody>
            </SheetContent>
        </div>
    )
}

public #universal SheetBackdrop(props) {
    return <div {...props} class={${sheet_backdrop_styles(page)}}></div>
}

public #universal SheetContent(props) {
    return <div {...props} class={${sheet_content_styles(page)}} data-side={props.side}>{props.children}</div>
}

public #universal SheetHeader(props) {
    return <div {...props} class={${sheet_header_styles(page)}}>{props.children}</div>
}

public #universal SheetTitle(props) {
    return <div {...props} class={${sheet_title_styles(page)}}>{props.children}</div>
}

public #universal SheetClose(props) {
    return <button {...props} type="button" class={${sheet_close_styles(page)}}>{props.children}</button>
}

public #universal SheetBody(props) {
    return <div {...props} class={${sheet_body_styles(page)}}>{props.children}</div>
}
