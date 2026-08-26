# Universal Component Testing Design

**Status:** Proposed
**Date:** August 2026

## Summary

Universal components need two different kinds of tests:

1. **Compiler and output tests**: verify generated HTML, CSS, and JavaScript.
2. **Browser behavior tests**: verify hydration, events, state updates, focus,
   portals, and accessibility in a real browser engine.

The second kind must not create a native window for every test. The proposed
default is a **single test process, one WebView, one page, and one test session**.
The page contains all test fixtures. Each test resets or replaces only its
fixture and reports its result to the native runner. Tests run sequentially in
the WebView because browser DOM state and the WebView UI thread are shared.

This is a separate component-test mode built on the existing `@test` discovery
and reporting concepts. Ordinary `@test` behavior remains unchanged.

## Findings

### Existing universal output already supports a suite page

`HtmlPage` in `lang/libs/page/src/page.ch` accumulates HTML, CSS, and JavaScript
for a complete page. The universal converter emits SSR markup and hydration code
into that page. A component therefore does not need its own document or WebView.

The existing component E2E design has the right shape: a single app page with
many fixtures, stable `data-testid` hooks, and tests that scope selectors to one
fixture. It also catches failures that string-level plugin tests cannot catch,
including duplicate hydration nodes, stale reactive props, portal behavior, and
focus handling.

### The current `@test` runner is not a direct fit

The test library discovers annotated functions through
`intrinsics::get_tests<TestFunction>()`. `lang/libs/test/src/runner.ch` normally
launches each test in a child process, often concurrently. This is useful for
crash isolation, but it would create one WebView and one window per component
test if component tests used the same path.

Component tests also interact with an event-driven API. WebView JavaScript
evaluation completes through callbacks, and `webview_run()` owns the native
message loop. A test function should not directly block the UI thread waiting
for its own JavaScript callback.

### WebView is useful, but it is an environment-specific backend

The WebView library already has the important primitives:

- `webview_create`, `webview_load_html`, `webview_show`, and `webview_run`
- `webview_evaluate_js_result` for assertions and DOM queries
- `webview_bind` for a JavaScript-to-native result channel
- `webview_dispatch` for scheduling work on the UI thread
- `webview_stop` for ending the suite

The current WebView tests also document the constraint: creation and display
require GTK/WebKit and a graphical environment. Consequently, WebView tests
should be a separate opt-in suite, not part of the normal headless compiler
test run.

For CI and developer machines without a display, the existing static browser
E2E runner remains the best backend. The component-test contract should be
shared between the WebView runner and the Playwright runner where practical.

## Proposed User Model

The first implementation should keep component tests familiar to Chemical
users while hiding the asynchronous transport.

### Suite source

A component test package contains one suite page and annotated test functions:

```chemical
@component_fixture("button-fixture")
#universal ButtonFixture(props) {
    state clicks = 0

    return <div data-testid="button-fixture">
        <Button data-testid="button" onClick={() => { clicks++ }}>
            Click me
        </Button>
        <span data-testid="count">{clicks}</span>
    </div>
}

@component_test
public func button_increments_count(env : &mut ComponentTestEnv) {
    env.fixture("button-fixture")
    env.click("[data-testid=button]")
    env.expect_text("[data-testid=count]", "1")
}

public func main(argc : int, argv : **char) : int {
    return component_test_runner(argc, argv)
}
```

The exact fixture annotation is not fixed by this document. An initial version
can use a convention such as a `component_test` root page and
`data-testid` attributes, avoiding a new annotation until the runtime protocol
works. The important user-facing behavior is:

- A fixture is rendered once in the suite page.
- `env.fixture(name)` scopes all following operations to that fixture.
- `click`, `type`, `press`, `focus`, `wait_for`, `expect_text`, `expect_attr`,
  `expect_visible`, and `expect_count` are synchronous-looking helpers.
- The helpers pump the WebView event loop through the runner protocol; users do
  not write callback plumbing for every assertion.
- A failing assertion records a message and stops the current test, then the
  runner resets the fixture before continuing.

The API should prefer accessible queries in addition to CSS selectors:

```chemical
env.get_by_role("button", "Click me").click()
env.get_by_role("dialog").expect_visible()
env.get_by_text("Selected: Banana").expect_visible()
```

CSS selectors remain necessary for component-specific hooks, but role and text
queries make accessibility regressions visible in the normal test style.

## Execution Architecture

### Parent process

The component test executable owns one `WebView` for the entire run.

1. Discover `@component_test` functions at startup.
2. Create one WebView and load the generated suite HTML.
3. Install a single bridge handler, for example `__chemical_test__`.
4. Run one test at a time.
5. Receive assertion results, logs, and completion messages.
6. Reset the fixture and start the next test.
7. Print results using the existing `TestFunctionState` reporting format.
8. Stop and destroy the WebView once, after all tests finish.

The initial implementation should run component tests sequentially. Parallel
browser contexts can be considered later, but parallel tests inside one WebView
would make state, focus, timers, portals, and failure reporting nondeterministic.

### JavaScript side

Inject a small test harness before or alongside the generated universal bundle.
It should provide:

- selector and role-based lookup
- click, keyboard, input, focus, and blur actions
- polling with a bounded timeout
- assertion serialization
- fixture cleanup/reset
- a monotonically increasing command id

The harness communicates with native code using the existing WebView bridge.
The native side sends commands by evaluating JavaScript and receives a JSON
result through `webview_evaluate_js_result`. The bridge is preferable for
completion notifications because it already supports JS-to-native calls and
works across the Linux and Windows implementations.

All messages should include a test id and command id. Late callbacks from a
previous test must be ignored after the runner advances to the next test.

### Async control without async Chemical syntax

Do not expose raw WebView callbacks in the first user API. The runner can use a
small state machine:

- native test code submits one command;
- the WebView evaluates it;
- the callback stores the result;
- the native event loop continues until the command completes or times out;
- the helper returns a pass/fail result to the test function.

The test function itself should execute on a worker thread, or the runner must
schedule each native test step around the UI loop. It must never call a blocking
wait from the GTK/WebView UI callback thread. `webview_dispatch` is useful for
the latter, but the worker-thread approach is easier to make correct initially.

## Fixture Reset

The simplest reliable reset is to reload the suite document between tests, but
that is slower and can make 300 tests unnecessarily expensive. Use two levels:

1. **Default reset:** each fixture is wrapped in a host element and replaced by
   a fresh copy of its initial HTML/state. This should be the normal path.
2. **Full reset:** reload the document when a test uses a portal, global event
   listener, timer, navigation, or explicitly requests isolation.

The first implementation may use a full page reload for correctness, then add
fixture replacement after the command protocol is stable. Even with reloads,
there is still only one native window and one WebView.

Tests must not depend on execution order. The runner should report the fixture
name and test name whenever reset fails.

## Reporting and Failure Semantics

Reuse the existing `TestFunctionState`, `TestLog`, and `TestEnv.error` concepts.
Add component-specific details to failure logs:

- test and fixture name
- operation that failed
- selector or accessible query
- expected value and observed value
- timeout
- browser console/runtime errors
- optional HTML snapshot for the fixture

The runner should fail a test on any of these conditions:

- an assertion fails;
- a command times out;
- the WebView reports a JavaScript exception;
- the universal runtime reports a component render or hydration error;
- fixture reset fails;
- the WebView process exits unexpectedly.

One test failure should not close the suite. A WebView crash is different: mark
the current test and all not-yet-run tests as unavailable, then exit non-zero.

Use existing filters where possible: `--test-names`, `--test-ids`,
`--failure-only`, and `--no-logs`. Add `--component-tests` as an explicit mode
and avoid changing the meaning of normal `test_runner` invocation.

## Fast Implementation Plan

### Phase 1: prove the protocol

Implement a dedicated `component_test` library or test package with:

1. `ComponentTestEnv` containing the current fixture name, command id, timeout,
   and failure state.
2. A single WebView suite page generated from one Chemical application.
3. A minimal JavaScript harness with `click`, `text`, and `visible` commands.
4. A single native bridge handler and JSON command/result messages.
5. Sequential execution and full page reload between tests.
6. Reuse of existing test result printing.
7. A dedicated command such as `./scripts/test.sh --tcc --components-webview`.

Start with one Button fixture, one controlled Input fixture, one Dialog or
Select fixture, and one hydration smoke test. These cover event dispatch,
state propagation, text input, portals, and SSR-to-client behavior.

### Phase 2: make it pleasant

Add role/text locators, keyboard actions, bounded polling, fixture reset, and
runtime error capture. Add stable helper methods for common assertions instead
of making users write JavaScript strings.

Add a `--headed` option only for debugging. The default should use a hidden or
minimized WebView where the platform supports it. Do not create or destroy a
window per test.

### Phase 3: share the contract with browser E2E

Keep the existing Playwright suite for headless CI and broad browser coverage.
Extract fixture conventions and test semantics so a fixture can be tested by
either backend. The WebView runner is valuable for applications that already
ship a WebView, but it is not a replacement for a full browser automation
engine.

## Alternatives Considered

### One WebView per `@test`

Rejected as the default. It provides strong process isolation, but startup cost,
window creation, GTK/WebKit resources, and display requirements scale with the
number of tests. It also makes a 300-test suite slow and difficult to run.

### One WebView per test process, with existing parallel runner

Better crash isolation but still has the same resource explosion. It can be
offered as a `--component-isolate` debugging mode later, not as the normal path.

### Run all tests as JavaScript inside the page

Fast and natural for DOM assertions, but it moves test discovery, reporting,
filtering, retries, and failure semantics out of Chemical's test infrastructure.
It is a useful implementation detail for the harness, not the primary user API.

### Use only Playwright

This is the fastest route for headless CI and already exists as a proven
components E2E suite. It does not help users whose application is specifically
embedded in Chemical's WebView, and it requires a separate TypeScript test
project. Keep it as the preferred CI backend while adding the single-WebView
runner for native/WebView integration.

## Recommendation

Implement a dedicated sequential component-test mode using one WebView and one
generated suite page. Keep ordinary `@test` process isolation unchanged. Start
with a small synchronous-looking `ComponentTestEnv` API backed by a JSON bridge,
full page reloads, and existing test reporting. Add fixture replacement and
more locators only after the end-to-end protocol works.

This gives users a simple test model, avoids hundreds of windows, preserves
failure filtering and reporting, and can be implemented without redesigning the
general Chemical test runner or the universal component compiler.
