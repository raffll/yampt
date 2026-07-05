# GUI Document Architecture

## Polymorphic Document Interface

`main_window_t` holds a single `std::unique_ptr<document_t> active_doc_` pointer. All single-document operations (table build, edit commit, save, progress reporting) route through this pointer instead of branching on document type.

Two concrete implementations:
- `dict_document_t` — wraps a `dict_slot_t*` from `dict_workspace_t`, delegates save via path-based callback
- `yaml_document_t` — manages a YAML l10n file independently with tmp-file persistence

Cross-slot workspace operations (merge, convert, make-base, annotations, save-all-dirty) remain on `dict_workspace_t` unchanged.

## document_t Interface

```cpp
class document_t
{
public:
    virtual ~document_t() = default;
    virtual std::string path() const = 0;
    virtual bool is_dirty() const = 0;
    virtual bool is_read_only() const = 0;
    virtual std::vector<table_row_t> build_rows() const = 0;
    virtual void commit_edit(tools_t::rec_type_t type, size_t chapter_index,
                             const std::string & new_text) = 0;
    virtual void save() = 0;
    virtual int translated_count() const = 0;
    virtual int total_count() const = 0;
    virtual void set_dirty(bool dirty) = 0;
};
```

## dict_document_t — Thin View Over a Slot

- Stores `dict_slot_t * slot_`, `std::string slot_path_`, `save_fn_t save_fn_`, `bool read_only_`
- `path()` returns normalized `slot_path_` (NOT `slot_->path` directly)
- `is_dirty()` reads `slot_->dirty` — the document is a transparent view, not a copy
- `save()` calls `save_fn_(slot_path_)` — the callback resolves the current index via `workspace_.find_by_path()`
- Does NOT own or manage slot lifetime. When a slot is unloaded, `active_doc_` must be reset first.

## Path Normalization

Both document constructors normalize their stored path (replace `\\` with `/`) at construction time. `path()` always returns a consistent forward-slash format. Direct `==` comparison works without ad-hoc normalization at call sites.

`file_list_t` already normalizes all stored paths. `dict_slot_t::path` is also normalized in practice (comes from `file_list_` paths). The constructor normalization is a defensive guarantee.

## rebuild_table() Does NOT Use build_rows() for Dicts

For dict documents, `rebuild_table()` accesses `dict_doc->slot()->data` directly via `dynamic_cast<dict_document_t*>`. This preserves the existing counting, BNAM interleaving, sub-type classification, and filter tree update logic unchanged.

For YAML documents, `rebuild_table()` uses `active_doc_->build_rows()` to get a flat row vector, then applies status/search filtering only.

`dict_document_t::build_rows()` exists for interface symmetry and possible future use (export), but is NOT called by the table display path.

## Use-After-Free Prevention — Unload Guards

`dict_document_t` stores a raw `dict_slot_t*`. Vector erasure in `workspace_.unload_dict()` invalidates this pointer. Every code path that calls `unload_dict()` must reset `active_doc_` FIRST if the active document's slot is being unloaded.

Guarded paths:
- `on_unload_slot` — single-slot unload via sidebar
- `remove_folder_requested` — bulk unload by folder root
- `delete_folder_requested` — bulk unload by folder path prefix
- `on_delete_requested` — single file deletion + unload
- `rescan_timer_` — unloads active slot when its file is deleted on disk

Pattern: check if `active_doc_->path()` matches the slot(s) being unloaded → reset `active_doc_` → call `unload_dict()`.

## Slot Index Instability

`dict_workspace_t` stores slots in a `std::vector`. Indices shift on erasure. `dict_document_t` stores `slot_path_` (not an index). Any operation needing the current index resolves via `workspace_.find_by_path(slot_path_)` at call time.

The save callback:
```cpp
[this](const std::string & path) {
    int idx = workspace_.find_by_path(path);
    if (idx >= 0)
        save_dict_encoded(idx);
}
```

## workspace_.set_active() Stays In Sync

`workspace_.set_active(slot_idx)` is always called in parallel with setting `active_doc_` for dicts, and `workspace_.set_active(-1)` for YAML. Functions that use `workspace_.get_active_slot()` directly (find-replace, propagation, annotations, update_status_counts) continue to work because the workspace active index tracks the active document.

## Dict-Specific Commit Features

When `commit_current_edit()` operates on a `dict_document_t` (gated on `dynamic_cast`):
1. `history_manager_.record_change(...)` — before commit
2. `validation_manager_.validate(...)` — determines status ("error" vs "in_progress")
3. `slot->modified_records.insert({type, chapter_index})` — after commit
4. `annotation_manager_.update_term(...)` — after commit
5. `propagate_translation(...)` — mass-updates matching entries

YAML documents skip all five — their commit is a simple text update + `save_tmp()`.

## YAML Auto-Save on Every Edit

Current behavior: every YAML edit commit calls `save_lua_temp()` immediately. After refactoring, the YAML branch in `commit_current_edit()` calls `yaml_doc->save_tmp()` via downcast after every commit. The `save()` method also calls `save_tmp()` internally.

## load_config Startup

`load_config()` must create a `dict_document_t` when restoring `config_.active_dict_index`, not just call `workspace_.set_active()`. Otherwise `rebuild_table()` sees `active_doc_ == nullptr` and shows an empty table.

## What NOT to Do

- Do NOT use `dict_document_t::build_rows()` inside `rebuild_table()` for dicts — loses counting/grouping
- Do NOT store slot indices in `dict_document_t` — they shift on erasure
- Do NOT access `active_doc_` after calling `workspace_.unload_dict()` — pointer is invalidated
- Do NOT add `lua_active_path_`, `lua_entries_`, or `lua_modified_indices_` back to `main_window_t`
- Do NOT call `workspace_.get_active_slot()` for table/editor/save paths — use `active_doc_` instead
- Do NOT remove `workspace_.set_active()` calls — other code still relies on them (find-replace, propagation, annotations)
