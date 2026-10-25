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
Debug builds keep runtime checks enabled without release-only `/GL`, define
`_DEBUG`, and use the matching debug DLL runtime (`/MDd`) in both CMake and
Visual Studio. A manually configured Debug build with `UNION_RUNTIME=MT` uses
the debug static runtime (`/MTd`).

## IntelliSense

Configure the real preset you want to edit or build. CMake makes the ignored
root `compile_commands.json` point to that preset's database for clangd and
updates the ignored `cmake/IntelliSenseActivePreset.h` and
`cmake/IntelliSenseActiveProperties.json` symlinks used by Microsoft C/C++.
Selecting G1, G1A, G2, G2A, MP x2, or MP x4—and Debug, MT
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
definitions. CMake generates these paths from the configured project and Union
SDK directories, `UNION_EXTRA_INCLUDE_DIRECTORIES`, and the toolchain's discovered
MSVC and Windows SDK headers. This includes their ATL/MFC, STL, `shared`, `ucrt`,
`um`, and `winrt` directories; `clang-cl` cannot discover those Wine-installed
paths itself. Alternate SDK roots and newly installed toolset versions therefore
apply to both the build and Microsoft IntelliSense after configuring again.

The tracked `.vscode/c_cpp_properties.json` is a relative symlink to
`cmake/IntelliSenseActiveProperties.json`; the ignored second link selects the
generated JSON under the active build directory. The configuration template is
`cmake/IntelliSenseProperties.json.in`. Configure once after checking out these
files. If you customized the previous JSON, preserve those changes in the template
before updating; CMake never overwrites a regular file at the generated-link path.

The compiler and IntelliSense use the corrected project-local `UnionAfx.h` and
ZenGin files directly. The stable `cmake/IntelliSenseContext.h` is forced only
for standalone `.cpp` fragments listed by `Sources.h`; it loads the engine
context and `Headers.h`, including shared declarations in `Plugin.h`, and
derives its engine namespace from the active preset's definitions. For MP
x2/x4, G2A remains the representative
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
`compile_commands.json`, `cmake/IntelliSenseActivePreset.h`, and
`cmake/IntelliSenseActiveProperties.json` symlinks are ignored too. The relative
`.vscode/c_cpp_properties.json` link and its template are tracked, so configuring
another preset does not change tracked editor files. The CMake/tooling files and
the case/separator corrections in the
project-local UnionAfx/ZenGin template are intentional repository changes. The
installed Union headers below `~/my_msvc/opt/Union/1.0m` are referenced
directly and are neither copied nor modified.

Run the native editor configuration checks without Wine or an installed SDK
(requires CMake):

```sh
cmake -P cmake/tests/IntelliSensePropertiesTest.cmake
cmake -P cmake/tests/VisualStudioConfigurationTest.cmake
```
