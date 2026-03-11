func button_styles(page : &mut HtmlPage) : *char {
    return #css {
        background : var(--chx-primary);
        color : var(--chx-primary-fg);
        padding : 8px;
        border : 0;
        border-radius : var(--chx-radius);
    }
}

public #universal Button(props) {
    return <button {...props} class={${button_styles(page)}}>{props.children}</button>
}