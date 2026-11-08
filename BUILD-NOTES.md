# Build notes

Practical notes from building this plugin on Windows with a *prebuilt* CommonLibSSE-NG tree.

## Toolchain that works

| Item | Value |
|---|---|
| Visual Studio | 2026 Community, toolset v145 / MSVC 14.51 |
| CMake | 4.3.1 (bundled with VS: `Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`) |
| Generator | **Visual Studio 18 2026**, platform x64 |
| Standard | `stdcpplatest` (C++23 — CommonLibSSE-NG's exported target raises the requirement) |
| CRT | `MultiThreadedDLL` (`/MD`), matching the `x64-windows-static-md` triplet |
| CommonLibSSE-NG | prebuilt `x64-windows-static-md` tree: `include`, `lib`, `share\CommonLibSSE` |

`build.ps1` loads the MSVC environment with `vcvars64.bat`, prefers toolset `14.51.36231`, then
configures and builds. Pass `-Prefix` (or set `OBITUARY_COMMONLIBSSE`) to point at the CommonLibSSE
tree; otherwise it looks for `../vcpkg-install/x64-windows-static-md`.

## Pitfalls (all hit in practice)

1. **Do not use the Ninja generator.** On this machine it hangs inside `mspdbsrv` (pipe EOF never
   returns). Use the Visual Studio generator.
2. **Do not add `/utf-8`.** The build tree lives under a non-ASCII path; with `/utf-8` CMake-generated
   intermediates trip `C4828`. This project uses `/source-charset:utf-8` instead, which is enough to
   keep UTF-8 comments from mojibaking and does not change the execution charset (all narrow literals
   are ASCII).
3. **Do not use vcpkg manifest mode / CMakePresets.** It contacts the registry and rebuilds
   CommonLibSSE-NG from source. Consume the prebuilt tree directly.
4. **`Microsoft::DirectXTK` target.** CommonLibSSE-NG's export references this target but never pulls
   it in; `CMakeLists.txt` creates an `IMPORTED` target pointing at the same tree, otherwise CMake
   generation fails.
5. **Hard-coded openvr path.** The NG export references
   `D:\VS\vcpkg\buildtrees\commonlibsse-ng\src\<hash>\extern\openvr\lib\win64\openvr_api.lib`.
   Do not delete that `buildtrees` folder. A warning is emitted if it is missing.
6. **C++20 is not achievable.** NG's interface requires `cxx_std_23`; declare it explicitly.
7. **`build.ps1` is ASCII-only.** Windows PowerShell 5.1 reads `.ps1` as ANSI; non-ASCII comments
   would break it.
8. **PowerShell 5.1 `Set-Content -Encoding UTF8` writes a BOM.** Harmless for MSVC, but strip it for
   `.py`/`.json` files consumed by other tools.
9. **Version numbers live in two places**: `project(VERSION)` in `CMakeLists.txt` and `.Version` in
   `src/plugin.cpp`. Keep them in sync (also `mod/meta.ini`).
10. **Ninja/PowerShell sandboxes aside**, the plugin is a normal DLL: `SKSEPlugin_Load`,
    `SKSEPlugin_Query`, `SKSEPlugin_Version` are exported; no `.lib` is shipped.
11. **The SMF header is optional.** Without `external/SKSEMenuFramework.h` the build produces a
    menu-less DLL (about 725 KB instead of 754 KB); `src/Menu.cpp` guards the include with
    `__has_include`.
12. **Package layout.** `tools/package.ps1` builds, then copies `mod/` + the DLL + README/LOCALIZATION
    + `tools/localization.py` into `dist/ObituarySKSE/` and zips it.

## Verified builds

- Runtime target: **Skyrim SE/AE 1.7.104.0**, SKSE 2.3.1 (also whitelisted: 1.5.97, 1.6.1170).
- Clean build (empty `build-vs`) succeeds from scratch; unit behaviour verified in game for death
  capture, filters, log paging, message box and resurrection.
- **Not yet verified in game**: the *Teleport & Resurrect* arrival check, and the corner-notification
  method (`Debug.Notification` through the VM).