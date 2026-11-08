# Obituary（讣告）原生重写：行为规格与设计

> 目标：用 SKSE（C++ / CommonLibSSE-NG）重写 `【功能】讣告Obituary - NPC Death Log and Notification`
> 的可观察功能，产出**独立 mod**（不改原 mod、不重编译 Papyrus）。菜单：保留 MCM（MCM Helper JSON 实现），
> 另加 SKSE Menu Framework 菜单。运行时：Skyrim SE 1.7.104.0 / SKSE 2.3.1。

原版来源（只读）：
`E:\skyrim\Ae mod\mods\【功能】讣告Obituary - NPC Death Log and Notification\`
- `Obituary.esp`（3.2 KB）
- `scripts\source\_asd_obituaryquestscript.psc`（2179 行，SKI_ConfigBase MCM + 队列/日志/通知）
- `scripts\source\_asd_obituaryaliasscript.psc`（271 行，po3 OnActorKilled 过滤器）
- `scripts\source\_asd_obituaryresukeepeffectscript.psc`（279 行，保物品复活）
- `SKSE\Plugins\Obituary\Strings.json`（已有简体中文译文）

---

## 1. 行为规格（以源码为准）

### 1.1 死亡捕获

事件源：po3 Papyrus Extender 的 `OnActorKilled(Actor akVictim, Actor akKiller)`（ReferenceAlias 上注册）。

逐层过滤（顺序与原版一致）：

| 步骤 | 条件 | 不满足时 |
|---|---|---|
| 1 | `bModEnabled` | 丢弃 |
| 2 | `akVictim != PlayerRef` **或** `bLogAllowPlayer` | 丢弃 |
| 3 | `!akVictim.IsDisabled()` | 丢弃 |
| 4 | `!akVictim.IsDeleted()` | 丢弃 |
| 5 | `!BlacklistList.HasForm(akVictim.GetActorBase())` | 丢弃 |

通过后采集（仅在通知或日志任一启用时才算）：
- `sVictim = akVictim.GetDisplayName()`；空则 `sNoName`；非空则 `SanitizeName()`（`\n` → 空格）
- `bIsUnique = GetLeveledActorBase().IsUnique()`
- `bIsCreature = (akVictim.GetRace() as Form).HasKeyword(CreatureRaceKeyword)`
- `bIsKilledByPlayer = (akKiller == PlayerRef)`

### 1.2 通知过滤 → 入队 `QueueNotification(actor)`

依次判断：`bNotificationEnabled` → `bNotifPlayerKill` 或非玩家击杀 → `bNotifUniqueOnly` 或 unique →
`bNotifCreatures` 或非生物 → `GetRelationshipRank(PlayerRef) >= 关系表[iNotifMinRelIndex]` →
`StorageUtil.GetIntValue(victim, "NoObituary", 0) <= 0`（外部豁免）。

关系表下标：0=-4 宿敌, 1=-3 敌人, 2=-2 仇敌, 3=-1 对手, 4=0 相识, 5=1 朋友, 6=2 知己, 7=3 盟友, 8=4 爱人。

### 1.3 日志过滤 → 入队 `QueueEntry(entry, details)`

依次判断：`bLogEnabled` → `bLogPlayerKill` 或非玩家击杀 → `bLogUniqueOnly` 或 unique →
`bLogCreatures` 或非生物 → `StorageUtil "NoObituary" <= 0`。

条目文本：
- `sDeathTime` = `MM/DD/YYYY HH:MM`
  - 月 = `GameMonth + 1` 两位补零；日 = `GameDay` 两位；年 = `GameYear` 四位
  - 时:分 由 `GameHour`（浮点）拆出，均两位补零
- `entry` = `"<名字> (<死亡时间>)"`
- `details` = 8 行占位模板（占位符 `%1..%8`，显示时替换为八种标签）：
  ```
  %1 <名字>
  %2 <性别>
  %3 <种族>
  %4 <是否特殊>
  %5 <主文件>
  %6 <FormID>
  %7 <击杀者>
  %8 <死亡时间>
  ```
- 种族使用硬编码首字母大小写表（Argonian / Breton / Dark Elf / High Elf / Imperial / Khajiit / Nord /
  Orc / Redguard / Wood Elf，**末尾带空格**），因为游戏字符串缓存会把名字改回小写。
- 性别：`GetSex()==0` → 男，`==1` → 女
- `FormID`：`GetBaseObject().GetFormID()` → 8 位十六进制 → 去掉前两位 → 存 6 位；显示时再按主文件索引
  补回 2 位（非 light 插件用 `Game.GetModName(index)` 找到索引；找不到则用 `FE`），去前导零
- 主文件：`PO3_SKSEFunctions.GetFormModName(base, false)`

### 1.4 队列处理（延迟）

入队后若 `!bProcessing`：`bProcessing = true; RegisterForSingleUpdate(2.0)`。
`OnUpdate`：先处理通知队列，再处理日志队列，最后 `bProcessing = false`。

进入 MCM 时若 `bProcessing`，Settings/Log 页只显示「功能仍在处理中 / 请关闭所有菜单」。

### 1.5 通知发送

对队列里每个 actor：
1. `msg = StringReplace(sHasDied, "%s", 名字)`（`"%s 已经死亡。"`）
2. 把 `MessageHolder` 的 base object 名字设为 `msg`（消息框正文来自这个 Message 记录）
3. 方式 `iNotifMethodIndex`：
   - `0` 消息框 → `AlertMessageBox.Show()`
   - `1` 通知 → `Debug.Notification(msg)`；若 `bNotifTeleportOption || bNotifResurrectOption`，
     **额外**再弹一次消息框
4. 每条之间 `Wait(0.15)`；清空通知队列

消息框选项（index）：
- `0` 什么都不做
- `1` 传送：`PlayerRef.MoveTo(actor)`，等 2 秒
- `2` 复活：
  - 龙魂成本 `iNotifResurrectDragonSoulCost > 0`：读玩家 `DragonSouls` AV；够则扣（`DamageActorValue`）并可复活，
    不够则通知 `sNotEnoughDragonSouls`
  - 可复活时：若 `bNotifResurrectedKeepsItems && !生物` → 给 `ObituaryResuKeepSpell`（由法术效果脚本保物品复活）；
    否则直接 `Resurrect()` 并通知 `sHasBeenResurrected`

### 1.6 保物品复活（ResuKeep 效果）

`OnEffectStart`：等待 1 秒 → 在 `ObituaryCellCenterMarker` 位置 `PlaceAtMe(ObituaryResuContainer)` →
记录 13 个命名槽位 + 15 个未命名槽位 + 盾/左右手武器 → `RemoveAllItems(容器)` → `Resurrect()` →
清空复活后新刷出的物品 → 把容器物品还回 → 逐槽位重新装备 → 禁用+删除容器 → 通知 `sHasBeenResurrected` →
移除该法术。槽位掩码见源码（0x1 头 … 0x40000000 槽 60）。

### 1.7 日志存储与文件

- 内存环形缓冲：`EntryArray` / `DetailsArray`，定长 `iMaxLogSize`（默认 1024），满时整体左移腾位
- 可写文件时：`Data/SKSE/Plugins/Obituary/Logs/<玩家名>/GameHour<N>.txt`，`N = int(Game.GetRealHoursPassed())`；
  每条追加：
  ```
  -
  <entry>
  <details（%1..%8 已替换成标签）>
  ```
- 清除日志：清空两个数组

### 1.8 MCM 结构

两页：`设置` / `日志`。

设置页：模组启用（toggle）、恢复默认（text）、
日志组：启用 / 仅特殊 / 生物 / 玩家击杀 / 允许玩家 / 写入文件（6 toggle）+ 最大日志大小（slider）+ 清除日志（text）、
通知组：启用 / 仅特殊 / 生物 / 玩家击杀（4 toggle）+ 最低关系（menu 9 项）+ 方式（menu 2 项）+
传送选项 / 复活选项 / 复活保留物品（3 toggle）+ 复活龙魂消耗（slider）、
黑名单组：当前目标名、是否已列入、加入 / 移出 / 清空。

日志页：每页条目数（slider）、`总条目：x/max`、`页：x/y`、上一页 / 下一页 / 第一页 / 下 10 页、
条目列表（默认每页 16，最新在最上），点条目弹详情框（FormID 按主文件索引重算）。

### 1.9 默认值

| 键 | 默认 | 键 | 默认 |
|---|---|---|---|
| bModEnabled | true | bNotificationEnabled | false |
| bLogEnabled | true | bNotifUniqueOnly | true |
| bLogUniqueOnly | true | bNotifCreatures | true |
| bLogCreatures | true | bNotifPlayerKill | false |
| bLogPlayerKill | true | iNotifMinRelIndex | 4（0 相识） |
| bLogAllowPlayer | false | iNotifMethodIndex | 0（消息框） |
| bLogWriteToFile | false | bNotifTeleportOption | false |
| iMaxLogSize | 1024 | bNotifResurrectOption | false |
| iMaxEntriesPerPage | 16 | bNotifResurrectedKeepsItems | true |
|  |  | iNotifResurrectDragonSoulCost | 0 |

---

## 2. 原生映射设计

| 原版 | 新实现 |
|---|---|
| po3 `OnActorKilled` | `RE::TESDeathEvent` 事件源 sink（`ScriptEventSourceHolder`） |
| 队列 + `RegisterForSingleUpdate(2.0)` | 原生 tick（`BSTEventSink<RE::TESDeathEvent>` 后置队列 + 帧/计时器处理；不需要 Papyrus VM） |
| `Debug.Notification` | 引擎通知（VM 静态调用 `Debug.Notification` 或地址调用；备选：只弹消息框） |
| `AlertMessageBox.Show()` + Message 记录 | `RE::MessageBoxMenu::Create(text, callback, ..., "传送", "复活", nullptr)` 动态按钮 |
| `Resurrect()` / 保物品复活法术 | 原生：`Resurrect(false,true)` + 物品快照/恢复（遍历 `GetInventory()`，记录 base+数量+是否装备）→ 复活后重新添加并装备；无需容器参照与法术记录 |
| `BlacklistList`（FormList，cosave 保存） | 原生：`std::unordered_set<RE::FormID>`（按 ActorBase），存 SKSE co-save；另支持从 JSON 导入 |
| `StorageUtil "NoObituary"`（PapyrusUtil 存储） | **无法原生读取**；改为读取 `NoObituary` 列表（JSON/ini），行为差异见 §4 |
| `EntryArray`/`DetailsArray`（Papyrus 属性，cosave） | `std::deque<Entry>`（entry/details 字符串），SKSE co-save 记录 `OBIT` v1 |
| Strings.json | 沿用同一路径 `Data\SKSE\Plugins\Obituary\Strings.json`（现有中文译文直接生效），失败用内置默认 |
| MCM（SKI_ConfigBase） | MCM Helper JSON：`MCM\Config\<插件名>\config.json`；设置持久化到 `Data\MCM\Settings\<插件名>.ini`，DLL 读取该 ini |
| 日志浏览页 | SKSE Menu Framework（ImGui）表格 + 详情；MCM 页保留设置项 |

### 目录/文件（新 mod）

```
ObituarySKSE\
├─ SKSE\Plugins\ObituarySKSE.dll
├─ SKSE\Plugins\ObituarySKSE\config.ini      ; 兜底默认 + NoObituary 名单
├─ MCM\Config\ObituarySKSE\config.json       ; MCM Helper 菜单
└─ (可选) ObituarySKSE.esp                   ; 仅当 MCM Helper 必须有同名插件才生成（空插件）
```

不改动原 mod；安装后由用户停用原 mod（两个 mod 的 esp 不冲突，但功能会重复触发）。

### co-save 布局（记录 `OBIT`，版本 1）

```
u32 version
u32 entryCount
  repeated: u16 len + utf8 entry, u16 len + utf8 details
u32 blacklistCount
  repeated: u32 formID (ActorBase)
```

读档时清空并恢复；`Revert` 时清空。

---

## 3. 里程碑

| 阶段 | 交付 | 验收 |
|---|---|---|
| M1 | 工程能编译 + TESDeathEvent 打到 SKSE 日志 | 进游戏杀一个 NPC，日志出现记录 |
| M2 | 过滤器 + 环形日志 + 消息框通知 | 按设置分别命中/不命中，消息框按钮可点 |
| M3 | 传送 / 复活 / 保物品复活 + 文件写入 | 复活后物品与装备一致；日志文件按格式生成 |
| M4 | MCM JSON + 设置 ini 生效 | MCM 改动即时影响行为，重启保留 |
| M5 | SKSE Menu Framework 菜单（日志浏览、黑名单、清空） | 菜单能翻页、看详情、加入黑名单后不再记录 |
| M6 | co-save + 打包 | 存读档后日志仍在；禁用原 mod 只留新 mod 正常工作 |

---

## 5. 实施状态（2026-11-08）

### 已落地

| 模块 | 文件 | 状态 |
|---|---|---|
| 工程 / 构建 | `ObituarySKSE\CMakeLists.txt`、`build.ps1` | ✅ 离线编译通过（VS2026 v145 / MSVC 14.51，MSBuild 生成器） |
| 入口/声明/序列化注册 | `src\plugin.cpp` | ✅ 运行时白名单含 1.7.104.0 |
| 设置读写（MCM 兼容 ini） | `src\Config.h/.cpp` | ✅ 键名与原版属性一致 |
| 字符串（沿原版 Strings.json + 覆盖文件） | `src\Localization.h/.cpp` | ✅ 中文译文直接复用 |
| 死亡事件 + 过滤 + 条目格式化 | `src\Obituary.h/.cpp` | ✅ TESDeathEvent sink，原版 2 秒延迟队列 |
| 通知（消息框/角落通知）、传送、复活 | `src\Notifier.cpp` | ✅ 消息框用 `MessageBoxMenu::Create`，通知走 VM `Debug.Notification` |
| 日志环形缓冲 + 写文件 + co-save | `src\Obituary.cpp` | ✅ 记录 `OBIT` v1，含黑名单 |
| 发布包 | `package\ObituarySKSE\` | ✅ DLL + 设置 + MCM JSON + README |

产物：`ObituarySKSE\build-vs\Release\ObituarySKSE.dll`（693,248 字节）

### 待办 / 阻塞

1. **游戏内验证**：从未运行过（未动 E 盘）。首次运行重点看
   `%Documents%\My Games\Skyrim Special Edition\SKSE\ObituarySKSE.log`。
2. **MCM 菜单**：MCM Helper 的 JSON 菜单必须挂在一个继承了 `MCM_ConfigBase` 的 Papyrus 任务脚本上
   （DLL 里有 `Papyrus::MCM_ConfigBase::LoadConfig(TESQuest*)`、`modName did not match plugin` 等证据），
   纯原生插件无法自建该脚本。JSON 已随包发布，但**大概率不会出现菜单**，需要三选一：
   - (a) 接受：设置只放在 SKSE Menu Framework 菜单里；
   - (b) 实验：空插件 + 把预编译 `MCM_ConfigBase` 直接挂到任务上（需解包 MCMHelper.bsa 验证）；
   - (c) 装 Creation Kit 编译一个只有几十行的 `MCM_ConfigBase` 子类脚本（唯一引入 Papyrus 的路线）。
3. **SKSE Menu Framework 菜单**：本机没有 `SKSEMenuFramework.h`、也没有 imgui。
   需要浏览器下载头文件（命令行联网被拦），否则日志浏览/黑名单管理没有 UI。
4. **关系等级**：用 `BGSRelationship::GetRelationship` + `4 - level` 还原 Papyrus 的 rank，需实机抽样核对。
5. **通知**：VM `DispatchStaticCall("Debug","Notification")` 未实机验证；失败则退化为消息框。

---

## 6. 已知差异与风险（需知情）

1. **PapyrusUtil 的 `NoObituary` 豁免读不到**：原版靠 StorageUtil（PapyrusUtil 的 cosave 存储）。新实现改用
   自己的名单（config.ini / 菜单）。若有别的 mod 依赖给它打豁免，行为会不同。
2. **旧存档里的历史日志不迁移**：原版日志在 Papyrus cosave；新 mod 从空开始（首次启用时）。
3. **MCM 日志页的动态列表做不了**（MCM Helper 只支持静态控件）：日志浏览放到 SKSE Menu Framework 菜单。
   MCM 保留全部设置项。
4. **SKSE Menu Framework 头文件缺失**：本机没有 `SKSEMenuFramework.h`（`external\` 为空），也没有 imgui；
   需要你浏览器下载后我才能编 ImGui 菜单。核心功能（M1–M4、M6）不依赖它。
5. **MCM Helper 是否要求同名插件**待验证；若要求，需附一个空 ESP（可程序生成，几十字节）。
6. **通知**：引擎头文件里没有现成的 notification 辅助函数，需用 VM 静态调用
   `Debug.Notification`（`IVirtualMachine::DispatchStaticCall`）或地址调用；失败时退化为消息框。
---

## 7. 后续变更（与最初设计不同）

1. **取消 MCM**：MCM Helper 的 JSON 只能做“绑定设置值”的控件，`input` 控件要求脚本对象带
   `_inputStartText`，我们的任务挂的是预编译基类，点击会崩在 `MCMHelper!OnOptionInputOpen`。
   因此 MCM 整体移除（含 `ObituarySKSE.esp` 空壳），界面只保留 F1 菜单。
   设置文件也从 `Data/MCM/Settings/` 迁到 `Data/SKSE/Plugins/ObituarySKSE/settings.ini`，旧路径自动迁移。
2. **新增「传送并复活」**（原版没有）：先 `MoveTo`，确认玩家与目标同格且距离 ≤300 后再复活；
   10 秒内未到位则取消并提示 `tpnotarrived`。
3. **关系等级**：最终采用 Papyrus `Actor.GetRelationshipRank`（运行时值），
   `BGSRelationship` 查表只作 VM 调用失败时的兜底——原版行为已证实运行时值可能为负，
   而插件文件里可能根本没有对应 `RELA` 记录。
