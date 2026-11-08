# external/

## `SKSEMenuFramework.h` (not shipped)

The header belongs to [SKSE Menu Framework](https://github.com/QTR-Modding/SKSE-Menu-Framework-3)
(**GPL-3.0**), so this MIT repository does not redistribute it.

To build with the F1 menu:

1. download `resources/SKSEMenuFramework.h` from the upstream repository;
2. save it as `external/SKSEMenuFramework.h`;
3. re-run `build.ps1`.

Without it the plugin still compiles: `src/Menu.cpp` guards the include with `__has_include` and
falls back to a menu-less build (settings then live only in `settings.ini`). The log then says:

```
ObituarySKSE: built without SKSEMenuFramework.h - F1 menu disabled
```

The header is header-only glue: it resolves the framework's exported functions with
`GetProcAddress`, so no `.lib` is needed. At runtime, if `SKSEMenuFramework.dll` is missing, the
plugin simply has no menu.