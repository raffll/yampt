# Tasks — Detect Lua-Driven Record Changes in yEditor

Order: core detection first (parser, type map, crossref join) with unit tests, then wire into the scanner, then the editor UI, then docs. All language logic in `yampt.core`.

## 1. Core: shared Lua lexing

- [ ] 1.1 If comment/string stripping in `handler_parser.cpp` is file-local `static`, extract it into a shared `yampt.core/source/scanner/lua_lexing.hpp/.cpp` namespace and have `handler_parser` use it (no behavior change, no duplication). Add files to `yampt.core.vcxproj` + filters. (R5.1, DRY)

## 2. Core: mutation-call detection

- [ ] 2.1 Add data types (`lua_mutation_kind_t`, `lua_record_mutation_t`) — in `lua_scanner.hpp` or a dedicated header. (R1.2)
- [ ] 2.2 Create `scanner/lua_mutation_parser.hpp/.cpp` with `lua_mutation_parser_t::parse(parse_input_t)`; add to vcxproj + filters. (R1.1)
- [ ] 2.3 Implement recognizers using shared lexing + alias tracking: `create` (`world.createRecord`), `draft` (`types.<T>.createRecordDraft`), `content_edit` / `content_delete` (`content.<type>.records[...] = ...` / `= nil`). Capture type/id when literal; set `type_resolved`/`id_resolved`; capture snippet; copy context tags. (R1.1–R1.3, R2.1–R2.2)

## 3. Core: unit tests for detection

- [ ] 3.1 Add `yampt.tests/source/tests.lua_mutation_parser.cpp` (register in vcxproj + filters). (R6.1–R6.3, R6.6)
- [ ] 3.2 Tests: each kind detected; aliased `world`/`content`/`types`; matches in comments/strings ignored; unresolved type/id emitted (not dropped); context tags forwarded.

## 4. Core: Lua↔ESM type map

- [ ] 4.1 Create `scanner/lua_esm_type_map.hpp/.cpp` with `to_esm_type(std::string_view) -> std::string` and a static mapping table; add to vcxproj + filters. (R3.5)
- [ ] 4.2 Unit tests `tests.lua_esm_type_map.cpp`: known mappings; unmapped returns empty. (R6.5)

## 5. Core: cross-domain join

- [ ] 5.1 Add `lua_record_crossref_t` and a pure `build_crossrefs(mutations, lookup)` (`scanner/lua_record_crossref.hpp/.cpp`), taking an injected `(esm_type, id) -> plugin filenames` lookup. Add to vcxproj + filters. (R3.1–R3.4)
- [ ] 5.2 Implement: map type, query lookup, emit crossref for resolvable matches; unmapped type falls back to id-only match flagged as such; unresolved/non-matching produce none. (R3.2–R3.5)
- [ ] 5.3 Unit tests `tests.lua_record_crossref.cpp`: resolvable match flagged; unresolved/non-matching not flagged; unmapped-type id-only fallback. (R6.4, R6.6)

## 6. Core: scanner orchestration

- [ ] 6.1 Extend `lua_scan_result_t` with `record_mutations` and `record_crossrefs`. (R5.3)
- [ ] 6.2 In `lua_scanner.cpp` `parse_single_entry`, forward the omwscripts `context_tags` (stop dropping them) and run `lua_mutation_parser_t`, appending to `record_mutations`. Keep `scan(...)` signature stable. (R2.1, R5.3, R5.4)

## 7. Editor: compute crossrefs and wire the Lua tab

- [ ] 7.1 In `plugin_workspace_view_t::on_lua_scan_complete`, build an `(esm_type,id)->plugins` lookup adapting `m_session->scan().entries()` / `find(...)`, call core `build_crossrefs`, and store into `m_lua_scan_result.record_crossrefs`. (R3.3, R4.5)
- [ ] 7.2 `lua_tree_model_t`: add a record-mutations section grouped by mod; add a `node_info_t` discriminator; color cross-domain findings distinctly using the existing severity mechanism. (R4.1, R4.3)
- [ ] 7.3 `view_tree_model_t::set_lua_record_mutation(...)`: detail rows (Call Kind, Record Type, Record Id, Context, Script Path, Line, Snippet, defining plugins if cross-domain). (R4.2)
- [ ] 7.4 Route `on_lua_selection_changed` to `set_lua_record_mutation` for mutation nodes. (R4.2)
- [ ] 7.5 Extend `count_label_format` / `update_status` with a record-change (and cross-domain) counter. (R4.4)

## 8. Documentation

- [ ] 8.1 Update `docs/yEditor-Manual.md` describing the Lua record-change section and cross-domain flag (manual-style prose); note it is static analysis of Lua source, not runtime behavior. (R1.4)
- [ ] 8.2 Update `README.md`, `docs/README.bbcode` (mirror), `CHANGELOG.md` under `2.0beta` (`[NEW]`, yEditor section). No tests/scripts/build details. (changelog-categories, readme rules)

## Notes

- yEditor-only; yTranslator has no plugin scan.
- All detection/join logic is pure `yampt.core`; the editor only adapts `plugin_scan_t` and renders.
- Reuse existing lexing and alias tracking; do not duplicate.
- Building and running tests are done manually by the user (project no-build rule); tasks produce tests as artifacts, no "run tests" step.
