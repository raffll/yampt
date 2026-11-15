# TODO

Bugs take precedence over features — always fix what is broken before adding anything new.

## Bugs
- Exclude Record menu may revert after Advanced Filters (unconfirmed): after Exclude Record from Merged Patch, the record's context menu offers Include Record, but after confirming Advanced Filters it offers Exclude Record again, while the rule stays in the ini and Excluded Only still matches. Seen only through the UI driver (scripts/e2e/run_suite.py --only exclude_record_toggle); confirm by hand first. Menu state comes from view_context_menu.cpp add_exclude_record_action reading settings_store merge_excludes() — yEditor

- Lua scan counts `.omwscripts` files that are not in the load order: OpenMW only loads omwscripts listed as content files, but `lua_scanner_t::scan` (lua_scanner.cpp) parses every `.omwscripts` at each data-folder root, so a disabled script file can still produce Lua conflicts. Pass the load order in and skip files not in it — yEditor

## Features
- Decide whether the Locked Only filter should also match records that have only field, sub-record, or bit locks; it currently matches whole-record locks only (nav_tree_model.cpp has_whole_record_lock) — yEditor

- let the user customize the colors in settings (bigger task than just adjusting defaults) — yTranslator
- colors blend together / hard to distinguish — adjust the palette so states are easier to tell apart (user reported with screenshot) — yTranslator
- Cross-workspace search window: search a phrase across all open dictionaries, results in a separate window with dictionary name column and highlighted match — yTranslator
- unify validation feedback across apps: yTranslator informs (three-tier, own validator) while yEditor blocks (binary, validator in yampt.core) — contradicts Consistent Across Apps; needs a design decision (shared validation core, unified message wording, deliberate block-vs-inform per app) before implementing
