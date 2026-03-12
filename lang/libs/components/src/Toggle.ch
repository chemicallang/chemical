func checkbox_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        gap: 0.6rem;
        color: var(--chx-text-main);
        font-weight: 600;
        cursor: pointer;
    }
}

func checkbox_box_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 18px;
        height: 18px;
        border-radius: 4px;
        border: 1px solid var(--chx-border-strong);
        background: var(--chx-surface);
        display: inline-block;
    }
}

func switch_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        gap: 0.75rem;
        cursor: pointer;
        color: var(--chx-text-main);
        font-weight: 600;
    }
}

func switch_track_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 44px;
        height: 24px;
        border-radius: 999px;
        background: var(--chx-surface-2);
        border: 1px solid var(--chx-border);
        position: relative;
        display: inline-block;
    }
}

func switch_thumb_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 18px;
        height: 18px;
        border-radius: 50%;
        background: var(--chx-primary);
        display: inline-block;
        margin-left: 3px;
        margin-top: 2px;
    }
}

public #universal Checkbox(props) {
    return <label class={${checkbox_styles(page)}}>
        <input type="checkbox" checked={props.checked} />
        <span class={${checkbox_box_styles(page)}}></span>
        <span>{props.children}</span>
    </label>
}

public #universal Switch(props) {
    return <label class={${switch_styles(page)}}>
        <input type="checkbox" checked={props.checked} />
        <span class={${switch_track_styles(page)}}>
            <span class={${switch_thumb_styles(page)}}></span>
        </span>
        <span>{props.children}</span>
    </label>
}
