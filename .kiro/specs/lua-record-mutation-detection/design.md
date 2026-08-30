# Design — Detect Lua-Driven Record Changes in yEditor

All detection and cross-domain logic lives in `yampt.core` (pure, no Qt), mirroring the existing Lua scanner. The editor consumes the extended result. The existing handler-registration path is untouched.

## Data model additions (yampt.core)

New enums/structs in the scanner headers:

```cpp
enum class lua_mutation_kind_t
{
    create,          // world.createRecord(...)
    draft,           // types.<T>.createRecordDraft{...}
    content_edit,    // content.<type>.records[id] = ...
    content_delete   // content.<type>.records[id] = nil
};

struct lua_record_mutation_t
{
    std::string mod_name;
    std::string script_path;
    int line_number = 0;
    lua_mutation_kind_t kind = lua_mutation_kind_t::create;
    std::string lua_type;        // e.g. "Potion", "Npc", "gameSettings"; empty if unresolved
    std::string record_id;       // literal id if resolvable; empty otherwise
    bool type_resolved = false;
    bool id_resolved = false;
    std::vector<std::string> context_tags; // from omwscripts (LOAD/GLOBAL/PLAYER/...)
    std::string snippet;         // short source excerpt for display
};

struct lua_record_crossref_t
{
    lua_record_mutation_t mutation;      // the Lua finding
    std::string esm_type;                // mapped ESM 4-char type (e.g. "ALCH")
    std::string record_id;               // matched id
    std::vector<std::string> plugin_filenames; // plugins that define the same record
};
```

`lua_scan_result_t` (lua_scanner.hpp) gains:
```cpp
std::vector<lua_record_mutation_t> record_mutations;
std::vector<lua_record_crossref_t> record_crossrefs;
```
Existing fields (`registrations`, `conflicts`, `warnings`) are unchanged (R5.2, R5.3).

## Component 1 — Mutation-call detection (yampt.core)

New class `lua_mutation_parser_t` (`yampt.core/source/scanner/lua_mutation_parser.hpp/.cpp`), sibling to `handler_parser_t`. Kept separate so `handler_registration_t` (handler-shaped) is not overloaded and the existing parser stays focused.

```cpp
class lua_mutation_parser_t
{
public:
    struct parse_input_t
    {
        std::string file_content;
        std::string script_path;
        std::string mod_name;
        std::vector<std::string> context_tags;
    };
    std::vector<lua_record_mutation_t> parse(const parse_input_t & input);
};
```

- Reuse the existing comment/string-stripping helpers from `handler_parser` (extract them into a shared `lua_lexing` namespace in core if they are currently file-local `static`, so both parsers share one implementation — no duplication) so matches inside comments/strings are ignored (R1.1).
- Reuse the existing alias tracking approach to resolve `world` and `content`/`types` aliases (e.g. `local W = require('openmw.world')`).
- Recognizers:
  - `create`: `<world>.createRecord(` — record type/id usually not literal here (comes from the draft), so `type_resolved`/`id_resolved` often false (R1.3).
  - `draft`: `types.<Type>.createRecordDraft` — capture `<Type>` as `lua_type`.
  - `content_edit`: `content.<type>.records[<expr>] = ` or `content.<type>.records.<id> = ` — capture `<type>` and, if `<expr>`/`<id>` is a string literal, `record_id`; RHS `nil` (or the documented delete form) → `content_delete`.
- Extract a short `snippet` (the matched line, trimmed) for display.
- Context tags are copied from the input (R2.1, R2.2).

Draft and create are reported independently (chosen over draft→create correlation) — simpler and robust for static analysis (Open Decision resolved).

## Component 2 — Lua↔ESM type mapping (yampt.core)

New pure `lua_esm_type_map` namespace (`yampt.core/source/scanner/lua_esm_type_map.hpp/.cpp`):
```cpp
namespace lua_esm_type_map
{
    // "Potion" -> "ALCH", "Npc" -> "NPC_", "Weapon" -> "WEAP", "gameSettings" -> "GMST", ...
    std::string to_esm_type(std::string_view lua_type); // "" if no mapping
}
```
- A static table covering the Lua type names used by `openmw.content`/`openmw.types` and their ESM 4-char equivalents.
- Types with no ESM equivalent (or ambiguous) return empty; the cross-ref step then skips the type comparison and either matches by id only or omits the flag (decided below).

## Component 3 — Cross-domain join (yampt.core)

New pure function/class `lua_record_crossref_t build_crossrefs(mutations, plugin records)`:

- Input: the `lua_record_mutation_t` list plus a lightweight view of loaded plugin records. To keep core pure and testable, the join takes a simple injected lookup, e.g. `std::function<std::vector<std::string>(const std::string & esm_type, const std::string & id)>` returning the plugin filenames that define `(type,id)`, or a plain vector of `{esm_type, id, plugin}` tuples.
- For each mutation with `id_resolved`:
  - map `lua_type` → `esm_type` (empty if unmapped),
  - query the lookup; if it returns one or more plugins, emit a `lua_record_crossref_t` (R3.1, R3.2).
- Unresolved ids or no-match ids produce no crossref (R3.4).
- When `esm_type` is empty (unmapped Lua type), match by id only across all types (documented as a looser match) — Open Decision resolved toward id-only fallback so nothing is silently missed; the UI marks it as an id-only match.

The editor supplies the lookup by adapting `plugin_scan_t::entries()` / `plugin_scan_t::find(type, id)` (both already exist: `entries()` returns every `conflict_entry_t{rec_type, record_id}`, and `find` does exact `(type,id)` lookup — R3.3).

## Component 4 — Scanner orchestration (yampt.core)

`lua_scanner_t::scan` (lua_scanner.cpp) is extended:
- `parse_single_entry` also runs `lua_mutation_parser_t` on each `.lua` file, forwarding the omwscripts `context_tags` (currently dropped) into the parse input (R2.1). Append results to `lua_scan_result_t::record_mutations`.
- The cross-domain join needs plugin records. Two options:
  - (a) pass a plugin-record lookup into `scan(...)` as a new parameter, or
  - (b) leave `scan` as-is and run `build_crossrefs` in the editor after the scan, feeding `plugin_scan_t`.
  Design choice: (b) — keep `lua_scanner_t::scan` signature stable (background worker already calls it), and compute crossrefs in the editor’s scan-complete path using the loaded `plugin_scan_t`, storing the result into `lua_scan_result_t::record_crossrefs`. The join function itself stays pure in core and unit-tested with fixtures.

## Component 5 — Editor UI (yampt.editor)

Reuse the existing Lua tab and record view (R4.1):

- `plugin_workspace_view_t::on_lua_scan_complete` (already stores `m_lua_scan_result`) additionally calls the core `build_crossrefs` with a lookup adapting `m_session->scan()` and stores the crossrefs into the result. Then refreshes the Lua tree and status.
- `lua_tree_model_t` gains a new top-level section (alongside registrations/conflicts) listing record mutations grouped by mod, with cross-domain findings visually distinguished (reuse the existing severity color mechanism — e.g. cross-domain = red, plain change = amber) (R4.3). `node_info_t` gains a discriminator so selection can route to the mutation detail.
- `on_lua_selection_changed` routes a mutation node to a new `view_tree_model_t::set_lua_record_mutation(...)` (sibling to `set_lua_registration`/`set_lua_conflict`) showing rows: Call Kind, Record Type, Record Id, Context, Script Path, Line, Snippet, and (if cross-domain) the defining plugin(s) (R4.2).
- `count_label_format` / `update_status` include a record-change (and/or cross-domain) counter (R4.4).
- All wiring stays in the workspace view; no logic added to `editor_window_t` (R4.5).

## Files

New (yampt.core):
- `scanner/lua_mutation_parser.hpp/.cpp`
- `scanner/lua_esm_type_map.hpp/.cpp`
- `scanner/lua_record_crossref.hpp/.cpp` (the pure `build_crossrefs` + structs, or fold structs into lua_scanner.hpp)
- shared `scanner/lua_lexing.hpp/.cpp` if comment/string stripping is extracted from `handler_parser` for reuse.

Modified (yampt.core):
- `scanner/lua_scanner.hpp` — extend `lua_scan_result_t`.
- `scanner/lua_scanner.cpp` — run mutation parser, forward context tags.
- `scanner/omwscripts` wiring in `lua_scanner.cpp` — stop dropping `context_tags`.

Modified (yampt.editor):
- `model/lua_tree_model.*` — new section + node discriminator + coloring.
- `model/view_tree_model.*` — `set_lua_record_mutation`.
- `view/plugin_workspace_view.*` — compute crossrefs on scan complete, route selection, update count.
- `view/count_label_format.*` — new counter.

vcxproj / vcxproj.filters updated for all new core + test files (flat filters per project-paths rules).

Docs: yEditor manual + README/CHANGELOG for the user-visible feature (per changelog/doc rules); note the static-analysis limitation (R1.4).

## Testing (pure `[u]`, no file I/O)

- `lua_mutation_parser_t`: each kind detected; aliased `world`/`content`/`types`; matches in comments/strings ignored; unresolved type/id emitted as unresolved; context tags forwarded.
- `lua_esm_type_map::to_esm_type`: known mappings and unmapped returns empty.
- `build_crossrefs`: resolvable id matching a plugin record flagged; unresolved/non-matching not flagged; unmapped type falls back to id-only match and is marked as such.
- Fixtures provide Lua source strings and an in-memory plugin-record list; no disk access.
