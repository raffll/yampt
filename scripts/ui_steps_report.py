"""Print a compact per-step report for a `ui_driver.py run` result file.

Long results (tree dumps, text lists) are truncated; use --full to keep them.

Usage:
  py scripts/ui_steps_report.py <result.json> [--out report.txt] [--full] [--grep REGEX]
    --grep  for 'texts' steps, keep only texts whose name/value match REGEX
"""

import argparse
import json
import re
import sys


def describe_step(step, full, pattern):
    head = f"[{step['step']}] {step['cmd']}: {'OK' if step['ok'] else 'FAILED'}"
    if not step["ok"]:
        return [head, f"    error: {step.get('error')}"]
    result = step.get("result", {})
    if step["cmd"] == "texts":
        texts = result.get("texts", [])
        if pattern:
            texts = [text for text in texts if pattern.search(text.get("name", "")) or pattern.search(str(text.get("value", "")))]
        lines = [head + f" ({result.get('count')} texts, {len(texts)} shown)"]
        limit = None if full else 220
        for text in texts if full else texts[:80]:
            value = text.get("value") or ""
            lines.append(f"    {text['type']}: {text['name'][:limit]}" + (f" = {value[:limit]}" if value else ""))
        return lines
    rendered = json.dumps(result, ensure_ascii=False)
    return [head, "    " + (rendered if full else rendered[:400])]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("result_json")
    parser.add_argument("--out")
    parser.add_argument("--full", action="store_true")
    parser.add_argument("--grep")
    arguments = parser.parse_args()

    with open(arguments.result_json, "r", encoding="utf-8") as handle:
        payload = json.load(handle)
    pattern = re.compile(arguments.grep, re.IGNORECASE) if arguments.grep else None
    lines = []
    if not payload.get("ok"):
        lines.append("run failed: " + str(payload.get("error")))
    for step in payload.get("result", {}).get("steps", []):
        lines.extend(describe_step(step, arguments.full, pattern))
    text = "\n".join(lines) + "\n"
    if arguments.out:
        with open(arguments.out, "w", encoding="utf-8") as handle:
            handle.write(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
