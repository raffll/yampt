"""yEditor end-to-end scenario suite: one command, one compact report.

Each scenario builds its own scratch fixture under %TEMP%\\e2e\\suite (plugins are COPIED
from the POTI corpus, never edited in place), drives a yEditor snapshot through
yeditor_app.py, and checks the result from files and the Log tab. No screenshots are
taken unless a scenario fails.

Scenarios are tagged "confirmed" (already verified, kept for regression runs) or "new".
By default only "new" scenarios run; bug-fix work should not re-run confirmed ones.

Usage:
  py scripts/e2e/run_suite.py                     run the "new" scenarios
  py scripts/e2e/run_suite.py --only a,b          run the named scenarios
  py scripts/e2e/run_suite.py --regression        run every scenario
  py scripts/e2e/run_suite.py --list              list scenarios and tags

Prerequisite: py scripts/snapshot_build.py (the suite drives %TEMP%\\yampt_snapshot_release).
Report: %TEMP%\\e2e\\suite\\report.json; one summary line per scenario is printed.
"""

import argparse
import json
import os
import shutil
import sys
import time
import traceback

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from compare_plugin_edit import read_raw_records  # noqa: E402
from yeditor_app import act, step_error_t, yeditor_app_t  # noqa: E402

temp_dir = os.environ.get("TEMP", ".")
suite_dir = os.path.join(temp_dir, "e2e", "suite")
snapshot_exe = os.path.join(temp_dir, "yampt_snapshot_release", "yEditor.exe")
corpus_mods = r"C:\OMEN\Morrowind\MO2 profiles\POTI\mods"
ahnassi_source = os.path.join(corpus_mods, "Romance Ahnassi Regardless of Gender", "AhnassiRomanceNeutralGender.esp")
timescale_mod = os.path.join(corpus_mods, "TimeScale Change - OpenMW")
npdt_level_offset = 583
npdt_strength_offset = 585


class check_failed_t(Exception):
    pass


def expect(condition, message):
    if not condition:
        raise check_failed_t(message)


def fresh_dir(name):
    path = os.path.join(suite_dir, name)
    if os.path.isdir(path):
        shutil.rmtree(path)
    os.makedirs(path)
    return path


def read_npc_stats(plugin_path):
    with open(plugin_path, "rb") as handle:
        blob = handle.read()
    npdt = blob.find(b"NPDT") + 8
    return int.from_bytes(blob[npdt:npdt + 2], "little"), blob[npdt + 2]


def build_merge_fixture(name):
    """Three versions of NPC 'ahnassi': base, Level 9->10, Strength 59->60 (load order by file time)."""
    folder = fresh_dir(name)
    with open(ahnassi_source, "rb") as handle:
        original = bytearray(handle.read())
    expect(original[npdt_level_offset] == 9 and original[npdt_strength_offset] == 59, "corpus Ahnassi bytes changed")
    level_version = bytearray(original)
    level_version[npdt_level_offset] = 10
    strength_version = bytearray(original)
    strength_version[npdt_strength_offset] = 60
    stamp = time.time() - 3600
    for index, (filename, content) in enumerate(
            [("A_Base.esp", original), ("B_Level.esp", level_version), ("C_Strength.esp", strength_version)]):
        path = os.path.join(folder, filename)
        with open(path, "wb") as handle:
            handle.write(content)
        os.utime(path, (stamp + index * 300, stamp + index * 300))
    return folder


def scenario_header_preserve(app):
    folder = fresh_dir("header_preserve")
    original_path = os.path.join(suite_dir, "header_preserve_original.esp")
    shutil.copy2(ahnassi_source, original_path)
    edited_path = os.path.join(folder, "AhnassiRomanceNeutralGender.esp")
    shutil.copy2(ahnassi_source, edited_path)
    app.open_folder(folder, 1)
    app.nav_menu(r"\[000\] AhnassiRomanceNeutralGender\.esp$", "Set as Active Plugin")
    app.open_record(r"AhnassiRomanceNeutralGender\.esp$", r"^NPC \[\d+\]$", r"^ahnassi$")
    app.edit_cell("Level", r"AhnassiRomanceNeutralGender", "10")
    original, _, _ = read_raw_records(original_path)
    edited, _, _ = read_raw_records(edited_path)
    expect(len(original) == len(edited), f"record count changed {len(original)} -> {len(edited)}")
    changed = [index for index, (left, right) in enumerate(zip(original, edited)) if left["bytes"] != right["bytes"]]
    expect(changed == [0, 1], f"unexpected changed records {changed}")
    header_diff = [o for o in range(len(original[0]["bytes"])) if original[0]["bytes"][o] != edited[0]["bytes"][o]]
    expect(len(header_diff) <= 1, f"TES3 header changed at {header_diff} (only the record count may change)")
    return "only the Level byte and the header record count changed"


def scenario_stub_folder(app):
    folder = fresh_dir("stub_folder")
    for filename in ("TimescaleChange.omwaddon.esp", "TimescaleChange.omwaddon"):
        shutil.copy2(os.path.join(timescale_mod, filename), folder)
    shutil.copy2(ahnassi_source, folder)
    app.open_folder(folder, 3)
    titles = app.nav_titles()
    expect(any(title.endswith("TimescaleChange.omwaddon") for title in titles), "real .omwaddon not in tree")
    expect(not any(title.endswith(".omwaddon.esp") for title in titles), "stub visible while hidden by default")
    app.restart()
    app.wait_status(r"^3 plugins, ")
    return "real file loaded and visible, stub hidden, restored after restart"


def scenario_merge_lock_restart(app):
    folder = build_merge_fixture("merge_lock_restart")
    patch_path = os.path.join(folder, "Merged Patch.esp")
    app.open_folder(folder, 3)
    app.create_merged_patch(os.path.join(folder, "Merged Patch.esp"))
    expect(read_npc_stats(patch_path) == (10, 60), f"3-way merge gave {read_npc_stats(patch_path)}, expected (10, 60)")
    app.open_record(r"Merged Patch\.esp$", r"^NPC \[\d+\]$", r"^ahnassi$")
    app.lock_cell("Level", r"Merged Patc")
    app.edit_cell("Level", r"Merged Patc", "11")
    expect(read_npc_stats(patch_path)[0] == 11, "edited Level not saved to the merged patch")
    app.restart()
    app.wait_status(r"^4 plugins, ")
    app.create_merged_patch(os.path.join(folder, "Merged Patch.esp"))
    expect(read_npc_stats(patch_path) == (11, 60), f"after restart and regenerate got {read_npc_stats(patch_path)}")
    return "merge (10, 60); locked edit 11 kept after restart and regenerate"


def has_merged_patch_record(app):
    titles = app.nav_titles()
    return "ahnassi" in titles


def scenario_filters_locked_excluded(app):
    folder = build_merge_fixture("filters")
    app.open_folder(folder, 3)
    app.create_merged_patch(os.path.join(folder, "Merged Patch.esp"))
    app.set_special_filters({"Locked Only": True, "Excluded Only": False})
    app.expand_all_nav()
    expect(not has_merged_patch_record(app), "Locked Only shows a record before anything is locked")
    app.set_special_filters({"Locked Only": False, "Excluded Only": False})
    app.expand_nav(r"Merged Patch\.esp$")
    app.expand_nav(r"^NPC \[\d+\]$")
    app.nav_menu(r"^ahnassi$", "Lock in Merged Patch")
    app.set_special_filters({"Locked Only": True, "Excluded Only": False})
    app.expand_all_nav()
    expect(has_merged_patch_record(app), "Locked Only hides the whole-record locked record")
    app.set_special_filters({"Locked Only": False, "Excluded Only": True})
    app.expand_all_nav()
    expect(not has_merged_patch_record(app), "Excluded Only shows a record before any exclusion rule")
    app.set_special_filters({"Locked Only": False, "Excluded Only": False})
    app.expand_nav(r"\[000\] A_Base\.esp$")
    app.expand_nav(r"^NPC \[\d+\]$")
    app.nav_menu(r"^ahnassi$", "Exclude Record from Merged Patch")
    app.set_special_filters({"Locked Only": False, "Excluded Only": True})
    app.expand_all_nav()
    expect(has_merged_patch_record(app), "Excluded Only hides the excluded record")
    app.set_special_filters({"Locked Only": False, "Excluded Only": False})
    app.expand_all_nav()
    app.nav_menu(r"^ahnassi$", "Include Record in Merged Patch")
    return "Locked Only and Excluded Only show exactly the locked / excluded record"


def menu_labels(app, title_regex):
    app.click_nav(title_regex, button="right")
    labels = act("popup_items", "-")["items"]
    act("keys", r"^yEditor($| - )", "{ESC}")
    return [label for label in labels if "Record" in label]


def scenario_exclude_record_toggle(app):
    folder = build_merge_fixture("exclude_toggle")
    app.open_folder(folder, 3)
    app.expand_nav(r"\[000\] A_Base\.esp$")
    app.expand_nav(r"^NPC \[\d+\]$")
    if "Include Record in Merged Patch" in menu_labels(app, r"^ahnassi$"):
        app.nav_menu(r"^ahnassi$", "Include Record in Merged Patch")
    before = menu_labels(app, r"^ahnassi$")
    app.nav_menu(r"^ahnassi$", "Exclude Record from Merged Patch")
    after_exclude = menu_labels(app, r"^ahnassi$")
    app.set_special_filters({"Excluded Only": False})
    after_filter_dialog = menu_labels(app, r"^ahnassi$")
    app.restart()
    app.wait_status(r"^3 plugins, ")
    app.expand_nav(r"\[000\] A_Base\.esp$")
    app.expand_nav(r"^NPC \[\d+\]$")
    after_restart = menu_labels(app, r"^ahnassi$")
    detail = (f"before={before} after_exclude={after_exclude} after_filter_dialog={after_filter_dialog} "
              f"after_restart={after_restart}")
    expect("Include Record in Merged Patch" in after_restart, "exclusion lost after restart: " + detail)
    expect("Include Record in Merged Patch" in after_exclude, "menu offers Exclude right after excluding: " + detail)
    expect("Include Record in Merged Patch" in after_filter_dialog,
           "exclusion lost after confirming Advanced Filters: " + detail)
    app.nav_menu(r"^ahnassi$", "Include Record in Merged Patch")
    return detail


scenarios = {
    "exclude_record_toggle": (scenario_exclude_record_toggle, "new"),
    "header_preserve": (scenario_header_preserve, "confirmed"),
    "stub_folder": (scenario_stub_folder, "confirmed"),
    "merge_lock_restart": (scenario_merge_lock_restart, "confirmed"),
    "filters_locked_excluded": (scenario_filters_locked_excluded, "new"),
}


def run_scenario(app, name):
    function, _ = scenarios[name]
    started = time.time()
    try:
        app.launch()
        detail = function(app)
        return {"scenario": name, "status": "PASS", "detail": detail, "seconds": round(time.time() - started, 1)}
    except (check_failed_t, step_error_t) as error:
        status, detail = ("FAIL" if isinstance(error, check_failed_t) else "ERROR"), str(error)
    except Exception as error:
        status, detail = "ERROR", f"{type(error).__name__}: {error} @ {traceback.format_exc(limit=-1).strip()}"
    return {"scenario": name, "status": status, "detail": detail, "seconds": round(time.time() - started, 1),
            "screenshot": app.screenshot(name + "_failure"), "log_tail": app.log_tail()}


def select_scenarios(arguments):
    if arguments.only:
        names = [name.strip() for name in arguments.only.split(",") if name.strip()]
        unknown = [name for name in names if name not in scenarios]
        if unknown:
            raise SystemExit(f"unknown scenarios: {unknown}")
        return names
    if arguments.regression:
        return list(scenarios)
    return [name for name, (_, tag) in scenarios.items() if tag == "new"]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--only")
    parser.add_argument("--regression", action="store_true")
    parser.add_argument("--list", action="store_true")
    arguments = parser.parse_args()
    if arguments.list:
        for name, (_, tag) in scenarios.items():
            print(f"{name}  [{tag}]")
        return 0
    os.makedirs(suite_dir, exist_ok=True)
    app = yeditor_app_t(snapshot_exe, suite_dir)
    results = [run_scenario(app, name) for name in select_scenarios(arguments)]
    try:
        app.close()
    except Exception:
        pass
    with open(os.path.join(suite_dir, "report.json"), "w", encoding="utf-8") as handle:
        json.dump(results, handle, indent=1, ensure_ascii=False)
    sys.stdout.reconfigure(errors="replace")
    for result in results:
        print(f"{result['status']:5} {result['scenario']} ({result['seconds']}s): {result['detail']}")
    return 0 if all(result["status"] == "PASS" for result in results) else 1


if __name__ == "__main__":
    sys.exit(main())
