"""Digest a yampt log: count lines per prefix and group [error]/[warning] messages.

Works on any text file containing yampt log lines (CLI output, a dumped GUI Log
tab, ui_steps_report output). Quoted names and numbers are masked so repeated
messages about different records collapse into one pattern with a count.

Usage:
  py scripts/log_digest.py <log.txt> [--out digest.txt] [--examples 3]
"""

import argparse
import collections
import re
import sys

prefix_pattern = re.compile(r"\[(error|warning|info|debug)\]")
message_pattern = re.compile(r"\[(error|warning)\]\s*(.*?)(?=\s*(?:\[\d\d:\d\d\]|\[(?:error|warning|info|debug)\]|$))")
quoted_pattern = re.compile(r'"[^"]*"')
number_pattern = re.compile(r"\b\d+\b")


def mask(message):
    return number_pattern.sub("N", quoted_pattern.sub('"…"', message)).strip()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("log_path")
    parser.add_argument("--out")
    parser.add_argument("--examples", type=int, default=3)
    arguments = parser.parse_args()

    with open(arguments.log_path, "r", encoding="utf-8", errors="replace") as handle:
        text = handle.read()

    prefix_counts = collections.Counter(match.group(1) for match in prefix_pattern.finditer(text))
    groups = collections.defaultdict(list)
    for match in message_pattern.finditer(text):
        level, message = match.group(1), match.group(2).strip()
        groups[(level, mask(message))].append(message)

    lines = ["prefix counts: " + ", ".join(f"{name}={count}" for name, count in sorted(prefix_counts.items()))]
    for level in ("error", "warning"):
        level_groups = sorted(((key, messages) for key, messages in groups.items() if key[0] == level),
                              key=lambda item: -len(item[1]))
        lines.append(f"\n{level.upper()} patterns: {len(level_groups)}")
        for (_, pattern), messages in level_groups:
            lines.append(f"  {len(messages):5d} x {pattern[:200]}")
            for example in messages[: arguments.examples]:
                lines.append(f"          e.g. {example[:220]}")

    output = "\n".join(lines) + "\n"
    if arguments.out:
        with open(arguments.out, "w", encoding="utf-8") as handle:
            handle.write(output)
    else:
        sys.stdout.write(output)
    return 0


if __name__ == "__main__":
    sys.exit(main())
