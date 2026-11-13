"""Drive yTranslator / yEditor through Windows UI Automation.

Every command writes its result to a JSON file (and screenshots to PNG) so the
caller never depends on console output.

Usage:
  py scripts/ui_driver.py launch <exe> [--wait N]
  py scripts/ui_driver.py tree   <title_regex> [--depth N]
  py scripts/ui_driver.py shot   <title_regex> <png_path>
  py scripts/ui_driver.py click  <title_regex> <control_title> [--type T]
  py scripts/ui_driver.py menu   <title_regex> "File->Open..."
  py scripts/ui_driver.py keys   <title_regex> <keys>
  py scripts/ui_driver.py close  <title_regex>
  py scripts/ui_driver.py texts  <title_regex> [max] [--type Edit]   (visible names/values, e.g. a log pane)
  py scripts/ui_driver.py wait   - <seconds>
  py scripts/ui_driver.py run    <steps.json>   (many steps, one process, one result file)

Results go to %TEMP%/ui_driver_result.json unless --out is given.
"""

import argparse
import json
import os
import sys
import time
import traceback

from PIL import ImageGrab
from pywinauto import Application, Desktop

result_path_default = os.path.join(os.environ.get("TEMP", "."), "ui_driver_result.json")


def write_result(path, payload):
    with open(path, "w", encoding="utf-8") as handle:
        json.dump(payload, handle, indent=2, ensure_ascii=False)


def find_window(title_regex, timeout_seconds=15):
    deadline = time.time() + timeout_seconds
    import re
    pattern = re.compile(title_regex)
    while time.time() < deadline:
        for window in Desktop(backend="uia").windows(title_re=title_regex, visible_only=True):
            return window
        # Modal dialogs (e.g. the native folder picker) are owned windows that UIA
        # exposes as children of their parent window, not as top-level windows.
        for top in Desktop(backend="uia").windows(visible_only=True):
            try:
                for child in top.children():
                    if child.element_info.control_type == "Window" and pattern.search(child.element_info.name or ""):
                        return child
            except Exception:
                continue
        time.sleep(0.5)
    raise RuntimeError(f"no visible window matching /{title_regex}/")


def describe(element):
    info = element.element_info
    rect = info.rectangle
    return {
        "title": info.name,
        "type": info.control_type,
        "auto_id": info.automation_id,
        "enabled": element.is_enabled(),
        "rect": [rect.left, rect.top, rect.right, rect.bottom],
    }


def dump_tree(element, depth, max_depth):
    node = describe(element)
    if depth < max_depth:
        node["children"] = [dump_tree(child, depth + 1, max_depth) for child in element.children()]
    return node


def command_launch(arguments):
    application = Application(backend="uia").start(arguments.target, work_dir=os.path.dirname(arguments.target))
    time.sleep(arguments.wait)
    windows = [describe(window) for window in application.windows()]
    return {"pid": application.process, "windows": windows}


def command_tree(arguments):
    window = find_window(arguments.target)
    return dump_tree(window, 0, arguments.depth)


def command_shot(arguments):
    window = find_window(arguments.target)
    window.set_focus()
    time.sleep(0.4)
    rect = window.rectangle()
    image = ImageGrab.grab(bbox=(rect.left, rect.top, rect.right, rect.bottom), all_screens=True)
    image.save(arguments.extra)
    return {"saved": arguments.extra, "size": list(image.size)}


def command_click(arguments):
    window = find_window(arguments.target)
    criteria = {"title": arguments.extra}
    if arguments.type:
        criteria["control_type"] = arguments.type
    matches = window.descendants(**criteria)
    if not matches:
        raise RuntimeError(f"no control titled '{arguments.extra}' (type={arguments.type})")
    control = matches[0]
    window.set_focus()
    time.sleep(0.3)
    selectable_types = ("TabItem", "ListItem", "TreeItem")
    invokable_types = ("Button", "MenuItem", "SplitButton")
    control_type = control.element_info.control_type
    if control_type in selectable_types and hasattr(control, "select"):
        control.select()
        method = "uia_select"
    elif control_type in invokable_types:
        try:
            control.invoke()
            method = "uia_invoke"
        except Exception:
            control.click_input()
            method = "mouse_click"
    elif control_type == "CheckBox":
        control.toggle()
        method = "uia_toggle"
    else:
        control.click_input()
        method = "mouse_click"
    time.sleep(0.6)
    report = {"clicked": describe(control), "method": method}
    if hasattr(control, "is_selected"):
        report["selected_after"] = bool(control.is_selected())
    return report


def normalize_label(label):
    return label.replace("&", "").split("\t")[0].strip().rstrip(".").strip().lower()


def visible_menu_items():
    items = []
    for top in Desktop(backend="uia").windows(visible_only=True):
        try:
            items.extend(top.descendants(control_type="MenuItem"))
        except Exception:
            continue
    return items


def click_menu_item(label, scope=None):
    wanted = normalize_label(label)
    candidates = scope.descendants(control_type="MenuItem") if scope is not None else visible_menu_items()
    for item in candidates:
        if normalize_label(item.element_info.name) == wanted:
            item.click_input()
            return item.element_info.name
    names = sorted({item.element_info.name for item in candidates if item.element_info.name})
    raise RuntimeError(f"menu item '{label}' not found; visible: {names}")


def command_menu(arguments):
    """Open a menu path like 'File->Load MO2 Profile' (Qt popups are separate windows)."""
    window = find_window(arguments.target)
    window.set_focus()
    parts = [part for part in arguments.extra.split("->") if part.strip()]
    clicked = [click_menu_item(parts[0], scope=window)]
    for part in parts[1:]:
        time.sleep(0.6)
        clicked.append(click_menu_item(part))
    time.sleep(0.8)
    return {"menu": clicked}


def command_popup_items(arguments):
    """After opening a menu, list every visible menu item label (to discover exact names)."""
    if arguments.target and arguments.target != "-":
        window = find_window(arguments.target)
        window.set_focus()
        click_menu_item(arguments.extra, scope=window)
        time.sleep(0.8)
    return {"items": [item.element_info.name for item in visible_menu_items() if item.element_info.name]}


def command_dialog_path(arguments):
    """Fill a native file/folder dialog: type a path into its edit field and confirm.

    target = dialog title regex, extra = path. For folder pickers the dialog first
    navigates into the folder, then the Select Folder button picks it.
    """
    dialog = find_window(arguments.target, timeout_seconds=20)
    dialog.set_focus()
    edits = dialog.descendants(control_type="Edit")
    if not edits:
        raise RuntimeError("dialog has no edit field")
    named = [edit for edit in edits if normalize_label(edit.element_info.name).startswith(("folder", "file name", "nazwa pliku"))]
    field = (named or edits)[-1]
    field.set_focus()
    field.set_edit_text(arguments.extra)
    time.sleep(0.3)
    field.type_keys("{ENTER}", set_foreground=True)
    time.sleep(1.2)
    confirm_labels = ("select folder", "wybierz folder", "open", "otwórz", "ok")
    pressed = None
    try:
        still_open = find_window(arguments.target, timeout_seconds=2)
        for button in still_open.descendants(control_type="Button"):
            if normalize_label(button.element_info.name) in confirm_labels and button.is_enabled():
                button.click_input()
                pressed = button.element_info.name
                break
    except RuntimeError:
        pressed = "(closed after Enter)"
    time.sleep(0.8)
    return {"path": arguments.extra, "field": field.element_info.name, "confirm": pressed}


def command_keys(arguments):
    window = find_window(arguments.target)
    window.set_focus()
    window.type_keys(arguments.extra, with_spaces=True, set_foreground=True)
    time.sleep(0.5)
    return {"keys": arguments.extra}


def command_close(arguments):
    window = find_window(arguments.target)
    window.close()
    return {"closed": arguments.target}


def command_wait(arguments):
    time.sleep(float(arguments.extra or arguments.wait))
    return {"waited": float(arguments.extra or arguments.wait)}


def command_texts(arguments):
    window = find_window(arguments.target)
    criteria = {"control_type": arguments.type} if arguments.type else {}
    texts = []
    for element in window.descendants(**criteria):
        name = element.element_info.name
        value = ""
        try:
            value = element.iface_value.CurrentValue
        except Exception:
            value = ""
        if name or value:
            texts.append({"type": element.element_info.control_type, "name": name, "value": value})
    return {"count": len(texts), "texts": texts[: int(arguments.extra or 400)]}


def command_windows(arguments):
    """List visible top-level windows and, one level down, any dialog-like children."""
    listing = []
    for top in Desktop(backend="uia").windows(visible_only=True):
        info = top.element_info
        entry = {"title": info.name, "class": info.class_name, "pid": info.process_id}
        try:
            entry["dialogs"] = [child.element_info.name for child in top.children()
                                if child.element_info.control_type == "Window"]
        except Exception:
            entry["dialogs"] = []
        listing.append(entry)
    return {"windows": listing}


def command_wait_text(arguments):
    """Poll the window's visible texts until one matches a regex. extra = regex; --wait = timeout seconds."""
    import re
    pattern = re.compile(arguments.extra)
    timeout_seconds = float(arguments.wait) if arguments.wait else 300.0
    started = time.time()
    while time.time() - started < timeout_seconds:
        try:
            window = find_window(arguments.target, timeout_seconds=5)
            for element in window.descendants():
                name = element.element_info.name or ""
                if pattern.search(name):
                    return {"matched": name, "seconds": round(time.time() - started, 1)}
                if element.element_info.control_type == "Edit":
                    try:
                        value = element.iface_value.CurrentValue or ""
                    except Exception:
                        value = ""
                    found = pattern.search(value)
                    if found:
                        return {"matched": found.group(0), "in": "edit_value", "seconds": round(time.time() - started, 1)}
        except Exception:
            pass
        time.sleep(2)
    raise RuntimeError(f"no text matching /{arguments.extra}/ within {timeout_seconds}s")


def command_run(arguments):
    """Run a JSON list of steps in one process: [{"cmd": "click", "target": ..., "extra": ..., "type": ...}, ...]."""
    with open(arguments.target, "r", encoding="utf-8") as handle:
        steps = json.load(handle)
    results = []
    for index, step in enumerate(steps):
        step_arguments = argparse.Namespace(
            command=step["cmd"], target=step.get("target"), extra=step.get("extra"),
            depth=step.get("depth", 4), wait=step.get("wait", 4.0), type=step.get("type"), out=None)
        try:
            outcome = {"ok": True, "result": commands[step["cmd"]](step_arguments)}
        except Exception as error:
            outcome = {"ok": False, "error": f"{type(error).__name__}: {error}"}
        outcome.update({"step": index, "cmd": step["cmd"]})
        results.append(outcome)
        if not outcome["ok"] and step.get("stop_on_error", True):
            break
    return {"steps": results}


commands = {
    "windows": command_windows,
    "wait_text": command_wait_text,
    "popup_items": command_popup_items,
    "dialog_path": command_dialog_path,
    "wait": command_wait,
    "texts": command_texts,
    "run": command_run,
    "launch": command_launch,
    "tree": command_tree,
    "shot": command_shot,
    "click": command_click,
    "menu": command_menu,
    "keys": command_keys,
    "close": command_close,
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=sorted(commands))
    parser.add_argument("target")
    parser.add_argument("extra", nargs="?")
    parser.add_argument("--depth", type=int, default=4)
    parser.add_argument("--wait", type=float, default=4.0)
    parser.add_argument("--type", default=None)
    parser.add_argument("--out", default=result_path_default)
    arguments = parser.parse_args()

    try:
        payload = {"ok": True, "result": commands[arguments.command](arguments)}
    except Exception as error:
        payload = {"ok": False, "error": str(error), "trace": traceback.format_exc()}

    write_result(arguments.out, payload)
    return 0 if payload["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
