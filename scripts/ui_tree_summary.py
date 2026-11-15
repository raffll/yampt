"""Summarize a ui_driver.py result file into a readable list of interactive controls.

Accepts the output of `ui_driver.py tree ...` or `ui_driver.py run ...` (uses the
first successful tree step). Writes the summary to --out (default: stdout).

Usage:
  py scripts/ui_tree_summary.py <result.json> [--out summary.txt] [--types MenuItem,Button,TabItem]
"""

import argparse
import json
import sys

default_types = ("MenuBar", "MenuItem", "ToolBar", "Button", "TabItem", "CheckBox", "ComboBox", "Edit")


def find_tree(payload):
    result = payload.get("result", {})
    if "steps" in result:
        for step in result["steps"]:
            if step.get("cmd") == "tree" and step.get("ok"):
                return step["result"]
        return None
    return result


def walk(node, depth, wanted, lines):
    title = node.get("title") or ""
    control_type = node.get("type")
    if control_type in wanted and (title or control_type == "Edit"):
        state = "" if node.get("enabled", True) else "  [disabled]"
        lines.append(f"{'  ' * depth}{control_type}: {title}{state}")
    for child in node.get("children", []):
        walk(child, depth + 1, wanted, lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("result_json")
    parser.add_argument("--out")
    parser.add_argument("--types", default=",".join(default_types))
    arguments = parser.parse_args()

    with open(arguments.result_json, "r", encoding="utf-8") as handle:
        payload = json.load(handle)
    tree = find_tree(payload)
    lines = []
    if tree is None:
        lines.append("no successful tree step: " + json.dumps(payload)[:800])
    else:
        walk(tree, 0, set(arguments.types.split(",")), lines)

    text = "\n".join(lines) + "\n"
    if arguments.out:
        with open(arguments.out, "w", encoding="utf-8") as handle:
            handle.write(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
