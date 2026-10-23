# Reference Docs

Steering files (`.kiro/steering/`) contain **rules** only — behavioral directives
for how to work. Everything else lives under `.kiro/docs/`, split by reader, and is
NOT auto-loaded. Consult the relevant doc before working in its area.

- `.kiro/docs/architecture/` — the big picture (for humans): how the apps fit together and why key decisions were made.
- `.kiro/docs/reference/` — precise agent reference: byte layouts, schemas, enums, tables, config formats. Consult these so you never guess or re-derive from code.

## Architecture

| When working on... | Read first |
|---|---|
| Project layout, build, project dependencies, include rules, file placement | `.kiro/docs/architecture/project-structure.md` |
| Merge precedence, dictionary design decisions, converter invariants, revert/merge-removal behavior | `.kiro/docs/architecture/design-decisions.md` |

## Reference

| When working on... | Read first |
|---|---|
| ESM/ESP binary format, sub-records, record layouts | `.kiro/docs/reference/esm-format.md` |
| Per-record sub-record field inventory / schemas | `.kiro/docs/reference/record-schemas.md` |
| Decode/merge/eligibility decision tables — which file owns which concern, field & accessor names | `.kiro/docs/reference/decision-tables.md` |
| 3-way merge truth table (every version combination, presence/absence) | `.kiro/docs/reference/merge-truth-table.md` |
| FRMR / placed-object references, RefNum identity, cell-ref merge | `.kiro/docs/reference/frmr-references.md` |
| Entry statuses and how they drive convert/create/make-base | `.kiro/docs/reference/statuses.md` |
| Supported languages, codepages, translation models | `.kiro/docs/reference/languages.md` |
| Web translation providers (JSON configs, template vars, system prompt) | `.kiro/docs/reference/translation-providers.md` |
| Building CTranslate2, downloading/fine-tuning models | `.kiro/docs/reference/translation-engine-build.md` |

Acknowledged tech debt / known violations (do not re-report in audits) live in
`TODO.md`'s "Tech Debt" section.
