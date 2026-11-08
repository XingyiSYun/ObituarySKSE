# ObituarySKSE — behaviour specification

Reimplementation target: the original *Obituary* mod (`Obituary.esp` + three Papyrus scripts).
This document records the behaviour that was reproduced, the native mapping, and the deliberate
differences.

Source material (read-only): the original mod's
`scripts\source\_asd_obituaryquestscript.psc` (2179 lines, MCM + queues + log),
`_asd_obituaryaliasscript.psc` (271 lines, `OnActorKilled` filters),
`_asd_obituaryresukeepeffectscript.psc` (279 lines, keep-items resurrection),
`SKSE\Plugins\Obituary\Strings.json`.

## 1. Death capture and filtering

Event: the original used po3's `OnActorKilled(Actor victim, Actor killer)` on a reference alias.
This plugin subscribes to `RE::TESDeathEvent`.

Filters, in the original order:

| # | Condition | Result |
|---|---|---|
| 1 | `bModEnabled` | drop |
| 2 | victim is the player **and** `!bLogAllowPlayer` | drop |
| 3 | victim `IsDisabled()` | drop |
| 4 | victim `IsDeleted()` | drop |
| 5 | victim's ActorBase is in the blacklist | drop |

Collected per death:
- display name; empty → `sNoName`; otherwise `\n`/`\r` replaced with a space
- `unique` = `TESNPC` unique flag (ACBS bit 0x20)
- `creature` = race has the vanilla keyword **ActorTypeCreature (0x00013795)**
- `playerKill` = killer is the player

## 2. Notification filter → queue

`bNotificationEnabled` → `bNotifPlayerKill` (or not a player kill) → `bNotifUniqueOnly` (or unique) →
`bNotifCreatures` (or not a creature) → `GetRelationshipRank(player) >= threshold` → queue.

Threshold: the original stores a **menu index 0..8** (`iNotifMinRelIndex`) mapped as
`0=-4 Archnemesis … 4=0 Acquaintance … 8=+4 Lover`. The relationship value itself is read from the
running game (Papyrus `Actor.GetRelationshipRank`), so it includes save/script-modified state.

## 3. Log filter → entry

`bLogEnabled` → `bLogPlayerKill` → `bLogUniqueOnly` → `bLogCreatures` → entry.

Entry text: `"<name> (<MM/DD/YYYY HH:MM>)"`.
Details: eight lines built from the template `%1..%8` (name, sex, race, unique, master file, FormID,
killer, time). The FormID is stored as the **last 6 hex digits** (stable across load orders); the
details view prefixes the master's current compile index at display time (2 hex digits, `FE` for
light plugins). The text log keeps the 6-digit form.

## 4. Queue processing

The original set `bProcessing` and called `RegisterForSingleUpdate(2.0)`, then on `OnUpdate` processed
notifications first, then the log. This plugin keeps the same 2-second deferral (worker thread then a
main-thread task), so the message box never opens in the middle of the death event.

## 5. Notification display

`"%s has died."` with the display name. Method 0 = message box, method 1 = corner notification
(`Debug.Notification` through the VM) plus a message box when teleport/resurrect options are enabled.

Message box buttons are built from the settings: Nothing / Teleport / Resurrect /
**Teleport & Resurrect** (new). *Teleport & Resurrect* teleports, then polls (250 ms, up to 10 s) until
the player is in the same cell and within 300 units, then resurrects; on timeout it notifies and
aborts.

## 6. Resurrection

- Optional dragon-soul cost (`DragonSouls` actor value, `DamageActorValue`).
- "Keeps items": snapshot inventory, worn armour and both equipped hands, `Resurrect(resetInventory=false)`,
  then re-equip. Without it, `Resurrect(resetInventory=true)`.

## 7. Persistent data

Log entries and the blacklist live in the SKSE co-save (record `OBIT`, version 1). Settings live in
`Data\SKSE\Plugins\ObituarySKSE\settings.ini`; blacklist entries may also come from
`Blacklist.txt` next to it.

## 8. Differences from the original (intentional)

1. **No MCM.** MCM Helper's JSON only binds controls to setting values; `input` controls need a script
   variable (`_inputStartText`) and crash when the quest runs the precompiled base script. All UI is in
   the F1 menu.
2. **No `NoObituary` StorageUtil exemption** (that data lives in PapyrusUtil's storage); the blacklist
   replaces it.
3. **No history migration** from the original's cosave.
4. **Teleport & Resurrect** is new — the original only had Teleport and Resurrect separately.