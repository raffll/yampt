# Cleanest Approach

Always use the cleanest approach. Prefer proper architecture over shortcuts — forward declarations in dedicated internal headers over extern hacks, proper file splits over stuffing related code into one file with comments, and real abstractions over type-punning tricks.

Never propose or describe a solution as "the easiest way." Every solution must be the correct way given the existing architecture.

## Fixing Compilation/Linker Errors

When fixing compile or linker errors, do NOT change the project architecture. Do NOT restructure include hierarchies, move code between projects, convert inline functions to out-of-line, or change how projects link against each other. The fix must preserve the existing structure — if a project compiles .cpp files directly from another project folder, the fix is to add the missing .cpp file to that project. If a header needs a forward declaration instead of a full include, use a forward declaration. Never "simplify" the build by changing how projects consume each other's code.


## Always the Cleanest Solution

Every implementation must be the cleanest possible solution. No workarounds, no "good enough" shortcuts, no incremental patches that leave architectural debt. If the correct solution requires changing an interface, change the interface. If it requires a new abstraction, create it. If a function needs a different signature, change the signature.

Never call the same function twice with different parameters to work around a limitation. Instead, fix the function to accept what it needs in one call.
