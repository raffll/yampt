# Principles

## File Organization

- One file, one responsibility
- Max 1000 lines per file
- One class per file
- MVC pattern with files sorted in different folders
- Utility classes and functions separated from model classes
- Extract all types, enums, and lists of names into separate miscellaneous classes
- Extract all names and messages into YAML
- When splitting a large file, always extract into a new class with its own .hpp/.cpp pair — never split a single class across multiple .cpp files

Exceptions to the one-class-one-file rule:
- **Qt main windows** — a `_setup.cpp` companion file is acceptable for UI construction boilerplate (setup_*, connect_* methods). The class has one responsibility but Qt widget creation is verbose.
- **Qt tree models with complex decode** — a single model class may exceed 1000 lines if the decode logic is inseparable from the model's presentation role and has no external consumers.

## Main Window Anti-Gravity Rule

Never add new logic directly to `main_window_t` or `editor_window_t`. These classes are signal routers — they connect UI events to controllers. New features go on the appropriate controller or a new controller. The main window only:
- Constructs views and controllers
- Connects signals to controller methods
- Reads controller results to update views

If a new feature needs orchestration (showing dialogs, running operations, updating multiple views), create or extend a controller for it. The main window calls one method on the controller — it does not contain the logic itself.

## Function Design

- Max 50 lines per function
- Max 3 nesting levels per function
- Max 2 function arguments — if more are needed, create a struct
- Remove unused function arguments — do not leave unnamed or commented-out parameters
- Always do testable interfaces
- Unit test for every function
- Every algorithm must be callable from a unit test without instantiating heavy dependencies (no file I/O, no UI, no loaded plugins). Extract pure logic into public static methods on a dedicated class (e.g. `sub_record_merge_t::merge()`, `leveled_list_merge_t::merge()`) that accept data via parameters and return results. Class methods that need infrastructure (loading files, accessing plugin arrays) are thin wrappers that collect data and delegate to the testable class.

## Naming

- No name shorter than 5 characters — no abbreviations
- Names must always match what the variable actually is; if reused, generalize the name
- Do not use meaningless words like `data`, `manager`, `provider`
- Use one word for one meaning — e.g. if "model" refers to both MVC and AI, explicitly name them differently (e.g. `view_model` vs `translation_model`)
- Design pattern names should be explicitly named as patterns (e.g. `_factory`, `_observer`)
- Always name `first`/`second` explicitly via structured binding or `const auto &`
- Use `m_` prefix for class member variables (not for POD/helper struct fields — those use plain snake_case)
- Use `ptr_` prefix for pointers
- Use `it_` prefix for iterators
- All names must be meaningful — do not hesitate to create very long names

## Types and Safety

- Do not mix types — e.g. enum with int, or string disguised as one type
- Always use smart pointers instead of raw pointers
- Use `constexpr` whenever possible
- Use `string_view` where appropriate
- Always create `const` ref or `const` variable unless mutation is necessary
- Do not use magic numbers
- Do not use magic offsets
- Do not use pointer arithmetic without explicitly stating that you are doing so

## Code Quality

- SOLID, KISS, DRY principles
- Clean code
- Do not cut corners or do things the easiest way
- Every line of code must be understandable without reading the entire function
- Modernize — use C++20
- No comments unless the code cannot be explained by function or variable names alone. If a comment is unavoidable, keep it to one short line.
- No decorative comment banners (dashed lines, boxes, ASCII art).
- Always remove items from TODO.md that are done or cancelled — never leave stale entries.
- One class = one `.hpp` + one `.cpp`. Never split a class across multiple `.cpp` files. If a class exceeds 1000 lines, it has more than one responsibility — extract a new class, don't add a second `.cpp`.

## Localization

All user-visible strings in yampt.translator and yampt.editor must be wrapped for Qt translation:

- QWidget subclasses: `tr("text")`
- Non-QObject classes: `QCoreApplication::translate("yTranslator", "text")` or `"yEditor"`
- Never leave raw string literals in UI code (menus, tooltips, labels, messages, dialog titles, button text)
- Log messages (`app_logger_t`, `append_log`) stay English — they are developer-facing
- Shortcut key sequences (`"Ctrl+S"`, `"F10"`) are not translated
- Settings keys and internal identifiers are not translated

## Classes vs Namespaces

- **Class (`_t` suffix)** — has mutable state (member variables, including static mutable). Gets instantiated or manages a resource.
- **Namespace (`snake_case`)** — groups related functions with no shared mutable state. Constants (including `extern const`) are fine.
- **Struct (`_t` suffix)** — POD/data carriers with no behavior beyond trivial accessors.

Decision:
1. Does it hold mutable state? → class
2. Does it get instantiated? → class
3. Is it just functions + constants? → namespace

Never use a class with only static methods — use a namespace instead.

## Function Placement

- File-local `static` functions in `.cpp` — implementation-only helpers that serve one file. Internal types used by these helpers also live in the `.cpp`.
- Namespace functions in `.hpp` — shared logic callable from multiple files. Declared in a namespace in the header, defined in the `.cpp`.
- Class methods — behavior that operates on the class's own state.

Keep headers minimal. Only declare what external callers or tests need.

## Coding Standards Enforcement

Every code change must comply with ALL rules in the steering files. Before writing any code, verify:
- Max 50 lines per function
- Max 3 nesting levels
- Max 2 function arguments (use struct if more needed)
- File-local `static` for internal helpers, namespace functions for shared logic
- One class = one `.hpp` + one `.cpp` — never split across multiple `.cpp` files
- No comments, no magic numbers, no abbreviations under 5 characters
- `const auto &` by default
- Early returns to flatten logic
- Blank line after `continue`, `return`, `break` (unless last in block)

Never produce code that violates these rules even partially. If a change would exceed limits, split first, then implement.


## Always Ask Before Implementing

Never start implementing a solution without asking the user first. Propose the approach, explain it briefly, and wait for explicit approval before writing any code. This applies to every change — no exceptions.


## Changelog and README Rules

- Never include unit tests, test files, or test-related changes in the CHANGELOG or README.
- Never include scripts (PowerShell, Python, automation) in the CHANGELOG or README.
- Never include build system changes (vcxproj, paths, MSBuild targets) in the CHANGELOG or README.
- Only user-visible features, fixes, and behavioral changes belong in the CHANGELOG.
- Never include fixes for regressions introduced in the same release. If a refactor broke something and we fixed it before shipping, neither the break nor the fix appears in the CHANGELOG.
- The README describes what the application does for end users — not internal tooling.
