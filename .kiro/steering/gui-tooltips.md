# GUI Tooltips

Every new QPushButton, QToolButton, or QAction that is visible in the UI must have a `setToolTip(...)` call immediately after creation. No exceptions.

Tooltips should be short (under 60 characters), descriptive, and written in sentence fragment style without a trailing period.
