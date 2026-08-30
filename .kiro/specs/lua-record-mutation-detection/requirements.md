# Requirements — Detect Lua-Driven Record Changes in yEditor

## Background

OpenMW Lua mods can change game data at runtime or at load time. From the OpenMW engine source:

- `world.createRecord(draft)` (global scope) — inserts a NEW record with an engine-generated id (`ESMStore::insert`); cannot overwrite an existing record.
- `types.<Type>.createRecordDraft{...}` — builds a record draft struct in Lua, normally passed to `world.createRecord`.
- The `openmw.content` package (LOAD context, active only while content files load) exposes mutable `records` collections for nearly every type; assigning to `content.<type>.records[id]` creates, edits, or deletes a record "as if a content file had done so". Record ids are immutable, all other fields are editable. This is the only path that edits EXISTING record data.
- `overrideRecord` exists in the engine store but is NOT exposed to Lua.

So a Lua mod can change what the game data looks like via script, and today yEditor gives no visibility into that. A user auditing plugin conflicts cannot see that a Lua mod also touches records — especially records that an ESM/ESP in the load order defines.

## What yEditor Already Does (and Doesn't)

yEditor has a Lua scanning pipeline (`yampt.core` scanner + a "Lua" tab in `plugin_workspace_view`):

- `lua_scanner_t::scan` discovers `.omwscripts` manifests, reads the referenced `.lua` files, and runs `handler_parser_t` on each.
- `handler_parser_t` recognizes ONLY event-handler registrations on three interfaces (SkillProgression, ItemUsage, Activation), classifies them blocking/mutating/passive, and `conflict_detector_t` flags cross-mod handler conflicts.
- Results (`lua_scan_result_t`: registrations, conflicts, warnings) show in the Lua tab; selecting one shows key/value rows in the record view.

It does NOT detect any record/data mutation calls (`world.createRecord`, `createRecordDraft`, `content.*.records`), does NOT use the omwscripts context tag (GLOBAL/PLAYER/LOAD is parsed then discarded), and has NO association between a Lua finding and an ESM record id/type.

## Goal

Give yEditor visibility, in the existing Lua tab, into which Lua scripts change game records/data — what kind of change, which record type/id, in which execution context — and flag when a Lua script changes a record that a loaded ESM/ESP plugin also defines (cross-domain).

## User-Facing Outcomes

- In the Lua tab, a user sees, per mod, the record-changing calls a script makes: create, draft, content edit/override, or delete, with the record type and id when statically determinable, and the script's execution context (e.g. LOAD vs GLOBAL).
- When a Lua script changes a record id that a loaded plugin also defines, the user sees a cross-domain flag identifying both the Lua script and the plugin(s).
- The Lua tab status/count reflects the number of record-change findings.

## Requirements

### R1 — Detect record/data mutation calls in Lua source

1.1 The scanner detects the following call kinds in Lua source, using the existing comment/string-stripped analysis so matches inside comments or string literals are ignored:
- `world.createRecord(...)` (and aliased `world` variables) — kind `create`.
- `types.<Type>.createRecordDraft{...}` / `(...)` — kind `draft`.
- `content.<type>.records[<id>] = ...` and `content.<type>.records.<id> = ...` — kind `content_edit` (create/override/edit of an existing record).
- `content.<type>.records[<id>] = nil` (or documented delete form) — kind `content_delete`.
1.2 Each finding records: mod name, script path, line number, call kind, the record type when determinable (e.g. `Potion`, `Npc`, `Weapon`, `gameSettings`), and the record id when it is a string literal.
1.3 When the record type or id cannot be resolved statically (computed/variable), the finding is still emitted with the unresolved field left empty and marked as unresolved, rather than dropped.
1.4 Detection is static text analysis of Lua source; it reports calls present in the source and does not execute Lua. This limitation is documented in user-facing docs.

### R2 — Carry the omwscripts execution context

2.1 The omwscripts context tags (already parsed into `omwscripts_entry_t::context_tags` but currently discarded) are forwarded to each record-mutation finding for the script that produced it.
2.2 Findings expose the context so the UI can distinguish LOAD-context edits (which edit existing records) from GLOBAL-context creates (which add new records) and other contexts.

### R3 — Cross-domain association with loaded plugins

3.1 When a record-mutation finding has a resolvable record id, it is matched against the records of the loaded ESM/ESP plugins.
3.2 A cross-domain flag is produced when a Lua finding's (type, id) matches a record defined by one or more loaded plugins, identifying the Lua script/mod and the plugin(s) that define the same record.
3.3 The join uses the existing plugin scan record set (record type + record id) and its exact `(type, id)` lookup.
3.4 Findings with unresolved ids, or ids that match no loaded plugin record, produce no cross-domain flag but still appear as plain record-change findings.
3.5 Record-type naming differs between Lua (`Potion`, `Npc`) and ESM (`ALCH`, `NPC_`); the match maps the Lua type to the corresponding ESM record type before comparing. Types that cannot be mapped are compared by id only, or reported without a cross-domain flag — decided in design.

### R4 — Surface findings in the existing Lua tab

4.1 Record-mutation findings appear as a section within the existing Lua tab (not a new view).
4.2 Selecting a finding shows its details in the record view: call kind, record type, record id, execution context, script path, line, and a source snippet; and, when cross-domain, the plugin(s) that define the same record.
4.3 A cross-domain finding is visually distinguished from a plain record-change finding (consistent with the tab's existing severity coloring approach).
4.4 The Lua tab count/status includes the number of record-change findings (and/or cross-domain flags), consistent with the existing count label.
4.5 No new logic is added to the editor main window; wiring goes through the existing workspace view / controllers per the Anti-Gravity Rule.

### R5 — Core-owned, no regression

5.1 All detection and cross-domain logic lives in `yampt.core` (pure, no Qt), consistent with the existing scanner architecture; the editor consumes results.
5.2 Existing handler-registration detection, classification, and cross-mod handler conflict detection are unchanged.
5.3 The scan remains a background operation feeding `lua_scan_result_t`; the new findings are added to that result type without breaking existing consumers.
5.4 Loose `.lua` files still must be referenced by an `.omwscripts` manifest to be scanned (existing behavior preserved).

### R6 — Correctness verified by unit tests

6.1 Unit tests cover mutation-call detection for each kind (create, draft, content_edit, content_delete), including aliased `world`/interface variables, and ignoring matches inside comments/strings.
6.2 Unit tests cover unresolved type/id (computed expressions) producing an unresolved finding rather than being dropped.
6.3 Unit tests cover context forwarding (a LOAD-context script's findings are tagged LOAD).
6.4 Unit tests cover the cross-domain join: a finding whose (type, id) maps to a loaded plugin record is flagged; unresolved or non-matching ids are not.
6.5 Unit tests cover the Lua↔ESM record-type mapping.
6.6 Tests are pure in-memory `[u]` unit tests, no file I/O; Lua source and plugin record sets are provided as in-memory fixtures.

## Open Decisions (resolve during design)

- **Lua↔ESM type map coverage**: which Lua `content`/`types` type names map to which ESM 4-char record types, and how to handle types with no direct ESM equivalent (GMST as `gameSettings`, globals, cells, dialogue).
- **Draft→create correlation**: whether to correlate a `createRecordDraft` with a following `world.createRecord` on the same variable, or report them independently. Independent reporting is simpler and safer for a static analyzer.
- **content assignment id extraction**: parsing `records[<expr>]` vs `records.<id>` to extract a literal id, and how to detect the delete form reliably.
- **Finding vs conflict modeling**: whether cross-domain matches are a separate list or a flag on each finding, and how they map onto the existing tree/severity model.
- **Counter semantics**: what the tab count reports (total findings, only cross-domain, or both).
