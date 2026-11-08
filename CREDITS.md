# Third-party components and provenance

This project is an **independent reimplementation**. It contains none of the original Obituary's
Papyrus scripts or esp records; the behaviour was derived from reading the original `.psc` sources,
and the UI string keys follow the original `Strings.json` so that existing translations can be reused.

| Component | Location | Origin | License |
|---|---|---|---|
| Obituary behaviour spec | `docs/behavior-spec.md`, `docs/zh-CN/` | written for this project after reading the original `_asd_obituary*.psc` | this project (MIT) |
| `Strings.json` / `Strings.en.json` | `mod/SKSE/Plugins/ObituarySKSE/` | key names follow the original Obituary (author **asdasfa**); Chinese values come from that mod's Simplified Chinese strings, English values from the original script defaults | text belongs to the original author; no code copied |
| SKSE Menu Framework header | *not shipped* — downloaded by the user | [QTR-Modding/SKSE-Menu-Framework-3](https://github.com/QTR-Modding/SKSE-Menu-Framework-3) `resources/SKSEMenuFramework.h` | **GPL-3.0** — deliberately not redistributed in this MIT repository |
| CommonLibSSE-NG | build dependency, not shipped | [CharmedBaryon/CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) | Apache-2.0 |
| SKSE64 | runtime dependency, not shipped | <https://skse.silverlock.org/> | see skse.silverlock.org |
| Address Library for SKSE Plugins | runtime dependency, not shipped | <https://www.nexusmods.com/skyrimspecialedition/mods/32444> | see mod page |

## About the original mod

- Original **Obituary** by **asdasfa**. This project is not a fork and contains no esp or pex from it.
- If you redistribute `Strings.json`, credit the original author for the text.

## About `SKSEMenuFramework.h`

The header is part of a **GPL-3.0** project. Mixing it into an MIT repository would create a license
conflict, so it is intentionally excluded:

1. fetch `resources/SKSEMenuFramework.h` from the upstream repository;
2. place it in `external/SKSEMenuFramework.h`;
3. rebuild — `src/Menu.cpp` picks it up via `__has_include`.

Without the header the plugin builds a menu-less DLL (see [external/README.md](external/README.md)).