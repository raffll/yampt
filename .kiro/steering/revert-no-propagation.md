# Revert Must Never Propagate

Revert operations (single-entry or batch) must NEVER trigger propagation. They restore a previous state — they do not create a new translation that should spread to other entries.

## Rule

When reverting an entry to a historical text/status:
- Write `entry.new_text` and `entry.status` directly on the `dict_document_t::data_mut()` record
- Call `dict_doc->set_dirty(true)` and `dict_doc->modified_records_insert(...)`
- Do NOT call `dict_document_t::commit()` — it contains propagation logic

`commit()` is for new user-initiated translations where propagation is intentional. Revert is the opposite: restoring old state without side effects.

## Single Source of Undo

The History panel is the only source of undo/revert in the application. There is no separate undo stack, no Ctrl+Z for entry text, and no other revert mechanism. All revert operations — whether triggered from the History panel (single entry) or the table context menu (batch) — read from `edit_history_t`.

## Why

If revert calls `commit()`, the reverted text propagates to all entries sharing the same `old_text`, silently overwriting unrelated translations across the document.
