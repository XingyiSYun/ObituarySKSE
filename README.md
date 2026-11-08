# ObituarySKSE

A native **SKSE plugin** reimplementation of *Obituary* (NPC death log & notifications) for
*The Elder Scrolls V: Skyrim Special Edition / Anniversary Edition*.

The original mod is pure Papyrus. This project runs entirely in native code — death capture,
filtering, the death log, notifications, teleport and resurrection — so it does not depend on the
Papyrus VM. All UI lives in a single **F1 menu** (SKSE Menu Framework); there is no MCM.

**This is v1.0.0, the first release.** 
## Features

- **Death capture** via `TESDeathEvent` — no Papyrus Extender required
- **Two independent filter chains** (log and notification): unique only / creatures / player kills /
  allow player / minimum relationship
- **Death log**: ring buffer (1024 entries by default), optional `txt` output; each entry stores name,
  time, sex, race, unique flag, master file, FormID, killer
- **Notifications**: corner notification or message box. The message box buttons are built from your
  settings:
  - Leave / Teleport / Resurrect / **Teleport & Resurrect**
  - *Teleport & Resurrect* is new in this plugin: it teleports first, waits until the player is in the
    same cell and within 300 units of the target, then resurrects. Gives up after 10 seconds with a
    notification.
  - Resurrection honours dragon-soul cost and "keeps items" (inventory, worn armour, both hands)
- **Blacklist**: `Blacklist.txt` (one FormID / EditorID per line) or the F1 menu (crosshair target)
- **Single menu (F1)**: Settings / Log (paging, details, clear) / Blacklist
- **Persistence**: log and blacklist are written to the SKSE co-save (`OBIT` v1)

## Requirements

| Dependency | Link | Notes |
|---|---|---|
| Skyrim SE/AE 1.5.97 / 1.6.1170 / 1.7.104 | — | three runtimes are whitelisted in the plugin declaration |
| SKSE64 | <https://www.nexusmods.com/skyrimspecialedition/mods/30379> | match your runtime version |
| Address Library for SKSE Plugins | <https://www.nexusmods.com/skyrimspecialedition/mods/32444> | required |
| SKSE Menu Framework 3.x | <https://www.nexusmods.com/skyrimspecialedition/mods/120352> | 

No MCM Helper / SkyUI.

## Installation

1. Install the `ObituarySKSE` folder as a mod (or install the release zip).
2. This plugin is **fully standalone** — you do *not* need the original Obituary mod . If you already have it installed, disable its esp/scripts: running both would
   log and notify twice for the same death.
3. In game press **F1** → "Obituary".

## Configuration

| Path | Contents |
|---|---|
| `Data\SKSE\Plugins\ObituarySKSE\settings.ini` | all settings (written by the F1 menu) |
| `Data\SKSE\Plugins\ObituarySKSE\Blacklist.txt` | blacklist: `Nazeem` / `0001A67C` / `0x0001A67C`, `#` comments |
| `Data\SKSE\Plugins\ObituarySKSE\Strings.json` | all UI text (Simplified Chinese by default) |
| `Data\SKSE\Plugins\Obituary\Logs\<player>\GameHour*.txt` | death log when "Write to File" is on |

## Localization

1. Edit the values in `SKSE\Plugins\ObituarySKSE\Strings.json` (do not rename keys; keep `%s`, `%d`
   and `\n`).
2. `Strings.en.json` is the English reference; `LOCALIZATION.md` is the full key table.
3. `tools/localization.py` regenerates `Strings.json` / `Strings.en.json` / `LOCALIZATION.md`.

## Building

Requirements: Visual Studio 2022/2026 (MSVC), CMake 3.21+, and a prebuilt
[CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) `x64-windows-static-md` tree
(with `include`, `lib`, `share\CommonLibSSE`).

**The F1 menu is optional.** This repository does not ship the third-party
`SKSEMenuFramework.h`. To build with the menu, fetch it as described in
[external/README.md](external/README.md); without it the plugin still compiles (smaller DLL, no F1
menu, settings editable through `settings.ini`).

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 `
  -Prefix "C:\path\to\vcpkg-install\x64-windows-static-md"

# or
$env:OBITUARY_COMMONLIBSSE = "C:\path\to\vcpkg-install\x64-windows-static-md"
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

Output: `build-vs\Release\ObituarySKSE.dll`.

Build and package (assembles `dist\ObituarySKSE\` and a zip):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\package.ps1 -Prefix "<CommonLibSSE tree>"
```

Known build pitfalls (details in [BUILD-NOTES.md](BUILD-NOTES.md)): do not use the Ninja generator,
do not add `/utf-8`, and do not use CMakePresets / vcpkg manifest mode (it goes online).

## Repository layout

```
src\                 plugin sources (entry / events and storage / notifications / menu / settings / localization)
tools\               localization.py, package.ps1
external\            third-party headers (not shipped — see external\README.md)
docs\                behavior specification (English) and docs\zh-CN\ (Chinese)
mod\                 runtime files: Strings, Blacklist.txt, meta.ini (the DLL is built)
```

## Known limitations

1. **No MCM.** The original SkyUI menu is replaced by the F1 menu (settings, log browsing, blacklist).
2. **Relationship rank comes from the running game** (Papyrus `Actor.GetRelationshipRank`), so it
   includes save/modified state. This matches the original mod; NPCs without a relationship record
   return 0 (acquaintance).
3. **No data migration.** The original mod's Papyrus log stays in its own co-save; this plugin starts
   with an empty log.
4. **PapyrusUtil's `NoObituary` exemption is not read** (it lives in Papyrus storage); use the
   blacklist instead.

## Credits

- **asdasfa** — author of the original Obituary mod; the behaviour specification, message wording and
  default values are derived from its Papyrus sources
- [SKSE Menu Framework](https://github.com/QTR-Modding/SKSE-Menu-Framework-3) by QTR-Modding (GPL-3.0)
  — F1 menu; its header is **not** redistributed here
- [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) — SKSE plugin framework

## License

[MIT](LICENSE). Third-party components and text are covered by their own licenses — see
[CREDITS.md](CREDITS.md).