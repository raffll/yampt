"""Print a short summary of a check_merged_patch.py JSON report.

Usage:
  py scripts/e2e/summarize_check.py <report.json> [--examples 3] [--out summary.txt]
"""

import argparse
import json
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("report_json")
    parser.add_argument("--examples", type=int, default=3)
    parser.add_argument("--out")
    arguments = parser.parse_args()

    with open(arguments.report_json, "r", encoding="utf-8") as handle:
        report = json.load(handle)
    lines = [f"overall: {report['overall']}", f"unresolved plugins: {report['unresolved_plugins'][:10]}"]
    for name in ("header", "leveled", "dialogue_structure", "info_links"):
        section = report.get(name)
        if section is None:
            continue
        fields = {key: value for key, value in section.items() if key != "examples"}
        lines.append(f"{name}: {fields}")
        for example in section.get("examples", [])[: arguments.examples]:
            lines.append(f"    e.g. {example}")
    text = "\n".join(lines) + "\n"
    if arguments.out:
        with open(arguments.out, "w", encoding="utf-8") as handle:
            handle.write(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
