"""High-level yEditor driver for end-to-end scenarios (used by run_suite.py).

Wraps ui_driver.py into user-level actions (open a folder, open a record, edit or lock a
cell, create the merged patch, apply Advanced Filters) so scenarios read like the steps a
user takes. Every helper waits for its own result and raises step_error_t with a short
reason, so a scenario never needs screenshots or coordinates.

Known Qt/UIA quirks handled here (do not re-learn them in scenarios):
- Buttons that open a modal dialog are clicked with the mouse: a UIA invoke blocks until
  the dialog closes.
- Qt message boxes do not expose their buttons to UIA; they are answered with Alt+<key>.
- A UIA selection does not move Qt's current index; tree rows are clicked with the mouse.
- Only one yEditor instance may run: launch() stops strays from the snapshot first.
"""

import os
import re
import subprocess
import sys
import time
from argparse import Namespace

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import ui_driver  # noqa: E402

main_window = r"^yEditor($| - )"
regenerate_dialog = r"^Regenerate Merged Patch$"
plugin_select_dialog = r"^Select Plugins$"
filter_dialog = r"^Advanced Filters$"
nav_tree_marker = "nav_tree_view_t"
record_view_marker = "record_view_t"
preview_marker = "preview_view_t"


class step_error_t(Exception):
    pass


def act(command, target="-", extra=None, **options):
    arguments = Namespace(command=command, target=target, extra=extra, depth=options.get("depth", 4),
                          wait=options.get("wait", 4.0), type=options.get("type"), out=None,
                          mouse=options.get("mouse", False), offset_x=options.get("offset_x", 0))
    return ui_driver.commands[command](arguments)


def poll(description, predicate, timeout_seconds=30.0, interval=0.5):
    deadline = time.time() + timeout_seconds
    last_error = None
    while time.time() < deadline:
        try:
            result = predicate()
            if result:
                return result
        except Exception as error:  # UIA lookups fail transiently while Qt rebuilds widgets
            last_error = error
        time.sleep(interval)
    raise step_error_t(f"timed out waiting for {description}" + (f" ({last_error})" if last_error else ""))


class yeditor_app_t:
    def __init__(self, exe_path, screenshot_dir):
        self.exe_path = exe_path
        self.screenshot_dir = screenshot_dir

    def window(self):
        return ui_driver.find_window(main_window, timeout_seconds=10)

    def stop_snapshot_instances(self):
        """Stop yEditor processes started from the snapshot only; never touch the user's own instances."""
        snapshot_folder = os.path.dirname(self.exe_path).replace("'", "''")
        script = (f"Get-Process yEditor -ErrorAction SilentlyContinue | "
                  f"Where-Object {{ $_.Path -like '{snapshot_folder}\\*' }} | Stop-Process -Force")
        subprocess.run(["powershell", "-NoProfile", "-Command", script], capture_output=True)

    def start_process(self):
        """Start yEditor with --debug so %TEMP%\\yEditor.log carries the gated [debug] lines."""
        subprocess.Popen([self.exe_path, "--debug"], cwd=os.path.dirname(self.exe_path))
        poll("yEditor main window", lambda: self.window(), timeout_seconds=30)
        time.sleep(2.0)

    def launch(self):
        self.stop_snapshot_instances()
        time.sleep(1.0)
        self.start_process()

    def close(self):
        act("close", main_window)
        poll("yEditor to exit", lambda: not self.is_running(), timeout_seconds=20)

    def restart(self):
        self.close()
        self.start_process()

    @staticmethod
    def log_tail(line_count=40):
        """Last log lines, without the plugin reader's per-record [debug] noise."""
        log_path = os.path.join(os.environ.get("TEMP", "."), "yEditor.log")
        if not os.path.exists(log_path):
            return []
        reader_noise = re.compile(r"\[debug\] (tiling ok|record_begin=)")
        with open(log_path, "r", encoding="utf-8", errors="replace") as handle:
            lines = [line.rstrip("\n") for line in handle if not reader_noise.search(line)]
        return lines[-line_count:]

    def is_running(self):
        snapshot_folder = os.path.dirname(self.exe_path).replace("'", "''")
        script = (f"@(Get-Process yEditor -ErrorAction SilentlyContinue | "
                  f"Where-Object {{ $_.Path -like '{snapshot_folder}\\*' }}).Count")
        output = subprocess.run(["powershell", "-NoProfile", "-Command", script], capture_output=True, text=True).stdout
        return output.strip() not in ("", "0")

    def screenshot(self, name):
        path = os.path.join(self.screenshot_dir, name + ".png")
        try:
            act("shot", main_window, path)
            return path
        except Exception:
            return None

    def status_text(self):
        pattern = re.compile(r"\d+ plugins?, \d+ records")
        for element in self.window().descendants():
            name = element.element_info.name or ""
            if pattern.search(name):
                return name
        return ""

    def wait_status(self, regex, timeout_seconds=60.0):
        compiled = re.compile(regex)
        return poll(f"status /{regex}/", lambda: compiled.search(self.status_text()), timeout_seconds)

    def open_folder(self, folder_path, plugin_count):
        act("menu", main_window, "File->Open Folder...")
        act("dialog_path", "Select", folder_path)
        poll("plugin selection dialog", lambda: ui_driver.find_window(plugin_select_dialog, 2), 20)
        act("click", plugin_select_dialog, "OK", type="Button")
        self.wait_status(rf"^{plugin_count} plugins?, ")

    def tree_items(self, view_marker):
        items = self.window().descendants(control_type="TreeItem")
        return [item for item in items if view_marker in (item.element_info.automation_id or "")]

    def find_tree_item(self, view_marker, title_regex):
        compiled = re.compile(title_regex)
        matches = [item for item in self.tree_items(view_marker) if compiled.search(item.element_info.name or "")]
        return matches[0] if matches else None

    def nav_titles(self):
        return [item.element_info.name or "" for item in self.tree_items(nav_tree_marker)]

    def click_nav(self, title_regex, button="left"):
        item = poll(f"nav item /{title_regex}/", lambda: self.find_tree_item(nav_tree_marker, title_regex), 15)
        self.window().set_focus()
        rect = item.element_info.rectangle
        item.click_input(button=button, coords=(min(60, rect.width() // 2), rect.height() // 2))
        time.sleep(0.6)

    def expand_nav(self, title_regex):
        """Click the row, then Right arrow (Qt ignores UIA's ExpandCollapse pattern on these rows)."""
        self.click_nav(title_regex)
        self.window().type_keys("{RIGHT}", set_foreground=True)
        time.sleep(0.6)

    def expand_all_nav(self):
        for item in list(self.tree_items(nav_tree_marker)):
            title = item.element_info.name or ""
            if re.search(r"\[\d{3}\]", title):
                self.click_nav(re.escape(title) + "$")
                self.window().type_keys("*", set_foreground=True)
                time.sleep(0.4)

    def open_record(self, plugin_regex, group_regex, record_regex):
        """Expand plugin -> record group, then click the record (rows are matched in that order)."""
        self.expand_nav(plugin_regex)
        self.expand_nav(group_regex)
        self.click_nav(record_regex)
        poll("record view rows", lambda: self.tree_items(record_view_marker), 15)

    def nav_menu(self, title_regex, action_label):
        self.click_nav(title_regex, button="right")
        act("popup_click", "-", action_label)
        time.sleep(1.0)

    def cell_point(self, row_label, column_regex):
        """Window-relative point of the record-view cell in row `row_label` and column `column_regex`."""
        row = poll(f"record row '{row_label}'",
                   lambda: self.find_tree_item(record_view_marker, rf"^{re.escape(row_label)}$"), 15)
        compiled = re.compile(column_regex)
        headers = [element for element in self.window().descendants(control_type="HeaderItem")
                   if record_view_marker in (element.element_info.automation_id or "")
                   and compiled.search(element.element_info.name or "")]
        if not headers:
            raise step_error_t(f"no record-view column matching /{column_regex}/")
        window_rect = self.window().element_info.rectangle
        header_rect = headers[0].element_info.rectangle
        row_rect = row.element_info.rectangle
        return ((header_rect.left + header_rect.right) // 2 - window_rect.left,
                (row_rect.top + row_rect.bottom) // 2 - window_rect.top)

    def click_cell(self, row_label, column_regex, button="left"):
        point_x, point_y = self.cell_point(row_label, column_regex)
        self.window().set_focus()
        self.window().click_input(button=button, coords=(point_x, point_y))
        time.sleep(0.8)

    def lock_cell(self, row_label, column_regex):
        self.click_cell(row_label, column_regex, button="right")
        act("popup_click", "-", "Lock in Merged Patch")
        time.sleep(1.0)

    def edit_cell(self, row_label, column_regex, value):
        self.click_cell(row_label, column_regex)
        editors = [element for element in self.window().descendants(control_type="Edit")
                   if preview_marker in (element.element_info.automation_id or "")]
        if len(editors) < 2:
            raise step_error_t("edit panel text boxes not found")
        selected_editor = max(editors, key=lambda element: element.element_info.rectangle.left)
        selected_editor.click_input()
        self.window().type_keys("^a" + value, with_spaces=True, set_foreground=True)
        time.sleep(0.4)
        act("click", main_window, "Apply", type="Button", mouse=True)
        time.sleep(1.5)

    @staticmethod
    def file_stamp(path):
        return os.path.getmtime(path) if os.path.exists(path) else None

    def create_merged_patch(self, patch_path):
        """Click Create Merged Patch, confirm a regeneration prompt, and wait until patch_path is rewritten."""
        stamp_before = self.file_stamp(patch_path)
        act("click", main_window, "Create Merged Patch", type="Button", mouse=True)
        time.sleep(2.0)
        try:
            ui_driver.find_window(regenerate_dialog, timeout_seconds=2)
            act("keys", regenerate_dialog, "%y")
        except RuntimeError:
            pass
        poll("merged patch save", lambda: self.file_stamp(patch_path) not in (None, stamp_before), 300, 1.0)
        time.sleep(1.0)

    def set_special_filters(self, wanted):
        """wanted: {"Locked Only": bool, "Excluded Only": bool, ...}; toggles checkboxes to match, then OK."""
        act("click", main_window, "Advanced Filters...", type="Button", mouse=True)
        dialog = poll("Advanced Filters dialog", lambda: ui_driver.find_window(filter_dialog, 2), 15)
        for label, checked in wanted.items():
            boxes = dialog.descendants(control_type="CheckBox", title=label)
            if not boxes:
                raise step_error_t(f"filter checkbox '{label}' not found")
            if bool(boxes[0].get_toggle_state()) != checked:
                boxes[0].toggle()
        act("click", filter_dialog, "OK", type="Button", mouse=True)
        time.sleep(1.5)
