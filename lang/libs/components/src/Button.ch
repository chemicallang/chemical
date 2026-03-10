func button_styles(page : &mut HtmlPage) : *char {
    return #css {
        background : blue;
        color : white;
        padding : 8px;
        border : 0;
    }
}

public #universal Button(props) {
    return <button {...props}></button>
}
