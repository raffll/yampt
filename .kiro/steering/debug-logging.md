# Debug Logging

Do NOT remove `[debug]` diagnostic logs before committing. Keep them in the code, gated behind the global debug flag so they are silent by default and can be turned on when needed.

This overrides any earlier rule that said `[debug]` messages must be removed before commit. Diagnostics are valuable to keep — the fix is to gate them, not delete them.

## How to gate a debug log

`app_logger_t::add_log` already supports this via its `silent` parameter and the global `m_debug_flag`:

- `add_log(entry, /*silent=*/true)` — the entry is emitted ONLY when the debug flag is on (`app_logger_t::set_debug(true)`), and even then it goes to the in-memory log, not stdout.
- The global switch is `app_logger_t::set_debug(bool)` / `app_logger_t::is_debug()`.

So every `[debug]` line MUST be written as a debug-gated call:

```cpp
// Good — kept in the code, silent unless debug is enabled
app_logger_t::add_log("[debug] copy_field: patch_field failed for " + sub_type + "\r\n", true);
```

```cpp
// Bad — always prints, noise in normal runs
app_logger_t::add_log("[debug] copy_field: patch_field failed\r\n");
```

```cpp
// Bad — deleting the diagnostic before commit (the old rule; no longer allowed)
// (line removed)
```

## Callers without app_logger

Some classes log through a `m_log` callback (e.g. `merge_controller_t`) rather than `app_logger_t` directly. Those callbacks always emit. When adding a `[debug]` line in such a class, route it through `app_logger_t::add_log(..., true)` (which the debug flag gates) rather than the always-on `m_log` callback, OR gate the `m_log` call behind `if (app_logger_t::is_debug())`. Either keeps the diagnostic in the code but off by default.

## Rules

- Prefix diagnostic messages with `[debug]` (lowercase), per the log-prefix convention.
- Gate every `[debug]` line behind the debug flag (`silent=true` on `app_logger_t::add_log`, or an `is_debug()` guard for callback loggers). Never leave an always-on `[debug]` line.
- Never delete a `[debug]` diagnostic to "clean up" before commit. Keep it, gated.
- Debug lines are developer-facing and stay in English (they are not user-visible; the always-update-docs and localization rules do not apply to them).
- The debug flag defaults to off, so gated `[debug]` lines produce no output in normal use and add no noise to the CLI or the GUI log.
