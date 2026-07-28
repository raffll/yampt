# GUI Compact Tables

Every `QTableWidget` in settings dialogs must set compact row height:

```cpp
table->verticalHeader()->setDefaultSectionSize(24);
```

This applies to both yampt.translator and yampt.editor settings pages. The default Qt row height is too tall for dense data tables like provider lists and shortcut tables.
