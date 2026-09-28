# zUtilities: Linux build with msvc-wine

This integration was generated from `zUtilities/zUtilities.vcxproj`. The original
Visual Studio solution/project remains usable and is not rewritten.

## Prerequisites

- mstorsjo/msvc-wine installed at `~/my_msvc/opt/msvc`;
- Union 1.0m SDK installed at `~/my_msvc/opt/Union/1.0m`;
- CMake 3.25+, Ninja, Wine, `clang-cl`, and the VS Code C/C++ extension and/or
  clangd extension.

Override the locations with `MSVC_WINE_ROOT` and `UNION_SDK_ROOT` CMake cache
variables if yours differ.

## Configure and build

```fish
cmake --preset MP-x4-MT-Release-msvc-wine
cmake --build --preset MP-x4-MT-Release-msvc-wine
```

The DLL appears below
`out/build/MP-x4-MT-Release-msvc-wine/bin/`. Available presets cover G1, G1A,
G2, G2A, MP x2, and MP x4 in Debug, MT Release, and MD Release variants.
Single-engine presets compile and link only that engine's generated
`ClassDeclarators`, `Globals`, and `Statics` sources. MP x2 uses G1 and G2A;
MP x4 uses all four engines. Release builds mirror the Visual Studio template's
frame-pointer and incremental LTCG settings.

## IntelliSense

Configure the real preset you want to edit or build. CMake makes the ignored
root `compile_commands.json` point to that preset's database for clangd and
updates the ignored `cmake/IntelliSenseActivePreset.h` symlink used by
Microsoft C/C++. Selecting G1, G1A, G2, G2A, MP x2, or MP x4—and Debug, MT
Release, or MD Release—therefore changes both editor engines without editing a
VS Code or clangd path.

In VS Code select `Union 1.0m msvc-wine x86` if the Microsoft C/C++ extension
asks for a configuration, then run `C/C++: Reset IntelliSense Database` after
changing presets. If using clangd, reload the VS Code window or restart the
clangd extension after changing presets.

clangd reads `compile_commands.json`. The Microsoft C/C++ extension
intentionally does not use that database: when `compileCommands` is present,
cpptools ignores its native `forcedInclude`, while it also does not reliably
honor MSVC `/FI` from an msvc-wine command. Instead, cpptools uses the
equivalent explicit include paths and forces the shipped
`cmake/IntelliSenseContext.h`; that header includes the generated active-preset
definitions.
The installer discovers the newest installed MSVC toolset and Windows 10 SDK
and adds their ATL/MFC, STL, `shared`, `ucrt`, `um`, and `winrt` headers
explicitly; `clang-cl` cannot discover those Wine-installed paths itself.

The compiler and IntelliSense use the corrected project-local `UnionAfx.h` and
ZenGin files directly. The stable `cmake/IntelliSenseContext.h` is forced only
for standalone `.cpp` fragments listed by `Sources.h`; it loads both
`Headers.h` and the ordered `Sources.h` aggregation so old templates can see
symbols defined in earlier fragments, and it derives its engine namespace from
the active preset's definitions. For MP x2/x4, G2A remains the representative
standalone-fragment context, while the actual DLL still builds every engine
selected by the preset.

## Project-specific libraries

The stock libraries are linked automatically. For an unusual plugin, configure
extra dependencies without editing the generic CMake file:

```fish
cmake --preset MP-x4-MT-Release-msvc-wine \
  -DUNION_EXTRA_LIBRARIES='libcurl_a;Crypt32;Normaliz' \
  -DUNION_EXTRA_INCLUDE_DIRECTORIES=/path/to/include \
  -DUNION_EXTRA_LIBRARY_DIRECTORIES=/path/to/lib
```

Windows paths from a `.vcxproj` cannot always be translated to Linux paths, so
third-party include/library directories are the one expected manual exception.

## Source control and SDK safety

Build products live below ignored `out/`; the generated root
`compile_commands.json` and `cmake/IntelliSenseActivePreset.h` symlinks are
ignored too. The CMake/tooling files and the case/separator corrections in the
project-local UnionAfx/ZenGin template are intentional repository changes. The
installed Union headers below `~/my_msvc/opt/Union/1.0m` are referenced
directly and are neither copied nor modified.
