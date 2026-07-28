# Not Implemented Yet

Do not include unfinished features in the README, CHANGELOG, or user manuals.

- **Lua Handlers tab** (yEditor) — Lua handler conflict detection. Core scanner and UI are implemented but the tab is hidden until the feature is validated.

- **Batch translation** (yTranslator) — `start_batch_translation` is wired as a callback but never triggered from the UI. The function exists in `dict_operations_controller_t` with a hardcoded 10-entry cap. Dead code until a trigger is added.
