# Known Issues

Tracked violations that are acknowledged but not yet fixed. Do NOT re-report these in audits.

## main_window.cpp exceeds 1000 lines

`yampt.translator/source/main_window.cpp` exceeds the 1000-line file limit. Table/filter orchestration logic should be extracted into a dedicated controller.

## main_window_setup.cpp connect functions exceed 50-line limit

`connect_menu_signals` (~210 lines), `connect_sidebar_signals` (~87 lines), and `connect_editor_signals` (~238 lines) all exceed the 50-line function limit. The lambda logic inside these functions should be moved into controller methods, leaving only thin `connect()` calls in the setup file.

## translate_all_requested violates Anti-Gravity Rule

The `translate_all_requested` signal handler in `main_window` contains orchestration logic (showing dialogs, running operations, updating multiple views). This should be extracted into a translation controller per the Main Window Anti-Gravity Rule.
