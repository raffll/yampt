# Diagnostic Decisions

## Do NOT Add Per-Type Summary Logs to dict_creator

The `dict_creator_t` class (single, base, base_ordered modes) does NOT need per-type `print_log_line` calls after each `make_dict_*` function. The esm_converter has them because it converts an existing plugin and the totals are useful for verifying coverage. The dict_creator's output is the dictionary file itself — the entry counts are visible in the JSON output and in the `dict_writer` log line.

Do not add `print_log_line`, per-type counters, or summary messages to dict_creator_single.cpp, dict_creator_base.cpp, or dict_creator_base_ordered.cpp.

## Do NOT Log Individual Rejected Entries in dict_merger

The `dict_merger_t::merge_dict()` function only logs totals (merged, rejected, identical). Individual rejected keys should NOT be logged — even in silent mode. The rejection count is sufficient. Logging each key would produce thousands of lines for large merges and provides no actionable information (the first-wins semantics are by design).
