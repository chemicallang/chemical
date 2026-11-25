// The allowed `document` surface works after SSR + hydration.
//
// The build itself is half the test: `DocumentFixture` reads `document.title`
// and `document.activeElement`, so a WEB-BODY-002 false positive for either
// member stops `--universal` before a WebView starts. The script half asserts
// the values arrived through the real runtime.

#universal_test("allowed document surface works after hydration") {
    <DocumentFixture />
    <script>
        await t.sleep(20)
        const f = byTestId('document-fixture')
        expect(f.text().length > 0).toBe(true)
        expect(f).toHaveAttribute('data-active', 'yes')
    </script>
}
