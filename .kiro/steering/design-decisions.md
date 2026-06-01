# Design Decisions — yampt

## DictMerger Uses First-Wins Semantics

`mergeDict` iterates dictionaries in order. When a key already exists in the merged dict with a different value, the **first** dictionary's value is kept. Later dictionaries do NOT overwrite.

The log message says "Warning: replaced ..." and the counter is named `counter_replaced`, but these are misleading — the value is NOT replaced. The warning means "a later dictionary tried to provide a different value for this key, but it was ignored."

The test `"DictMerger merge first-wins precedence"` confirms this: when path1 has "FirstValue" and path2 has "SecondValue" for the same key, the result is "FirstValue".

Do NOT add `search->second = elem.second;` to the "different value" branch. The original behavior is correct.

## XML Dictionary Format Is Frozen

The XML dictionary format is obsolete, poorly structured, and must NOT be "fixed" or improved. It exists solely for backward compatibility with existing dictionary files. Do not refactor the XML tag names, structure, nesting, or escaping scheme. Any changes to the XML format would break all existing dictionaries in the wild.

New features (like JSON export) are the path forward. The XML format stays as-is.

## No std::regex — Use find + Manual Checks

MSVC's `std::regex` implementation is orders of magnitude slower than `string::find`. Do NOT use `std::regex` anywhere in yampt. All pattern matching must use `find`-based approaches:

- Word boundaries: `find(keyword)` then check `isalnum`/`_` on adjacent chars
- Quoted strings: `find('"')` pairs
- Token extraction: character-class loops with `isalnum`, `_`, `.`, `-`

This applies to new code AND to fixes for existing code. When fixing a bug that currently uses regex, replace the regex with find-based logic — do not "improve" the regex.
