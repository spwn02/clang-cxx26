# Does clangd survive a build rewriting its module files? (#487)

    repro.py <clangd> <clang++> [--in-place | --truncate] [--workaround]

Builds a module with 4000 functions, opens a file importing it in clangd, rewrites the
module file and asks for hovers.

| mode | clangd before the reader fix | clangd now |
| --- | --- | --- |
| `--in-place` (truncate + rewrite the same inode, what the old compiler did) | dies (SIGBUS/SIGSEGV) | dies unless `--copy` (`.clangd` CopyModuleFiles) |
| `--truncate` (module file left half written) | dies in `ASTIdentifierIterator::Next` | survives (#488) |
| `--workaround` (response file as CMake writes it + `.clangd` with `CompileFlags: Remove: "@*"`) | survives | survives |

Mapping is the default again (owner decision 2026-11-02): copying module files into memory
costs private memory, about twice the size of the imported module files per open file
(`rss.py`: 6 open files, 4.4 MB module: 257 MB vs 199 MB RssAnon). The fix is therefore
layered:

* the compiler writes module files atomically (`92cae3f49dd2` / `81edfe0243bb`): the primary fix;
* clangd copies module files into memory when the project asks for it, in `.clangd`:

      CompileFlags:
        CopyModuleFiles: true

  (`repro.py --in-place` kills clangd with SIGBUS by default, `repro.py --in-place --copy` survives);
* a load that fails on a half-written file no longer leaves a module without an identifier
  table behind (`ASTIdentifierIterator`, #488; `repro.py --truncate`);
* workaround without the option: `CompileFlags:\n  Remove: "@*"` in `.clangd` so clangd builds its
  own module files from the project's module units instead of reading the build directory's.
