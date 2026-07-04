# Don't Delete Tests

Never delete a failing test without explicit user approval. If a test fails, diagnose the root cause and fix either the test data or the code under test. Do not remove the test to make the suite pass.

Never remove a test file or test entry from a vcxproj to fix build or linker errors. If a test doesn't compile or link, fix the underlying issue (add missing source files, fix includes, resolve MOC dependencies). Removing tests from the build is never an acceptable fix.

If a test approach turns out to be fragile or unreliable, report the finding and propose an alternative — do not silently remove or skip the test.

## Synthetic ESM Tests Are Valid

Synthetic ESM tests (hand-crafted binary records) are the correct approach for testing the converter. They test exactly the case we included — no more, no less. Real ESM files also don't cover all edge cases — we have to figure out edge cases ourselves by testing a lot of real plugins and then encoding each discovery as a synthetic test. When a synthetic test fails, fix the test data to match the actual binary format (check sub-record sizes, key formats like zero-padded INDX, null terminators). Do not dismiss synthetic tests as unreliable.

