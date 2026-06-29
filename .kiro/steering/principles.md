# Principles

## File Organization

- One file, one responsibility
- Max 1000 lines per file
- One class per file
- MVC pattern with files sorted in different folders
- Utility classes and functions separated from model classes
- Extract all types, enums, and lists of names into separate miscellaneous classes
- Extract all names and messages into YAML

## Function Design

- Max 50 lines per function
- Max 3 nesting levels per function
- Max 2 function arguments — if more are needed, create a struct
- Remove unused function arguments — do not leave unnamed or commented-out parameters
- Always do testable interfaces
- Unit test for every function

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
