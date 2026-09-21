// WebView component tests for universal components.
//
// Each #universal_test declares an inline fixture rendered with the production
// SSR + hydration pipeline, plus raw JS steps. All fixtures share one page and
// one WebView (tests marked `isolate` get their own).

#universal Counter(props) {
    state count = props.start || 0
    return <div>
        <button data-testid="inc" onClick={() => count += 1}>Increment</button>
        <span data-testid="count">Count: {count}</span>
    </div>
}

#universal Greeting(props) {
    return <h1 data-testid="greet">Hello {props.name}</h1>
}

#universal Toggle(props) {
    state open = false
    return <div>
        <button data-testid="toggle" onClick={() => open = !open}>Toggle</button>
        {open ? <p data-testid="panel">Open</p> : null}
    </div>
}

#universal InputBox(props) {
    state text = ""
    return <div>
        <input data-testid="input" value={text} onInput={(e) => text = e.target.value} />
        <span data-testid="mirror">{text}</span>
    </div>
}

public func main(argc : int, argv : **char) : int {
    return universal_test_runner(argc, argv)
}
