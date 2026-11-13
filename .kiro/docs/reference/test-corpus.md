# Real-World Test Corpus

A local Mod Organizer 2 install with a large real load order. Use it to test yEditor and yTranslator against real plugins (stress, real headers, real stubs). Synthetic tests stay the unit-test standard; this corpus is for end-to-end and manual verification.

**Never write into the corpus.** All plugin and profile files are the user's real mod data. To test anything that saves (edits, header preservation, merged patch output), copy the plugin(s) to a scratch folder under `%TEMP%` first.

## Location

- MO2 root: `C:\OMEN\Morrowind\MO2 profiles\POTI` (portable MO2 install)
- Mods: `...\POTI\mods\` (about 1006 mod folders)
- Profile: `...\POTI\profiles\Path of the Incarnate - Standard\` (`loadorder.txt` about 876 entries, `modlist.txt` about 1009)
- MO2 output (overwrite): `...\POTI\overwrite\`
- OpenMW configs: `...\POTI\.OpenMW\`

## Catalog

The catalog lists every plugin with its TES3 header (version, flags, author, description, record count), masters, record-type counts and providing mod. It also resolves the profile's load order and picks suggested test targets (small plugins with a real author, ESM-flagged files, the smallest plugins containing INFO, NPC_, LEVI, LEVC, CELL, SCPT or DIAL, the largest files, and the ones with the most masters).

- Summary: `.agents/corpus/poti-summary.md` (read this first)
- Full data: `.agents/corpus/poti-catalog.json`
- `.agents/` is gitignored, so the catalog is machine-local. Regenerate it with (about 10 s):

```
py scripts/catalog_corpus.py "C:\OMEN\Morrowind\MO2 profiles\POTI" .agents/corpus --name poti
```

Rough shape at the last scan: 1148 plugins (1031 .esp, 27 .esm, 90 .omwaddon), 216 double-extension OpenMW stubs, about 448k records (INFO about 181k, CELL about 62k), no malformed files. Four load-order entries are not in an enabled mod: the three base masters (resolved from the game data path) and `Merged Patch.esp` (yEditor output).

## Driving the apps (UI automation notes)

- Run the apps from a **snapshot** (`py scripts/snapshot_build.py`, which prints `%TEMP%\yampt_snapshot_release`) so a parallel build can still relink `x64\Release`.
- The agent console drops output and sends spurious `^C` on long or blocking calls. Run every UI session as one step list (`ui_driver.py run steps.json`) inside `scripts/run_detached.py`, then poll with `run_detached.py --wait TAG --timeout 90` in short bursts. Read results only from files.
- Qt tabs, list items and tree items: `click ... --type TabItem` uses UIA select, which is reliable. A synthetic mouse click can be ignored when the window is not in front.
- Menus: `menu "File->Open MO2 Profile"` (labels are matched ignoring `&`, a trailing `...` and shortcut text). To discover labels, use `popup_items "<window>" "File"`.
- Native folder/file pickers are owned windows. `find_window` searches children of top-level windows, so `dialog_path "Select MO2 Profile Folder" <path>` works.
- Wait for completion with `wait_text "^yEditor" "^[0-9]{3,} plugins, "` (status-bar summary). Read the Log tab with `click Log --type TabItem` then `texts`, and digest it with `scripts/log_digest.py`.
- Verify outcomes from files, not pixels: `scripts/esp_inspect.py` (headers, records), `scripts/e2e/check_merged_patch.py` (merged-patch invariants). Use screenshots only for purely visual checks.
- MO2-mode merges write to `overwrite/`. Use `scripts/e2e/make_mo2_sandbox.py`, whose junction makes `mods` readable while the merge output stays in the sandbox. Never edit a corpus plugin through the junction; copy plugins to a scratch folder and use Open Folder instead.
- The sandbox is kept between sessions at `%TEMP%\e2e\poti_sandbox` (profile `Path of the Incarnate - Standard`). Reuse it if it exists; only its `overwrite/` folder changes between runs. Recreate it with `make_mo2_sandbox.py` only if it is missing. Never delete it with a recursive delete, because that would follow the `mods` junction into the real corpus. Use `make_mo2_sandbox.py --remove`.

## Related scripts

- `scripts/catalog_corpus.py`: builds the catalog (read-only).
- `scripts/ui_driver.py`: drives the real yTranslator/yEditor through Windows UI Automation (launch, tree, click/select, menu, keys, screenshot, close).
- `scripts/run_detached.py`: runs a long command in the background and records its log and exit code in files.
- `scripts/debug_rebuild_and_test.py`: clean Debug rebuild plus the Debug logic suite with stall detection.
