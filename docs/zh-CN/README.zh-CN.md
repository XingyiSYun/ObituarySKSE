ObituarySKSE —— 讣告（Obituary）原生重写版
================================================

用 SKSE 插件（C++ / CommonLibSSE-NG）重写的 Obituary 功能，**不依赖原模组的 Papyrus 脚本**。
可观察行为对齐原版：死亡捕获、过滤、日志、通知（消息框/角落通知）、传送、复活（含保留物品/龙魂消耗），
并额外增加了原版没有的「**传送并复活**」。


安装
----
1. 把本文件夹整个放进 MO2 的 mods 目录，启用。
2. 本插件**可单独安装**，不需要原版 Obituary（无需它的 esp 和脚本）。
   如果你已经装了原版，请停用它的 esp/脚本——两套同时跑，同一次死亡会记录/通知两次。
3. 需要：SKSE64（本机 2.3.1）、Address Library for SKSE Plugins；
   F1 菜单需要 SKSE Menu Framework 3.x（未安装时插件照常工作，只是没有菜单）。
   **不再需要 MCM Helper / SkyUI** —— 本版没有 MCM 菜单。
4. 字符串：优先读本模组的 Data\SKSE\Plugins\ObituarySKSE\Strings.json；
   没有时回退到原模组的 Data\SKSE\Plugins\Obituary\Strings.json。


菜单（只有 F1 一个入口）
------------------------
按 F1 → 「讣告 Obituary」，三页：

  设置     全部开关与滑条（与原版 MCM 的设置项一一对应）
  日志     条目浏览：翻页、点条目看八行详情、清空日志
  黑名单   准星选中目标一键加入/移出、清空

设置同时写进 Data\SKSE\Plugins\ObituarySKSE\settings.ini。


死亡通知的四个按钮
------------------
消息框里按开启的选项动态出现：

  离开           不做任何事
  传送           只把玩家传送到死亡 NPC 身边
  复活           只复活（含龙魂消耗/保留物品规则）
  传送并复活     先传送，**确认玩家真的到位**（同格且距离 ≤300）后再复活；
                 10 秒内没到位则取消并提示「传送未完成，已取消复活」

「传送并复活」是本模组新增的，原版只有前两个。


日志文件
--------
勾选「写入文件」后：Data\SKSE\Plugins\Obituary\Logs\<玩家名>\GameHour<N>.txt
格式与原版一致：
    -
    <名字> (MM/DD/YYYY HH:MM)
    Name:... / Sex:... / Race:... / Unique:... / Master:... / Form ID:... / Killer:... / Time of Death:...


存档
----
日志与黑名单写在 SKSE co-save（记录 'OBIT' v1）。旧存档里原版 Papyrus 的历史日志**不会迁移**。


验收清单
--------
[ ] skse64.log 出现 "ObituarySKSE ... loaded correctly"
[ ] ObituarySKSE.log 出现 "TESDeathEvent sink registered"、"settings notif=... "
[ ] 杀一个有名 NPC：日志页出现条目；按设置该弹的时候弹消息框
[ ] 消息框里出现「传送并复活」，点了先传送、到位后才复活
[ ] 复活后物品与装备与死亡前一致（默认「保留物品」开启）
[ ] 龙魂不足时提示且不复活
[ ] 存读档后日志仍在；黑名单仍在


本地化
------
界面文本都在 SKSE\Plugins\ObituarySKSE\Strings.json（键名不要改）：
  Strings.en.json 是英文模板，LOCALIZATION.md 是键表，tools\localization.py 可重新生成。
MCM 已经去掉，所以不再需要生成 config.json。


已知差异（相对原版）
--------------------
1. **没有 MCM 菜单**，所有设置与日志/黑名单操作都在 F1 菜单（原版在 SkyUI MCM 里）。
2. PapyrusUtil 的 "NoObituary" 豁免读不到（原版靠 StorageUtil）；本版用黑名单代替。
3. 关系等级取自游戏运行时（Papyrus GetRelationshipRank），因此会受存档/其它 mod 改动影响，
   这一点与原版一致。
4. 复活时的物品处理改为原生实现（不放置临时容器），结果等价但不产生临时引用。
5. 首次启用时日志为空（不迁移旧存档数据）。