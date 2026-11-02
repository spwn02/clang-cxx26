# Does clangd survive a build rewriting its module files? (#487)

    repro.py <clangd> <clang++> [--in-place | --truncate] [--workaround]

Builds a module with 4000 functions, opens a file importing it in clangd, rewrites the
module file and asks for hovers.

| mode | clangd before the reader fix | clangd now |
| --- | --- | --- |
| `--in-place` (truncate + rewrite the same inode, what the old compiler did) | dies (SIGBUS/SIGSEGV) | survives |
| `--truncate` (module file left half written) | dies in `ASTIdentifierIterator::Next` | still dies: the half file is read as a module (separate bug, #488) |
| `--workaround` (response file as CMake writes it + `.clangd` with `CompileFlags: Remove: "@*"`) | survives | survives |

The fix is two-sided: the compiler writes module files atomically (`92cae3f49dd2` /
`81edfe0243bb`), and clangd reads module files into memory instead of mapping them
(`HeaderSearchOptions::ReadModuleFilesIntoMemory`, set in `clangd/Compiler.cpp`).
The user-level workaround, for clangd versions without the reader fix: put
`CompileFlags:\n  Remove: "@*"` in `.clangd` so clangd builds its own module files from
the project's module units instead of reading the build directory's.
