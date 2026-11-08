// Default frame allocation hooks used by compiler-generated async ramps
// (design Section 12.3). They are declared `@extern` in `core::async`; these are
// the concrete definitions. An executor may replace them later (task arena)
// without changing generated code, because ramps call through these symbols.
public namespace async {

}
