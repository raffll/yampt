# Design — Compose yEditor Filters

## Context (current mechanics)

- `plugin_workspace_view_t` (yampt.editor/source/view/plugin_workspace_view.hpp/.cpp) owns all filter orchestration. There is no filter controller; the view is where filter state lives.
- The navigation model filter is `nav_tree_filter_t` (yampt.editor/source/model/nav_tree_filter.hpp/.cpp). Its `filter_state_t` holds every dimension at once: `filter_conflict_all` + `conflict_all_set`, `filter_conflict_this` + `conflict_this_set`, `filter_by_type` + `type_set`, `filter_by_id` + `id_text`, `filter_by_name` + `name_text`, `search_case_sensitive`, `search_regex`, `filter_deleted`, `filter_lua_severity` + `lua_severity_set`, `filter_lua_interface` + `lua_interface_set`. `operator==` is defaulted.
- `nav_tree_filter_t::passes()` evaluates every active dimension as a logical AND, and `set_filter(state)` does `m_filter = state` (wholesale replace). `clear()` sets `m_has_filter = false` and resets `m_filter`. So the model already composes; it just receives whatever full state the view hands it.
- `nav_tree_model_t::filter_state_t` is an alias of `nav_tree_filter_t::filter_state_t`.

### The three entry points (current behavior)

- `set_conflicts_only(bool)` → `on_filter_changed()`: builds a FRESH empty `filter_state_t`, sets conflict-all to `{conflict, override_benign}` only if `m_conflicts_only`, and pushes it — ignoring advanced and search. Wipes them.
- `on_advanced_filter()`: opens `filter_dialog_t`, reads its advanced fields into a fresh `nav_state`, manually re-copies the search fields (`filter_by_id/id_text/filter_by_name/name_text`) from `m_last_filter_state`, but does NOT add Conflicts Only. Drops the toggle.
- `apply_search(query, in_id, in_name, case, regex)`: mutates the search fields on `m_last_filter_state`, preserving advanced fields, and pushes it — but also without Conflicts Only.

### The duplicated view state (current)

`m_last_filter_state`, `m_last_quick_filter`, `m_filter_active`, `m_has_filter_active` all track "the current filter" and are written by different methods, so they drift. `reset_all_filters()` (recently added) already zeroes all of them and calls `clear_filter()`.

## Design Goals

Make Conflicts Only, advanced, and search compose as a single AND (R1); treat Conflicts Only as a preset for the conflict-all dimension (R2); collapse the duplicated state into one input per control derived through one build-and-apply path (R3); keep Reset clearing everything (R4); pre-populate the advanced dialog from the current advanced input (R5). Honor architecture rules (≤2 args, `_t` suffix, snake_case, `tr()`, early returns, one class per file, Anti-Gravity for the window).

## Decision: three inputs + one effective-filter builder

Replace the drifting state with three independent inputs, each owning only its own dimensions:

1. `bool m_conflicts_only` — unchanged; the shortcut toggle.
2. `nav_tree_model_t::filter_state_t m_advanced_filter` — only advanced dimensions: conflict-all, conflict-this, type, deleted, Lua. Never carries search fields.
3. `nav_tree_model_t::filter_state_t m_search_filter` — only the search dimensions: `filter_by_id/id_text/filter_by_name/name_text/search_case_sensitive/search_regex`. Never carries advanced fields.

A single private builder composes them into the effective state, and a single applier pushes it:

```cpp
nav_tree_model_t::filter_state_t plugin_workspace_view_t::build_effective_filter() const;
void plugin_workspace_view_t::apply_effective_filter();
```

`build_effective_filter()` starts from `m_advanced_filter`, overlays the search dimensions from `m_search_filter`, then applies the Conflicts Only shortcut to the conflict-all dimension:

- If `m_conflicts_only`: set `filter_conflict_all = true` and `conflict_all_set = {conflict, override_benign}`, overriding whatever `m_advanced_filter` had for conflict-all.
- Else: keep `m_advanced_filter`'s conflict-all as-is.

All other dimensions come straight from their owning input, so `passes()` ANDs them together — the composition (R1) and the shortcut semantics (R2) fall out naturally.

`apply_effective_filter()`:

```cpp
const auto state = build_effective_filter();
if (state == nav_tree_model_t::filter_state_t{})
    m_nav_view->clear_filter();
else
    m_nav_view->set_filter(state);
update_status();
```

The default-constructed comparison (via the defaulted `operator==`) decides empty-vs-active (R3.4), replacing the ad-hoc `m_has_filter_active` bookkeeping.

### Why not merge conflict-all sets

Conflicts Only and the advanced dialog both drive the same `conflict_all` field, which `passes()` treats as "entry.conflict_all must be IN the set." Two independent constraints on one field cannot both hold without a merge rule. The user's decision is that Conflicts Only is a shortcut: when on, it DEFINES conflict-all as `{conflict, override_benign}`. So the builder overrides rather than intersects or unions — no empty-set wipeout, predictable behavior (R2.1).

## Component Changes

### 1. plugin_workspace_view_t (view/plugin_workspace_view.hpp/.cpp)

Header:
- Remove members: `m_filter_active`, `m_last_filter_state`, `m_has_filter_active`, `m_last_quick_filter`.
- Add members: `nav_tree_model_t::filter_state_t m_advanced_filter;` and `nav_tree_model_t::filter_state_t m_search_filter;` (keep `m_conflicts_only`).
- Add private methods: `nav_tree_model_t::filter_state_t build_effective_filter() const;` and `void apply_effective_filter();`.

`.cpp`:
- `on_filter_changed()` — reduce to: `apply_effective_filter();` (it no longer builds state itself; `set_conflicts_only` still calls it). The conflicts-only branch logic moves into `build_effective_filter`.
- `on_advanced_filter()` — pre-populate the dialog from `m_advanced_filter` (R5.1), and on accept store ONLY the advanced dimensions into `m_advanced_filter`, then `apply_effective_filter()`. Do not touch search fields; do not read/write Conflicts Only here (R2.4, R5.2).
- `apply_search(...)` — write the six search fields into `m_search_filter` (mirroring today's computation of `filter_by_id = in_id && !query.empty()`, etc.), then `apply_effective_filter()`.
- `reset_all_filters()` — set `m_conflicts_only = false; m_advanced_filter = {}; m_search_filter = {};` then `apply_effective_filter()` (R4). Keep leaving `m_hide_duplicates` untouched.

The advanced dialog's `filter_state_t` is its own nested subset type; `on_advanced_filter` continues to translate between `filter_dialog_t::filter_state_t` and `nav_tree_model_t::filter_state_t`, but the target/source is now `m_advanced_filter` (advanced dimensions only), not the combined `m_last_filter_state`.

### 2. nav_tree_filter_t / nav_tree_model_t

No changes. `passes()` already ANDs; `set_filter`/`clear` already behave as needed. The defaulted `operator==` on `filter_state_t` is used for the empty check.

### 3. editor_window_t

No changes. The toolbar already routes: Conflicts Only QAction → `set_conflicts_only`; Advanced button → `on_advanced_filter`; search apply → `apply_search`; Reset → `reset_all_filters`. Each now composes internally. The window remains a thin router (Anti-Gravity honored).

## Data Flow

- Toggle Conflicts Only → `set_conflicts_only` sets `m_conflicts_only` → `on_filter_changed` → `apply_effective_filter` → builder overlays shortcut on conflict-all → `set_filter`/`clear_filter`.
- Apply Advanced → `on_advanced_filter` stores advanced dims in `m_advanced_filter` → `apply_effective_filter`.
- Apply Search → `apply_search` stores search dims in `m_search_filter` → `apply_effective_filter`.
- Reset → clears the three inputs → `apply_effective_filter` → empty state → `clear_filter`.

At all times the applied state = advanced ∧ search ∧ (conflicts-only preset on conflict-all).

## Error Handling

- Empty effective filter → `clear_filter()` (show all), decided by comparison against a default-constructed state.
- Regex errors in search remain handled inside `nav_tree_filter_t::matches_text` (returns false on `regex_error`); unchanged.
- No new failure modes: the change is state bookkeeping in the view, not new I/O or parsing.

## Testing Strategy

The composition and shortcut logic must be testable without UI. `build_effective_filter()` is pure (inputs → output state), but it is a private method on a Qt view. To satisfy the unit-test rules (pure `[u]`, no UI/disk), extract the merge into a free function or a small pure helper that the view calls:

```cpp
// pure, unit-testable
nav_tree_model_t::filter_state_t compose_filter(
    bool conflicts_only,
    const nav_tree_model_t::filter_state_t & advanced,
    const nav_tree_model_t::filter_state_t & search);
```

`build_effective_filter()` becomes a one-line call to `compose_filter(m_conflicts_only, m_advanced_filter, m_search_filter)`.

Pure `[u]` tests on `compose_filter`:
- conflicts_only on + empty advanced/search → conflict-all = `{conflict, override_benign}`, nothing else set.
- conflicts_only off + advanced conflict-all `{conflict}` → conflict-all = `{conflict}` (advanced preserved).
- conflicts_only on + advanced conflict-all `{conflict}` → conflict-all = `{conflict, override_benign}` (shortcut overrides that dimension), advanced conflict-this/type/deleted preserved.
- advanced + search both set → result carries both advanced dims and search dims (verify each field).
- all empty → result equals default-constructed state (drives clear).

The `passes()` AND behavior is already covered by the model; UI wiring (toolbar, dialog round-trip) is verified manually per the no-build rule.

## Files Touched

| File | Change |
|------|--------|
| `yampt.editor/source/view/plugin_workspace_view.hpp` | swap drifting members for `m_advanced_filter` + `m_search_filter`; declare `build_effective_filter`/`apply_effective_filter` |
| `yampt.editor/source/view/plugin_workspace_view.cpp` | rewrite `on_filter_changed`, `on_advanced_filter`, `apply_search`, `reset_all_filters` to compose via one path |
| `yampt.editor/source/model/nav_tree_filter.hpp/.cpp` (or a new `filter_composer.hpp/.cpp`) | add pure `compose_filter(...)` |
| `yampt.tests/source/tests.filter_composer.cpp` (new) | `[u]` tests for `compose_filter` |
| `yampt.tests/yampt.tests.vcxproj` + `.vcxproj.filters` | register the new test file (and composer .cpp if placed in a new file) |

## Documentation

- CHANGELOG `[CHANGE]` (yEditor): Conflicts Only, Advanced Filters, and search now combine instead of replacing each other; Conflicts Only acts as a shortcut for showing conflicts.
- `docs/yEditor-Manual.md`: describe that the three filter controls stack, and that Conflicts Only is a quick preset composed with the rest.
- README / README.bbcode: keep in sync if they describe yEditor filtering; add that filters combine.
