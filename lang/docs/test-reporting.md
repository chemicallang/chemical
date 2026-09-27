# Streaming & Combining Test Runners

## Motivation

The language ships more than one kind of test runner:

| Runner | Library | Discovers | Historically printed |
|--------|---------|-----------|----------------------|
| `@test` | `lang/libs/test` (`test_runner`) | `intrinsics::get_tests<TestFunction>()` | coloured, grouped, `Summary: N tests - P passed, F failed` |
| `#universal_test` | `lang/libs/universal_test` (`universal_test_runner`) | `intrinsics::get_universal_tests<UTFunction>()` | uncoloured `PASS/FAIL name` lines |

Each runner used to own its whole presentation, so they looked different and
could not be combined. As new runners appear, users want to run several of them
from one executable, see a single streamed output, and get one final report:
per-runner counts **and** grand totals.

`lang/libs/test_report` provides the shared abstraction that makes this
possible.

## Architecture

```
              +----------------------+
   runner --->|  TestReporter        |<--- one shared reporter
              |  begin_runner(name)  |
              |  on_test(outcome)    |  (streamed per test)
              |  end_runner()        |
              |  finish() -> int     |  (once, grand totals)
              +----------------------+
```

* **`TestOutcome`** (`test_report`) — the trivially-copyable event describing one
  finished test: runner, name, group, id, passed/skipped, exit code, failure
  message, borrowed structured logs, duration.
* **`TestReporter`** (`test_report`) — the sink interface. A runner calls
  `begin_runner("name")`, streams one `on_test(...)` per finished test, then
  `end_runner()`. The driver calls `finish()` once; it returns non-zero when
  anything failed.
* **`ConsoleReporter`** (`test_report`) — the default implementation. It prints
  the same presentation the `test` library always had (`  - name id`, indented
  logs, `    PASS/FAIL`, `Test run summary`, `Summary: N tests - P passed, F
  failed`) and now also prints a per-runner breakdown when more than one runner
  ran.
* **`TestRunnerHandle`** — a runnable runner: a name plus a `run_fn` and the
  discovered test array. Built by each library via a *comptime* builder so
  discovery happens at the user's call site.
* **`MultiTestRunner`** — runs several handles under one shared reporter and
  prints one combined summary at the end.

Every runner still exposes its historical single-runner entry point
(`test_runner`, `universal_test_runner`) unchanged; those now simply drive a
default `ConsoleReporter` internally.

## Use cases

### 1. A single `@test` runner

```chemical
public func main(argc : int, argv : **char) : int {
    return test_runner(argc, argv)
}
```

Unchanged. Output is the familiar coloured grouped listing plus summary.

### 2. A single `#universal_test` runner

```chemical
public func main(argc : int, argv : **char) : int {
    return universal_test_runner(argc, argv)
}
```

Unchanged API; output now uses the shared coloured reporter.

### 3. Two runners, one after the other

```chemical
public func main(argc : int, argv : **char) : int {
    var multi = new_multi_test_runner()
    multi.add(test_runner_handle())
    multi.add(universal_test_runner_handle())
    return multi.run(argc, argv)
}
```

Each library provides a comptime `*_runner_handle()` builder. Because it is
comptime, `get_tests()` / `ut_all()` are evaluated at this call site, exactly
like the existing `test_runner` / `universal_test_runner` entry points.

### 4. Many runners

```chemical
public func main(argc : int, argv : **char) : int {
    var multi = new_multi_test_runner()
    multi.add(test_runner_handle())          // @test
    multi.add(universal_test_runner_handle()) // #universal_test
    multi.add(my_custom_runner_handle())      // any future runner
    return multi.run(argc, argv)
}
```

Any library can expose a handle by returning:

```chemical
public struct TestRunnerHandle {
    public var name : string_view
    public var tests_ptr : *void
    public var tests_count : size_t
    public var run_fn : (h : *mut TestRunnerHandle, reporter : &mut TestReporter, argc : int, argv : **char) => int
}
```

The runner's `run_fn` recovers its typed test array from `tests_ptr` /
`tests_count`, creates a `std::span`, and calls its own
`run_*_reporting(span, reporter, argc, argv)` which drives the reporter
lifecycle (but does **not** call `finish`).

## Output

Each finished test is delivered to the reporter via `on_test` as soon as the
runner has it. The `@test` runner's filtered path
(`--test-id`/`--test-ids`/`--test-names`) emits immediately; its parallel path
emits in declaration order once the tests complete, so output stays
deterministic (and group headers stay contiguous).

With more than one runner the console reporter emits:

```
==> runner: test
Group: (no-group)
  - some_test 1073741823
    PASS
  ...

==> runner: universal_test
Group: (no-group)
  - ssr renders initial state 0
    PASS
  ...

Test run summary
  Total: 46 | Passed: 45 | Failed: 1

  test           : 40 tests - 40 passed, 0 failed
  universal_test : 6 tests - 5 passed, 1 failed

Summary: 46 tests - 45 passed, 1 failed
```

`scripts/test.sh` keeps working: it extracts
`Summary: <N> tests - <P> passed, <F> failed` (last occurrence, so the grand
total wins) and the `  - <name>` / `    FAIL` pair for failing-test names.
When a runner produces **zero** tests the reporter stays completely silent
(no `Test run summary` block), matching the historical behaviour of the
sequential `--tcc` suite, which registers no `@test` functions at all.

## Child processes

The `test` library executes each `@test` in a spawned child (IPC). A combined
executable re-enters its own `main` in that child. `MultiTestRunner.run`
detects `--comm-id` and then runs **only** the `test` runner (silently), which
reports nothing back, matching the historical child behaviour. Standalone
`test_runner` does the same through `run_test_runner_reporting`.

## Extending

To add a reporter that writes JSON/JUnit/host events, implement `TestReporter`
and pass it to any `run_*_reporting` function, or add a `MultiTestRunner`
variant that accepts a custom reporter instead of building a
`ConsoleReporter`. No runner changes are required.
