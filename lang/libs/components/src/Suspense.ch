// Suspense boundary for async / loading data.
//
// Renders `fallback` (a text/string label) while `loading` is truthy, otherwise
// `children`. Pair it with a `state loading` + `useEffect` load in the parent:
// on the server the initial (loading) state renders the fallback, and on the
// client the boundary swaps to `children` when the loading signal flips.
//
//   state loading = true
//   state data = ""
//   useEffect(() => {
//       props.load().then((v) => { data = v; loading = false })
//   }, [])
//   return <Suspense loading={loading} fallback="Loading…">
//       <p>{data}</p>
//   </Suspense>
//
// Props:
//   loading   bool (usually a state signal) — show fallback while truthy
//   fallback  text shown while loading (a JSX fallback prop is client-only,
//             since JSX-as-prop is not SSR-serialized)
//   children  content shown once loaded

public #universal Suspense(props) {
    return <div data-suspense="" data-loading={props.loading ? "true" : "false"}>
        {props.loading ? <span class="chx-suspense-fallback">{props.fallback}</span> : props.children}
    </div>
}
