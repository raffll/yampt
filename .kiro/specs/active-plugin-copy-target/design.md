# Design — Active Plugin as Copy Target in yEditor

The active plugin is a separate designation (`m_active_plugin_idx`) from the merged patch (`m_merge_plugin_idx`). Unlike the merged patch, which is store-backed (phantom column, content diverted to `m_merge_store`), the active plugin is a normal loaded plugin whose content lives in its own `esm_reader_t`. Copying into it reuses the deferred in-place edit path (`commit_to_source`), and saving reuses `save_plugin`. Core logic (designation, esm append) stays in `yampt.core`; the dialog, filename↔index mapping, and menu wiring stay in `yampt.editor`.

## Why a separate designation, not reuse of the merge slot

The merge idx is coupled to `m_merge_store` in three places: `rebuild_conflicts` skips it in normal ingest and re-projects the store as a phantom column; `read_record_content` and `compute_conflict` divert to the store. An active plugin that must be saved to its own file needs its content in the esm, not a store, and must coexist with the merged patch. Therefore `m_active_plugin_idx` is a pure marker — no store, no ingest-skip, no read/compute special-case. The plugin already appears as a real column via the normal ingest loop; the marker only says "copies land here."

## Component 1 — Active designation in plugin_scan_t (yampt.core)

`plugin_scan.hpp`:

```cpp
int m_active_plugin_idx = -1;   // alongside m_merge_plugin_idx
```

```cpp
void set_active_plugin(int plugin_idx);
void clear_active_plugin();
int  active_plugin_idx() const;
bool is_active_plugin(int idx) const;   // idx == m_active_plugin_idx && idx >= 0
bool has_active_plugin() const;         // m_active_plugin_idx >= 0
```

`set_active_plugin(idx)` validates the index, rejects if `idx == m_merge_plugin_idx` (R1.4 default: reject, since merge is store-backed) and sets the marker. `clear_active_plugin()` sets it to -1. No store, no ingest change — `rebuild_conflicts`, `read_record_content`, `compute_conflict` are NOT modified (the active plugin flows through the normal esm branch exactly like any loaded plugin).

Index invalidation: `m_active_plugin_idx` is a positional index into `m_plugins`. It must be cleared/remapped whenever plugins are reloaded (same lifetime concern as `m_merge_plugin_idx`). The session persists a filename (Component 5) and re-resolves the index after load, so the authoritative source across reloads is the filename in `plugin_session_t`.

## Component 2 — esm_reader_t append (yampt.core)

`record_t::id` is `const std::string`, so a `record_t` cannot be reassigned and a `std::vector<record_t>` cannot be `erase`/`insert`-shuffled freely. `replace_record` only mutates the selected record's content. To copy a record that does not yet exist in the active plugin, `esm_reader_t` needs to append.

`esm_reader.hpp`:

```cpp
void append_record(const std::string & record_id, const std::string & content);
```

```cpp
void esm_reader_t::append_record(const std::string & record_id, const std::string & content)
{
    m_records.push_back(record_t{ record_id, content, content.size(), true });
    ptr_record = nullptr;   // invalidate cached selection
    m_key = {};
    m_value = {};
}
```

`push_back` on `std::vector<record_t>` works because `record_t` is move-constructible from a braced init even with a const member (the const member is initialized, not assigned, during construction; the vector grows by moving existing elements, which requires the move constructor — a const member makes copy/move assignment deleted but move construction is still generated since it initializes the const member). If the toolchain rejects the move on reallocation (deleted move due to the const member forcing a copy that is also deleted), the fallback is to store records as `std::vector<std::unique_ptr<record_t>>` — but that is a larger change; the design first attempts the direct `push_back` and only falls back if it does not compile. (Implementation task notes this decision point explicitly.)

The plugin's `plugin_index_t` must also learn about the new record so `rebuild_conflicts` ingests it. `plugin_index_t` is built from the esm's records; after `append_record`, the scan must refresh that plugin's index (a `plugin_scan_t` helper `refresh_index(int plugin_idx)` that rebuilds `m_plugins[idx]->index` from the esm, or an incremental `index.add_entry(...)`). The design uses a full re-index of just that one plugin for simplicity and correctness; it is O(records in one plugin), acceptable on a copy action.

## Component 3 — Copy landing branch (yampt.editor)

The copy content producers in `merge_controller_t` (`copy_whole_record`, `copy_cell_record`, `copy_sub_record`, `copy_group`, `copy_field`) each compute a content string and end with:

```cpp
m_session.scan().copy_record_to_merge_raw(rec_type, record_id, content);
refresh_after_merge(rec_type, record_id);
save_merged_patch();
```

The active-plugin path replaces this tail. Rather than fork every method, introduce a single landing function and a target enum:

```cpp
enum class copy_target_t { merged_patch, active_plugin };

void merge_controller_t::land_copied_record(
    copy_target_t target, const std::string & rec_type,
    const std::string & record_id, const std::string & content);
```

- `merged_patch`: current behavior — `copy_record_to_merge_raw` + `refresh_after_merge` + `save_merged_patch`.
- `active_plugin`: commit into the active plugin's esm:
  1. `const int active = m_session.scan().active_plugin_idx();`
  2. Find the active plugin's existing version of `(rec_type, record_id)`. If present, `mutable_plugin(active).select_record(idx); replace_record(content);`. If absent, `mutable_plugin(active).append_record(record_id, content);` then `scan().refresh_index(active);`.
  3. `m_session.mark_plugin_dirty(active);`
  4. `scan().recompute_single_conflict(rec_type, record_id);`
  5. `refresh_after_merge(rec_type, record_id)` (redisplays the record) — but NOT `save_merged_patch` and NOT a disk write (deferred model, R3.4).

Each copy method takes the target (or reads it from a controller field set by the menu). The "ensure base record" step for sub-record/group/field copies (`ensure_merge_record`) gets an active-plugin analogue `ensure_active_record` that looks up / seeds the record in the active plugin's esm instead of the merge store, returning the current content to patch against.

The content-production code (source read via `read_source_content`, `sub_record_merge_t`, `merge_patch_ops_t`) is unchanged and shared by both targets.

## Component 4 — Copy-to-new-plugin fallback (yampt.editor)

When a copy action fires and `!has_active_plugin()`:

1. Prompt: `QInputDialog::getText(...)` for a filename (translated title/prompt). Append `.esp` if missing.
2. Resolve path: `resolve_output_directory()` + "/" + filename. If it exists (on disk or as a loaded plugin), refuse and re-prompt (R5.4 default).
3. Build the new plugin: reuse `patch_builder_t` — masters from the copied record's source plugin (a one-record scope of `collect_contributing_plugins`/`build_master_list`), one record (the copied content), header via `build_tes3_header`. `patch_builder_t::save(path, "yEditor", "", masters)`.
4. Load it: `scan().load_plugin(path)`, `scan().rebuild_conflicts()`.
5. Designate active: map the new filename → its loaded index, `scan().set_active_plugin(idx)`, `m_session.set_active_plugin(filename)`, persist session, rebuild nav.

Subsequent copies now hit the active-plugin path (Component 3), deferred. The new plugin was written once at creation (it must exist on disk to be loaded); later copies mark it dirty and require Save.

## Component 5 — Session persistence (yampt.editor)

`plugin_session_t`:

```cpp
std::string m_active_plugin;   // filename
const std::string & active_plugin() const;
void set_active_plugin(const std::string & filename);
```

`save_session_state`: `settings.setValue("session/active_plugin", QString::fromStdString(m_active_plugin));`
`restore_session_state`: read it back, then after plugins load, resolve filename→index and call `scan().set_active_plugin(idx)` if found (else leave cleared). Mirrors how the folder/MO2/OpenMW restore re-establishes state, and how excluded/patch sets are restored — but scalar.

The scan's `m_active_plugin_idx` is derived from the session filename on load; the session filename is authoritative across reloads.

## Component 6 — Nav-tree marker + icon consistency (yampt.editor)

Add an "active plugin" marker to the plugin-icon precedence, updated in BOTH `nav_tree_model.cpp::file_node_display_text`-area icon logic and `view_tree_model.cpp::headerData` (plugin-icons-consistent rule). Choose a glyph not already used (existing: lock/shield/gear/pen/scroll/lightning/page). Placement in the precedence list is decided in implementation (R4.2 deferred); a reasonable slot is just below the merged-patch/guard markers. The active plugin also keeps its normal dirty asterisk when it has unsaved copies.

## Component 7 — Context menu (yampt.editor)

`view_context_menu_t`:

- Nav menu (`build_source_file_menu`, alongside Exclude/Guard): add "Mark as Active Plugin" / "Unmark as Active Plugin". The handler sets/clears `m_session.set_active_plugin(filename)`, maps to the scan index (`set_active_plugin`/`clear_active_plugin`), persists session, and `rebuild_preserving_state()`. Reject marking active if the plugin is the merged patch (log message).
- Record-view menu (`show_view_menu`): the copy dispatch currently gates on `has_merge()`. Extend so that when the clicked column is a source plugin, the menu offers:
  - "Copy … to Active Plugin" when `has_active_plugin()` (routes to the active-plugin target), OR when no active plugin exists, a "Copy … to New Plugin…" action that triggers the Component 4 flow.
  - The existing "Copy … to Merged Patch" actions remain when `has_merge()` and the column is not the active plugin.
  Both sets can appear together when both a merged patch and an active plugin exist (Open Decision → default: offer both, labeled distinctly). The menu-building helpers (`build_copy_to_merge_menu` / `build_source_copy_menu`) are parameterized by `copy_target_t` (and a label string) so the same row-kind logic produces either the merged-patch or active-plugin action set without duplication.

## Files

Modified (yampt.core):
- `scanner/plugin_scan.hpp/.cpp` — add `m_active_plugin_idx` + accessors; add `refresh_index(int)`. No change to rebuild_conflicts/read_record_content/compute_conflict logic paths (active plugin uses the normal esm branch).
- `io/esm_reader.hpp/.cpp` — add `append_record(record_id, content)`.
- `scanner/plugin_index` — a way to refresh/add an entry for the appended record (or full re-index via `refresh_index`).

Modified (yampt.editor):
- `controller/merge_controller.hpp/.cpp` — `copy_target_t`, `land_copied_record`, `ensure_active_record`; parameterize copy methods by target; new-plugin creation flow.
- `controller/view_context_menu.hpp/.cpp` — Mark/Unmark Active in nav menu; target-parameterized copy actions and the no-active "Copy to New Plugin…" fallback.
- `session/plugin_session.hpp/.cpp` — `m_active_plugin` + accessors + save/restore.
- `model/nav_tree_model.cpp` + `model/view_tree_model.cpp` — active-plugin icon in both header paths (consistent).
- `view/plugin_workspace_view.cpp` — wire the active-plugin marker set into the nav model like excluded/patch/dirty sets are wired.

vcxproj/filters: no new files expected (all additions to existing files); if any new .hpp/.cpp is introduced, update both vcxproj and flat filters.

Docs: user-visible feature → `docs/yEditor-Manual.md` (new "Active Plugin" description under the context-menu/merge sections), `README.md` + `docs/README.bbcode` (mirror), `CHANGELOG.md` under the unreleased section, yEditor, `[NEW]`. No tests/scripts/build details in docs.

## Testing (pure `[u]`, no file I/O)

- `esm_reader_t::append_record`: appended record is present in `get_records()` and selectable; cached selection invalidated.
- `plugin_scan_t` active designation: `set_active_plugin`/`clear_active_plugin`/`is_active_plugin`/`has_active_plugin`; active and merge on different plugins coexist; setting active on the merge plugin is rejected.
- Landing into the active plugin (core-level, using an in-memory scan): copying a record that is absent appends it to the active plugin's esm with the copied content; copying one that exists replaces it; the active plugin is the record's version in `entries()` after `recompute_single_conflict`.
- New-plugin file creation is integration-level (loadable single-record plugin, correct header/masters), not `[u]`.

Building and running tests are done manually by the user (project no-build rule); tasks produce tests as artifacts, no "run tests" step.
