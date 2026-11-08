# ObituarySKSE 本地化说明

所有界面文本都在这两个文件里，键名不要改：

- `SKSE\Plugins\ObituarySKSE\Strings.json` —— 插件与 F1 菜单的运行时文本（默认简体中文）
- `MCM\Config\ObituarySKSE\config.json` —— MCM 菜单的控件文本（MCM Helper 只认字面量）

## 翻译步骤

1. 复制 `Strings.json` 为你的语言版本，逐条改值（保留 `%s` `%d` 占位符和 `\n`）；
2. 改完跑一次 `tools\localization.py --strings Strings.json --out <模组根目录>`，
   MCM 的 config.json 会用同一份文本重新生成；
3. 不想跑脚本也可以直接手改 `config.json` 里的 `text` / `help` 字段。

## 键表

| 键 | 当前值 |
|---|---|
| `blacklistadd` | 加入黑名单 |
| `blacklistaddask` | 将目标角色加入黑名单？ |
| `blacklistadddone` | 目标角色已加入黑名单。 |
| `blacklistaddinfo` | 基于角色基底将目标角色加入黑名单。 |
| `blacklistcheck` | 已列入黑名单 |
| `blacklistclear` | 清空黑名单 |
| `blacklistclearask` | 清空黑名单？ |
| `blacklistcleardone` | 黑名单已清空。 |
| `blacklistclearinfo` | 清空黑名单。 |
| `blacklistinput` | 黑名单（FormID 或 EditorID，逗号分隔） |
| `blacklistinputinfo` | 手动排除的 ActorBase：FormID（0001A67C）或 EditorID（Nazeem），用逗号分隔。\n留空表示不排除。也可以在 F1 菜单里用准星选中目标一键加入。 |
| `blacklistpage` | 黑名单 |
| `blacklistremove` | 移出黑名单 |
| `blacklistremoveask` | 将目标角色移出黑名单？ |
| `blacklistremovedone` | 目标角色已移出黑名单。 |
| `blacklistremoveinfo` | 基于角色基底将目标角色移出黑名单。 |
| `blacklistsettings` | 黑名单设置 |
| `blacklisttarget` | 目标 |
| `btnnothing` | 离开 |
| `btnresurrect` | 复活 |
| `btnteleport` | 传送 |
| `btnteleportresurrect` | 传送并复活 |
| `clearlog` | 清除日志 |
| `clearlogask` | 清除日志？ |
| `clearlogdone` | 日志已清除！ |
| `clearloginfo` | 清除日志。 |
| `click` | 点击 |
| `female` | 女  |
| `firstpage` | 第一页 |
| `hasbeenresurrected` | %s 已被复活。 |
| `hasdied` | %s 已经死亡。 |
| `labelformid` | 表单ID： |
| `labelkiller` | 击杀者： |
| `labelmaster` | 主文件： |
| `labelname` | 姓名： |
| `labelrace` | 种族： |
| `labelsex` | 性别： |
| `labeltimeofdeath` | 死亡时间： |
| `labelunique` | 特殊： |
| `log` | 日志 |
| `logallowplayer` | 允许玩家 |
| `logallowplayerinfo` | 若启用，允许日志记录玩家的死亡。\n默认：关闭 |
| `logcreatures` | 生物 |
| `logcreaturesinfo` | 若启用，生物的死亡将被记录。\n默认：开启 |
| `logenabled` | 日志启用 |
| `logenabledinfo` | 若启用，NPC的死亡将被记录。\n默认：开启 |
| `logplayerkill` | 玩家击杀 |
| `logplayerkillinfo` | 若启用，被玩家击杀的NPC将被记录。\n默认：开启 |
| `logsettings` | 日志设置 |
| `loguniqueonly` | 仅特殊 |
| `loguniqueonlyinfo` | 若启用，仅记录带有特殊标记的NPC的死亡。\n默认：开启 |
| `logwritetofile` | 写入文件 |
| `logwritetofileinfo` | 若启用，NPC死亡将被记录到文件中。\n文件位于 Data/SKSE/Plugins/Obituary/Logs/<玩家名>/<游戏小时>.txt\n默认：关闭 |
| `male` | 男  |
| `maxentriesperpage` | 每页最大条目数 |
| `maxentriesperpageinfo` | 日志浏览页每页显示多少条。\n默认：16 |
| `maxlogsize` | 最大日志大小 |
| `maxlogsizedone` | 最大日志大小已更改为 %d。 |
| `maxlogsizeinfo` | 可记录到MCM中的最大条目数。\n值越高，处理时间越长。\n默认：1024 |
| `menuadd` | 加入黑名单 |
| `menublacklist` | 黑名单 |
| `menublacklisted` | 已列入：%d 个 |
| `menuclear_blacklist` | 清空黑名单 |
| `menuclear_log` | 清除日志 |
| `menuentries` | 条目：%d / %d     页：%d / %d |
| `menufirst` | 第一页 |
| `menuheader_log` | 日志设置 |
| `menuheader_notif` | 通知设置 |
| `menulog` | 日志 |
| `menumethodlabel` | 方式 0=消息框 1=通知 |
| `menunext` | 下一页 |
| `menunotarget` | 准星目标：无 |
| `menuprev` | 上一页 |
| `menurelabel` | 最低关系 |
| `menurelvalues` | -4 宿敌 / -3 敌人 / -2 仇敌 / -1 对手 / 0 相识 / 1 朋友 / 2 知己 / 3 盟友 / 4 爱人 |
| `menuremove` | 移出黑名单 |
| `menusettings` | 设置 |
| `menutarget` | 准星目标：%s |
| `menutitle` | 讣告 Obituary |
| `menuunique` | 仅特殊（Unique） |
| `modenabled` | 模组启用 |
| `modenabledinfo` | 模组是否激活。 |
| `next10pages` | 下10页 |
| `nextpage` | 下一页 |
| `no` | 否  |
| `noname` | [无名] |
| `none` | [无] |
| `notarget` | [无目标] |
| `notenoughdragonsouls` | 你没有足够的龙魂来复活 %s。 |
| `notifcreatures` | 生物 |
| `notifcreaturesinfo` | 若启用，生物死亡时你将收到通知。\n默认：开启 |
| `notificationenabled` | 通知启用 |
| `notificationenabledinfo` | 若启用，NPC死亡时你将收到通知。\n默认：关闭 |
| `notificationsettings` | 通知设置 |
| `notifmethod` | 方式 |
| `notifmethodinfo` | 通知玩家死亡的方式。\n默认：消息框 |
| `notifmethodmessagebox` | 消息框 |
| `notifmethodnotification` | 通知 |
| `notifminrelationship` | 最低关系 |
| `notifminrelationshipinfo` | 显示通知所需的最低关系等级。\n默认：0 相识 |
| `notifplayerkill` | 玩家击杀 |
| `notifplayerkillinfo` | 若启用，被玩家击杀的NPC死亡时将通知你。\n默认：关闭 |
| `notifresurrectdragonsoulcost` | 复活消耗龙魂 |
| `notifresurrectdragonsoulcostinfo` | 复活选项所需消耗的龙魂数量。\n默认：0 |
| `notifresurrectedkeepsitems` | 复活后保留物品 |
| `notifresurrectedkeepsitemsinfo` | 若启用，复活的NPC将保留复活前的物品，\n而不是在物品栏中获得一套新物品。\n默认：开启 |
| `notifresurrectoption` | 复活选项 |
| `notifresurrectoptioninfo` | 若启用，在死亡通知中添加复活选项。\n默认：关闭 |
| `notifteleportoption` | 传送选项 |
| `notifteleportoptioninfo` | 若启用，在死亡通知中添加传送选项。\n默认：关闭 |
| `notifteleportresurrectoption` | 传送并复活选项 |
| `notifteleportresurrectoptioninfo` | 若启用，在死亡通知中添加「传送并复活」：先传送到目标身边，确认到位后再复活。\n默认：开启 |
| `notifuniqueonly` | 仅特殊 |
| `notifuniqueonlyinfo` | 若启用，仅特殊NPC死亡时通知。\n默认：开启 |
| `page` | 页： |
| `previouspage` | 上一页 |
| `processinglogmsg` | 日志仍在更新。 |
| `processingpleaseclosemsg` | 请关闭所有菜单。 |
| `processingsettingsmsg` | 此模组的一个功能仍在处理中。 |
| `relacquaintance` | 0 相识 |
| `relally` | 3 盟友 |
| `relarchnemesis` | -4 宿敌 |
| `relconfidant` | 2 知己 |
| `relenemy` | -3 敌人 |
| `relfoe` | -2 仇敌 |
| `relfriend` | 1 朋友 |
| `rellover` | 4 爱人 |
| `relrival` | -1 对手 |
| `restoredefaults` | 恢复默认 |
| `restoredefaultsask` | 恢复设置为默认值？ |
| `restoredefaultsdone` | 已恢复默认设置！ |
| `restoredefaultsinfo` | 恢复默认设置。 |
| `settings` | 设置 |
| `totalentries` | 总条目： |
| `tpnotarrived` | 传送未完成，已取消复活。 |
| `yes` | 是  |
