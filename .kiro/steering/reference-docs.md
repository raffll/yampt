# Reference Docs

Steering files (`.kiro/steering/`) contain **rules** only — behavioral directives
for how to work. Domain knowledge, file-format specs, architecture, and codebase
analysis live in `.kiro/docs/` and are NOT auto-loaded. Consult the relevant doc
before working in its area.

| When working on... | Read first |
|---|---|
| Project layout, build, project dependencies, include rules, file placement in the solution | `.kiro/docs/project-structure.md` |
| ESM/ESP binary format, sub-records, record layouts | `.kiro/docs/esm-format.md` |
| FRMR / placed-object references, RefNum identity, cell-ref merge | `.kiro/docs/frmr-reference-model.md` |
| Merge precedence, dictionary design decisions, converter invariants, revert/merge-removal behavior | `.kiro/docs/design-decisions.md` |
| 3-way merge truth table (every version combination, presence/absence) | `.kiro/docs/merge-truth-table.md` |
| Sub-record schema spec / per-parent field layouts | `.kiro/docs/schema-spec-notes.md` |
| Validation error feedback differences between yTranslator and yEditor | `.kiro/docs/validation-gui-differences.md` |
| Entry statuses and how they drive convert/create/make-base | `.kiro/docs/status-definitions.md` |
| Supported languages, codepages, translation models | `.kiro/docs/supported-languages.md` |
| Web translation providers (JSON configs, template vars, system prompt) | `.kiro/docs/web-translator-architecture.md` |
| Building CTranslate2, downloading/fine-tuning models | `.kiro/docs/translation-engine-build.md` |
| Acknowledged tech debt / known violations (do not re-report) | `.kiro/docs/known-issues.md` |

Before auditing code for steering violations, check `.kiro/docs/known-issues.md` —
those are tracked and must not be re-reported.
